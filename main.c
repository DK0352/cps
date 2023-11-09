/*
    cps - a program that copies and synchronizes files and directories
    Copyright (C) 2023  Danilo Kovljanić

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "main.h"
#include "dlist.h"
#include "data_copy_info.h"
#include "errors.h"
#include "options.h"
#include <getopt.h>
#define BUF 100

struct copied_or_not {
	int copied_data;		// if 1, add size in stats
	int aborted_copying;		// if 1, user aborted copying missing files and dirs
	int copied_surplus;		// if 1, add size in stats
	int deleted_surplus;		// if 1, subtract size in stats
	int ow_smaller;
	int ow_larger;
	int ow_newer;
	int ow_older;
	int full_dir1_copied;		// if 1, add size in stats
	int full_dir2_copied;		// if 1, add size in stats
};

int open_dirs(struct thread_struct *thread_data);	// function to open and read directories
void build_tree(struct thread_struct *thread_data);	// build (file) tree
int compare_trees(struct thread_struct *thread_data_a, struct thread_struct *thread_data_b); 		// compare file trees a and b
int read_write_data(DList *, int choose, char *source, char *destination);				// read and write files and directories that should be copied, or delete them if specified
int clean_tree(DList_of_lists *, short);								// free the dynamically allocated file tree
int write_contents_to_file(DList_of_lists *directory, short opened, int f_descriptor);			// write the file trees to a file
void list_stats(int after_c, struct copied_or_not copied);
void destroy_data_structs(void);

int full_dir_write = 0;		// if set to 1, copy the complete source directory, if set to 2, copy the complete destination directory
char file_loc1[PATH_MAX];	// file tree content file location
struct options_menu options;				// data structure used to set options
struct Data_Copy_Info data_copy_info;			// data structure with statistical info, holds lists of files and dirs to copy...

int main(int argc, char *argv[])
{
	dev_t major1, major2, minor1, minor2;				// used to determine whether the two locations to compare are on the same disk, or on a separate disk.
	pthread_t th1, th2, th3, th4;					// threads used to read directories with build_tree() function concurently if two locations to compare are on separate disks.
	int th1_status, th2_status;					// if concurent threads access is used, these are return values for the threads
	int th3_status, th4_status;					// if concurent threads access is used, these are return values for the threads
	void *th1_retval, *th2_retval;
	void *th3_retval, *th4_retval;
	struct thread_struct *thread_data_a, *thread_data_b;		// main data structure for reading directories a (source) and b (destination). despite it's name, it is used even if threads are not used.
	DList *file_list, *dir_list, *file_surp_list, *dir_surp_list;	// file and directory lists, file and directory surplus lists
	DList *file_ms_list, *file_ml_list;				// file main smaller and main larger lists
	DList *file_mn_list, *file_mo_list;				// file main newer and main smaller lists
	DListElmt *file_list_element, *dir_list_element;		// used to loop through file and directory lists to display files and diretories to copy, etc...
	struct copied_or_not copied;					// determine if user enterted yes to copy data
	extern struct options_menu options;				// options for program
	extern struct errors_data error;				// errors information
	extern int full_dir_write;					// if 1, the destination is empty, so copy the complete source. if 2, the source is empty, so copy the complete destination if you wish
	int use_threads = 0;						// in case source and destination directories are on different disk, use threads is set to 1
	int open_linearly = 0;
	int len;
	char *pathname1, *pathname2;					// pathnames for directory1 and directory2
	extern char file_loc1[PATH_MAX];				// location of the text file with the complete source and destination file trees
	char file_loc2[PATH_MAX];					// location of the text file with the data to copy content file location
	const char *src = "source";
	const char *dst = "destination";
	char line[BUF];				// get yes or no answer from the user
	int length;
	int copy_files;				// there are files to copy if 1
	int copy_dirs;				// there are directories to copy if 1
	int files_surplus;			// there are surplus files to copy or delete if 1
	int dirs_surplus;			// there are surplus directories to copy or delete if 1
	int ow_main_smaller;			// there are files with the same name, smaller in the main location to potentially overwrite
	int ow_main_larger;			// there are files with the same name, larger in the main location to potentially overwrite
	int ow_main_newer;			// there are files with the same name, newer in the main location to potentially overwrite
	int ow_main_older;			// there are files with the same name, older in the main location to potentially overwrite
	struct stat buf1, buf2; 		// used to test source and destination arguments for program, whether arguments are directories and devices they are located on.
	int copyfile;				// file descriptor for copy content file
	int c;					// for getopt_long
	int ind1 = 0;				// used for checking argv arguments
	int ind2 = 0;				// used for checking argv arguments
	int index;				// used for checking argv arguments
	char *version = "1.0.5";		// cps version number

	char *string1 = "Files copied:\n";
	char *string2 = "Directories copied:\n";
	char *string3 = "Files overwritten (smaller)\n";
	char *string4 = "Files overwritten (larger)\n";
	char *string5 = "Surplus files copied back:\n";
	char *string6 = "Surplus directories copied back:\n";
	char *string7 = "Surplus files deleted:\n";
	char *string8 = "Surplus directories deleted:\n";
	char *string9 = "Files overwritten (newer)\n";
	char *string10 = "Files overwritten (older)\n";

	char *help_string1 = "--copy-surplus-back or -b";
	char *help_string2 = "Copy the surplus data from the secondary (directory 2) location into the main location (directory 1) while synchronizing the directories.";
	char *help_string3 = "--delete-surplus or -x";
	char *help_string4 = "Delete the surplus data from the secondary (directory 2) location while synchronizing the directories.";
	char *help_string5 = "--overwrite-with-smaller or -s";
	char *help_string6 = "If two files with the same name are found, overwrite the larger file in the secondary location with the smaller from the main location.";
	char *help_string7 = "--overwrite-with-larger or -l";
	char *help_string8 = "If two files with the same name are found, overwrite the smaller file in the secondary location with the larger file from the main location.";
	char *help_string9 = "--list-conflicting or -L";
	char *help_string10 = "List files with the same name, but different size or modification time.";
	char *help_string11 = "--list-surplus or -f";
	char *help_string12 = "Just list surplus files and directories, but dont copy them.";
	char *help_string13 = "--dont-list-data-to-copy or -d";
	char *help_string14 = "Don't list the files and directories to copy after scaning.";
	char *help_string15 = "--help or -h";
	char *help_string16 = "Show help and options.";
	char *help_string17 = "--content-file=[FILE] or -c";
	char *help_string18 = "Write the content of both directories to a file before copying.";
	char *help_string19 = "--just-content-file=[FILE] or -C";
	char *help_string20 = "Just write the content of both directories to a file and exit the program.";
	char *help_string21 = "--copy-content-file=[FILE] or -k";
	char *help_string22 = "Write the files and directories to copy into a file.";
	char *help_string23 = "--just-copy-content-file=[FILE] or -K";
	char *help_string24 = "Just write the files and directories to copy into a file and exit the program.";
	char *help_string25 = "--dont-list-stats or -D";
	char *help_string26 = "Don't list statistics about the file and directory size, number, etc.";
	char *help_string27 = "--dont-quit-read-errors or -y";
	char *help_string28 = "Don't quit on read errors. Useful for unexpected permissions on a file or directory.";
	char *help_string29 = "--dont-quit-write-errors or -z";
	char *help_string30 = "Don't quit on write errors. Useful for unexpected permissions on a file or directory.";
	char *help_string31 = "--dont-quit-delete-errors or -p";
	char *help_string32 = "Don't quit if deleting the file or directory fails. Useful for unexpected permissions on a file or directory.";
	char *help_string33 = "--no-questions or -q";
	char *help_string34 = "Don't ask for confirmation to write the data.";
	char *help_string35 = "--unit=OPTION";
	char *help_string36 = "Show sizes in unit of a choice (KB, MB, GB, TB, example: --unit=MB) insted of the default unit appropriate for the size.";
	char *help_string37 = "--si-units";
	char *help_string38 = "Use powers of 1000 instead of the default 1024";
	char *help_string39 = "--dont-show-read-process or -r";
	char *help_string40 = "Don't list files and directories currently reading.";
	char *help_string41 = "--dont-show-write-process or -w";
	char *help_string42 = "Don't list files and directories currently writing.";
	char *help_string43 = "--just-copy-surplus-back or -B";
	char *help_string44 = "Just copy the surplus data from the secondary (directory 2) location into the main location (directory 1), but don't synchronize directories.";
	char *help_string45 = "--follow-sym-links or -S";
	char *help_string46 = "Follow symbolic links.";
	char *help_string47 = "--no-access-time or -a";
	char *help_string48 = "Do not update the last access time on files in the source directory during copying.";
	char *help_string49 = "--preserve-atime or -A";
	char *help_string50 = "Preserve access time on the data to be copied.";
	char *help_string51 = "--preserve-mtime or -M";
	char *help_string52 = "Preserve modification time on the data to be copied.";
	char *help_string53 = "--overwrite-with-newer or -N";
	char *help_string54 = "If two files with the same name are found, overwrite the older file in the secondary location with the newer from the main location.";
	char *help_string55 = "--overwrite-with-older or -O";
	char *help_string56 = "If two files with the same name are found, overwrite the newer file in the secondary location with the older file from the main location.";
	char *help_string57 = "--naive-mode or -n";
	char *help_string58 = "Scan only directories with difference in size. This won't detect the case where some files/dirs have swapped places in the file tree and the size has remained the same.";
	char *help_string59 = "--time-mode or -T";
	char *help_string60 = "Scan based on modification time instead of size.";
	//char *help_string61 = "--detailed or -D";
	//char *help_string62 = "Show detailed information about each file (size, owner, permissions, last modification time) for files to copy/overwrite and content file.";

	// 0 option is inactive, 1 option is active
	options.quit_read_errors = 1;		// on by default
	options.quit_write_errors = 1;		// on by default
	options.quit_delete_errors = 1;		// on by default
	options.copy_surplus_back = 0;
	options.just_copy_surplus_back = 0;
	options.delete_surplus = 0;
	options.ow_main_smaller = 0;
	options.ow_main_larger = 0;
	options.follow_sym_links = 0;
	options.list_surplus = 0;
	options.dont_list_data_to_copy = 0;
	options.help = 0;
	options.write_content_file = 0;
	options.just_write_content_file = 0;
	options.write_copy_content_file = 0;
	options.just_write_copy_content_file = 0;
	options.dont_list_stats = 0;
	options.no_questions = 0;
	options.other_unit = 0;
	options.si_units = 0;
	options.show_read_proc = 1;		// on by default
	options.show_write_proc = 1;		// on by default
	options.open_flags = 0;
	options.noatime = 0;
	options.time_mods = 0;
	options.preserve_a_time = 0;
	options.preserve_m_time = 0;
	options.time_based = 0;
	options.size_based = 1;			// on by default
	options.ow_main_newer = 0;
	options.ow_main_older = 0;
	options.naive_mode = 0;
	options.list_conflicting = 0;
	//options.detailed = 0;

	copied.copied_data = 0;		// if 1, add size in stats
	copied.aborted_copying;		// if 1, user aborted copying missing files and dirs
	copied.copied_surplus = 0;	// if 1, add size in stats
	copied.deleted_surplus = 0;	// if 1, subtract size in stats
	copied.ow_smaller = 0;
	copied.ow_larger = 0;
	copied.full_dir1_copied = 0;	// if 1, add size in stats
	copied.full_dir2_copied = 0;	// if 1, add size in stats

	data_copy_info.files_to_copy_list = NULL;
	data_copy_info.dirs_to_copy_list = NULL;
	data_copy_info.files_surplus_list = NULL;
	data_copy_info.dirs_surplus_list = NULL;
	data_copy_info.diff_size_ms_list = NULL;
	data_copy_info.diff_size_ml_list = NULL;
	data_copy_info.global_files_to_copy_num = 0;
	data_copy_info.global_files_to_copy_size = 0;
	data_copy_info.global_dirs_to_copy_num = 0;
	data_copy_info.global_dirs_to_copy_size = 0;
	data_copy_info.global_files_surplus_num = 0;
	data_copy_info.global_files_surplus_size = 0;
	data_copy_info.global_dirs_surplus_num = 0;
	data_copy_info.global_dirs_surplus_size = 0;
	data_copy_info.global_diff_size_ms_num = 0;
	data_copy_info.global_diff_size_ms_size = 0;
	data_copy_info.global_diff_size_ms_orig_size = 0;
	data_copy_info.global_diff_size_ml_num = 0;
	data_copy_info.global_diff_size_ml_size = 0;
	data_copy_info.global_diff_size_ml_orig_size = 0;
	data_copy_info.global_diff_time_mn_num = 0;
	data_copy_info.global_diff_time_mn_size = 0;
	data_copy_info.global_diff_time_mn_orig_size = 0;
	data_copy_info.global_diff_time_mo_num = 0;
	data_copy_info.global_diff_time_mo_size = 0;
	data_copy_info.global_diff_time_mo_orig_size = 0;
	data_copy_info.global_file_num_a = 0;
	data_copy_info.global_file_num_b = 0;
	data_copy_info.global_files_size_a = 0;
	data_copy_info.global_files_size_b = 0;
	data_copy_info.global_dir_num_a = 0;
	data_copy_info.global_dir_num_b = 0;
	data_copy_info.global_sym_links_num_a = 0;
	data_copy_info.global_sym_links_num_b = 0;
	data_copy_info.global_sym_links_size_a = 0;
	data_copy_info.global_sym_links_size_b = 0;
	data_copy_info.dlist_of_lists_num = 0;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"copy-surplus-back", no_argument, 0, 'b' },
			{"delete-surplus", no_argument, 0, 'x' },
			{"just-copy-surplus-back", no_argument, 0, 'B' },
			{"overwrite-with-smaller", no_argument, 0, 's' },
			{"overwrite-with-larger", no_argument, 0, 'l' },
			{"list-surplus", no_argument, 0, 'f' },
			{"dont-list-data-to-copy", no_argument, 0, 'd' },
			//{"detailed", no_argument, 0, 'D' }, add it in getopt string
			{"help", no_argument, 0, 'h' },
			{"content-file", required_argument, 0, 'c' },
			{"just-content-file", required_argument, 0, 'C' },
			{"copy-content-file", required_argument, 0, 'k' },
			{"just-copy-content-file", required_argument, 0, 'K' },
			{"dont-list-stats", no_argument, 0, 'X' },
			{"dont-quit-read-errors", no_argument, 0, 'y' },
			{"dont-quit-write-errors", no_argument, 0, 'z' },
			{"dont-quit-delete-errors", no_argument, 0, 'p' },
			{"no-questions", no_argument, 0, 'q' },
			{"unit", required_argument, 0, 0 },
			{"si-units", no_argument, &options.si_units, 1 },
			{"dont-show-read-process", no_argument, 0, 'r' },
			{"dont-show-write-process", no_argument, 0, 'w' },
			{"follow-sym-links", no_argument, 0, 'S' },
			{"no-access-time", no_argument, 0, 'a' },
			{"preserve-atime", no_argument, 0, 'A' },
			{"preserve-mtime", no_argument, 0, 'M' },
			{"overwrite-with-newer", no_argument, 0, 'N' },
			{"overwrite-with-older", no_argument, 0, 'O' },
			{"naive-mode", no_argument, 0, 'n'},
			{"time-mode", no_argument, 0, 'T'}, 
			{"list-conflicting", no_argument, 0, 'L'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "abc:dfhk:lnopqrstuvwxyzABC:K:LMNOSTX", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
			case 0:
				if (strcmp(long_options[option_index].name,"unit") == 0) {
					options.other_unit = 1;
					printf("unit: ");
					if (strcmp(optarg,"KB") == 0) {
						strcpy(options.unit,"KB");
					}
					else if (strcmp(optarg,"MB") == 0) {
						strcpy(options.unit,"MB");
					}
					else if (strcmp(optarg,"GB") == 0) {
						strcpy(options.unit,"GB");
					}
					else if (strcmp(optarg,"TB") == 0) {
						strcpy(options.unit,"TB");
					}
				}
				break;
			case 'b':
				options.copy_surplus_back = 1;
				break;
			case 'x':
				options.delete_surplus = 1;
				break;
			case 's':
				options.ow_main_smaller = 1;
				break;
			case 'l':
				options.ow_main_larger = 1;
				break;
			case 'f':
				options.list_surplus = 1;
				break;
			case 'd':
				options.dont_list_data_to_copy = 1;
				break;
			case 'h':
				options.help = 1;
				break;
			case 'c':
				options.write_content_file = 1;
				strcpy(file_loc1,optarg);
				break;
			case 'C':
				options.just_write_content_file = 1;
				strcpy(file_loc1,optarg);
				break;
			case 'k':
				options.write_copy_content_file = 1;
				strcpy(file_loc2,optarg);
				break;
			case 'K':
				options.just_write_copy_content_file = 1;
				strcpy(file_loc2,optarg);
				break;
			case 'X':
				options.dont_list_stats = 1;
				break;
			case 'y':
				options.quit_read_errors = 0;	// on by default.
				break;
			case 'z':
				options.quit_write_errors = 0;	// on by default.
				break;
			case 'p':
				options.quit_delete_errors = 0;	// on by default.
				break;
			case 'q':
				options.no_questions = 1;
				break;
			case 'r':
				options.show_read_proc = 0;
				break;
			case 'w':
				options.show_write_proc = 0;
				break;
			case 'B':
				options.just_copy_surplus_back = 1;
				break;
			case 'S':
				options.follow_sym_links = 1;
				break;
			case 'a':
				options.noatime = 1;
				options.open_flags |= O_NOATIME;
				break;
			case 'A':
				options.preserve_a_time = 1;
				options.time_mods = 1;
				break;
			case 'M':
				options.preserve_m_time = 1;
				options.time_mods = 1;
				break;
			case 'N':
				options.time_based = 1;
				options.ow_main_newer = 1;
				break;
			case 'O':
				options.time_based = 1;
				options.ow_main_older = 1;
				break;
			case 'n':
				options.naive_mode = 1;
				break;
			case 'T':
				options.time_based = 1;
				break;
			case 'L':
				options.list_conflicting = 1;
				break;
			case 'D':
				options.detailed = 1;
				break;
			case '?':
				printf("%c Unknown option. Exiting.\n", optopt);
				exit(1);
				break;
			default:
				printf("Unknown option %c. Exiting.\n", c);
				exit(1);
				break;
		} // switch()
	} // while(1)

	if (argc < 2 || options.help == 1) {
		printf("\n");
		printf("Usage: cps OPTIONS directory1 directory2\n");
		printf("\n");
		printf("       directory1 (the main directory)\n");
		printf("       directory2 (the secondary directory that you wish to syncronize with the main directory).\n");
		printf("\n");
		printf("OPTIONS: (long option) or (short option) \n");
		printf("\n");
		printf("%-37s  %s\n", help_string1, help_string2);
		printf("%-37s  %s\n", help_string3, help_string4);
		printf("%-37s  %s\n", help_string43, help_string44);
		printf("%-37s  %s\n", help_string5, help_string6);
		printf("%-37s  %s\n", help_string7, help_string8);
		printf("%-37s  %s\n", help_string9, help_string10);
		printf("%-37s  %s\n", help_string53, help_string54);
		printf("%-37s  %s\n", help_string55, help_string56);
		printf("%-37s  %s\n", help_string45, help_string46);
		printf("%-37s  %s\n", help_string11, help_string12);
		printf("%-37s  %s\n", help_string13, help_string14);
		printf("%-37s  %s\n", help_string15, help_string16);
		printf("%-37s  %s\n", help_string17, help_string18);
		printf("%-37s  %s\n", help_string19, help_string20);
		printf("%-37s  %s\n", help_string21, help_string22);
		printf("%-37s  %s\n", help_string23, help_string24);
		printf("%-37s  %s\n", help_string25, help_string26);
		printf("%-37s  %s\n", help_string27, help_string28);
		printf("%-37s  %s\n", help_string29, help_string30);
		printf("%-37s  %s\n", help_string31, help_string32);
		printf("%-37s  %s\n", help_string33, help_string34);
		printf("%-37s  %s\n", help_string35, help_string36);
		printf("%-37s  %s\n", help_string37, help_string38);
		printf("%-37s  %s\n", help_string39, help_string40);
		printf("%-37s  %s\n", help_string41, help_string42);
		printf("%-37s  %s\n", help_string47, help_string48);
		printf("%-37s  %s\n", help_string49, help_string50);
		printf("%-37s  %s\n", help_string51, help_string52);
		printf("%-37s  %s\n", help_string57, help_string58);
		printf("%-37s  %s\n", help_string59, help_string60);
		printf("\n");
		printf("cps %s\n", version);
		printf("\n");
		exit(1);
	}

	if (options.naive_mode == 1 && options.time_based == 1) {
		printf("Error: naive mode cannot be used with time options. Exiting.\n");
		exit(1);
	}

	if (options.ow_main_smaller == 1 && options.ow_main_larger == 1) {
		printf("Error: two contradicting options: --overwrite-with-smaller and --overwrite-with-larger. Specify either one or the other.\n");
		exit(1);
	}

	if (options.time_based == 1) {
		options.size_based = 0;
		if (options.ow_main_newer == 1 && options.ow_main_older == 1) {
			printf("Error: Conflicting options. Both overwrite newer and overwrite older files options enabled. Exiting.\n");
			exit(1);
		}
		if (options.ow_main_smaller == 1 || options.ow_main_larger == 1) {
			printf("Error: Conflicting options: --overwrite-with-smaller and --overwrite-with-larger cannot be used with time based options.\n");
			exit(1);
		}
	}

	if (options.follow_sym_links == 0) {
		options.open_flags |= O_NOFOLLOW;
		options.stat_f = lstat;
	}

	else if (options.follow_sym_links == 1)
		options.stat_f = stat;

	if (options.copy_surplus_back == 1 && options.delete_surplus == 0)
		options.list_surplus = 1;
	if (options.copy_surplus_back == 0 && options.delete_surplus == 1)
		options.delete_surplus = 1;
	// or maybe allow this, one after the other?
	if (options.copy_surplus_back == 1 && options.delete_surplus == 1 || options.just_copy_surplus_back == 1 && options.delete_surplus == 1) {
		printf("Error: two contradicting options: --copy-surplus-back and --delete-surplus. Specify either the one or the other.\n");
		exit(1);
	}

	if (options.ow_main_smaller == 1 && options.ow_main_larger == 1) {
		printf("Error: two contradicting options: --overwrite-with-smaller and --overwrite-with-larger. Specify either one or the other.\n");
		exit(1);
	}
	if (options.write_content_file == 1 && options.just_write_content_file == 1) {
		printf("Error: two contradicting options: --write-content-file and --just-write-content-file. Specify either one or the other.\n");
		exit(1);
	}

	copy_files = 0;
	copy_dirs = 0;
	files_surplus = 0;
	dirs_surplus = 0;
	ow_main_smaller = 0;
	ow_main_larger = 0;

	// optind is the first non-option argument, so it should be a pathname for the first directory. 
	// then increment it to point to a second directory if there are more arguments, or exit with an error if there aren't
	index = 1;
	if (optind < argc) {
		while (optind < argc) {
			if (index == 1) {
				ind1 = optind;
			}
			else if (index == 2) {
				ind2 = optind;
			}
			else if (index == 3) {
				printf("You supplied too many non-option arguments to a program. Exiting.\n");
				exit(1);
			}
			index++;
			optind++;
		}
		if (ind1 == 0) {
			printf("Missing the directory1 argument. Exiting.\n");
			exit(1);
		}
		else if (ind2 == 0) {
			printf("Missing the directory2 argument. Exiting.\n");
			exit(1);
		}
	}
	else {
		printf("Missing the directory pathnames. Exiting.\n");
		exit(1);
	}

	errno = 0;
	pathname1 = realpath(argv[ind1],NULL);
	if (pathname1 == NULL) {
		printf("directory1: \n");
		perror("realpath");
		exit(1);
	}
	if (options.stat_f(pathname1, &buf1) < 0) {
		printf("directory1: \n");
		perror("lstat");
		exit(1);
	}
	if (!S_ISDIR(buf1.st_mode)) {
		printf("Directory 1 specified is actually not a directory. Exiting.\n");
		exit(1);
	}

	errno = 0;
	pathname2 = realpath(argv[ind2],NULL);
	if (pathname2 == NULL) {
		printf("directory2: \n");
		perror("realpath");
		exit(1);
	}
	if (options.stat_f(pathname2, &buf2) < 0) {
		printf("directory2: \n");
		perror("lstat");
		exit(1);
	}
	if (!S_ISDIR(buf2.st_mode)) {
		printf("Directory 2 specified is actually not a directory. Exiting.\n");
		exit(1);
	}

	major1 = major(buf1.st_dev);
	minor1 = minor(buf1.st_dev);

	major2 = major(buf2.st_dev);
	minor2 = minor(buf2.st_dev);

	// test whether directories are on the same or different disks
	if (major1 == major2)
		open_linearly = 1;

	else if (major1 != major2)
		use_threads = 1;

	if (strcmp(pathname1,pathname2) == 0) {
		printf("You supplied the same pathnames for both directories. Exiting.\n");
		exit(1);
	}

	// main data structures for reading directory content
	thread_data_a = malloc(sizeof(struct thread_struct));
	if (thread_data_a == NULL) {
		printf("error allocating thread_data_a.\n");
		exit(1);
	}
	thread_data_b = malloc(sizeof(struct thread_struct));
	if (thread_data_b == NULL) {
		printf("error allocating thread_data_b.\n");
		exit(1);
	}

	// method to identify source and destination for functions that use these data structures
	len = strlen(src)+1;
	thread_data_a->id = malloc(len);
	if (thread_data_a->id == NULL) {
		printf("thread_data_a->id malloc() error.\n");
		exit(1);
	}
	strcpy(thread_data_a->id,src);
	thread_data_a->other_thread_data = thread_data_b;

	len = strlen(dst)+1;
	thread_data_b->id = malloc(len);
	if (thread_data_b->id == NULL) {
		printf("thread_data_b->id malloc() error.\n");
		exit(1);
	}
	strcpy(thread_data_b->id,dst);
	thread_data_b->other_thread_data = thread_data_a;

	// open the source and destination directories, linearly if they are on the same disk, use threads otherwise
	if (open_linearly == 1 && use_threads == 0) {
		thread_data_a->directory = pathname1;
		thread_data_b->directory = pathname2;

		open_dirs(thread_data_a);
		open_dirs(thread_data_b);
	}
	else if (open_linearly == 0 && use_threads == 1) {
		thread_data_a->directory = pathname1;
		thread_data_b->directory = pathname2;
		th1_status = pthread_create(&th1,NULL,(void *)open_dirs,(void *)thread_data_a);
		if (th1_status != 0) {
			fprintf(stderr, "pthread_create() thread1 (%d)%s\n", th1_status, strerror(th1_status));
			exit(1);
		}
		th2_status = pthread_create(&th2,NULL,(void *)open_dirs,(void *)thread_data_b);
		if (th2_status != 0) {
			fprintf(stderr, "pthread_create() thread2 (%d)%s\n", th1_status, strerror(th1_status));
			exit(1);
		}
		th1_status = pthread_join(th1,(void *)&th1_retval);
		if (th1_status != 0) {
			fprintf(stderr, "pthread_join() thread1 (%d)%s\n", th1_status, strerror(th1_status));
			exit(1);
		}
		th2_status = pthread_join(th2,(void *)&th2_retval);
		if (th2_status != 0) {
			fprintf(stderr, "pthread_join() thread2 (%d)%s\n", th2_status, strerror(th2_status));
			exit(1);
		}
	}

	// build the source and destination file trees, linearly if they are on the same disk, use threads otherwise
	if (open_linearly == 1 && use_threads == 0) {
		build_tree(thread_data_a);
		build_tree(thread_data_b);
		if (compare_trees(thread_data_a,thread_data_b) == -1) {
			printf("Empty directories. Exiting.\n");
			clean_tree(thread_data_a->file_tree_top_dir,0);
			clean_tree(thread_data_b->file_tree_top_dir,0);
			free(pathname1);
			free(pathname2);
			if (thread_data_a->id != NULL)
				free(thread_data_a->id);
			if (thread_data_b->id != NULL)
				free(thread_data_b->id);
			free(thread_data_a);
			free(thread_data_b);
			exit(0);
		}
	}
	else if (open_linearly == 0 && use_threads == 1) {
		th3_status = pthread_create(&th3,NULL,(void *)build_tree,(void *)thread_data_a);
		if (th3_status != 0) {
			fprintf(stderr, "pthread_create() thread3 (%d)%s\n", th3_status, strerror(th3_status));
			exit(1);
		}
		th4_status = pthread_create(&th4,NULL,(void *)build_tree,(void *)thread_data_b);
		if (th4_status != 0) {
			fprintf(stderr, "pthread_create() thread4 (%d)%s\n", th4_status, strerror(th4_status));
			exit(1);
		}
		th3_status = pthread_join(th3,NULL);
		if (th3_status != 0) {
			fprintf(stderr, "pthread_join() thread3 (%d)%s\n", th3_status, strerror(th3_status));
			exit(1);
		}
		th4_status = pthread_join(th4,NULL);
		if (th4_status != 0) {
			fprintf(stderr, "pthread_join() thread4 (%d)%s\n", th4_status, strerror(th4_status));
			exit(1);
		}
		if (th3_status == 0 && th4_status == 0) {
			if (compare_trees(thread_data_a,thread_data_b) == -1) {
				printf("Empty directories. Exiting.\n");
				clean_tree(thread_data_a->file_tree_top_dir,0);
				clean_tree(thread_data_b->file_tree_top_dir,0);
				free(pathname1);
				free(pathname2);
				if (thread_data_a->id != NULL)
					free(thread_data_a->id);
				if (thread_data_b->id != NULL)
					free(thread_data_b->id);
				free(thread_data_a);
				free(thread_data_b);
				exit(0);
			}
		}
	}

	// if option is set: create the file with the complete file tree listed
	if (options.write_content_file == 1) {
		write_contents_to_file(thread_data_a->file_tree_top_dir,0,0);
		write_contents_to_file(thread_data_b->file_tree_top_dir,0,0);
	}
	else if (options.just_write_content_file == 1) {
		write_contents_to_file(thread_data_a->file_tree_top_dir,0,0);
		write_contents_to_file(thread_data_b->file_tree_top_dir,0,0);
		clean_tree(thread_data_a->file_tree_top_dir,0);
		clean_tree(thread_data_b->file_tree_top_dir,0);
		free(pathname1);
		free(pathname2);
		if (thread_data_a->id != NULL)
			free(thread_data_a->id);
		if (thread_data_b->id != NULL)
			free(thread_data_b->id);
		free(thread_data_a);
		free(thread_data_b);
		destroy_data_structs();

		return 0;
	}
	// copy entire source directory to the destination because it's empty
	if (full_dir_write == 1) {
		if (options.no_questions == 0) {
			if (options.dont_list_stats != 1)
				list_stats(0,copied);
			printf("\nDestination directory is empty, entire source directory will be copied. Do you want to write the data? Type yes or no ...\n");
			while (fgets(line,BUF,stdin) != NULL) {
				length = strlen(line);
				line[length-1] = '\0';
				if (strcmp(line,"yes") == 0) {
					read_write_data(NULL,3,pathname1,pathname2);
					copied.full_dir1_copied = 1;
					if (options.dont_list_stats != 1)
						list_stats(2,copied);
					clean_tree(thread_data_a->file_tree_top_dir,0);
					free(pathname1);
					free(pathname2);
					if (thread_data_a->id != NULL)
						free(thread_data_a->id);
					if (thread_data_b->id != NULL)
						free(thread_data_b->id);
					free(thread_data_a);
					free(thread_data_b);
					destroy_data_structs();

					return 0;
				}
				else if (strcmp(line,"no") == 0)
					break;
				else
					printf("Unrecognized answer. Type yes or no.\n");
			}
		}
		else {
			if (options.dont_list_stats != 1)
				list_stats(0,copied);
			read_write_data(NULL,3,pathname1,pathname2);
			if (options.dont_list_stats != 1)
				list_stats(2,copied);
			clean_tree(thread_data_a->file_tree_top_dir,0);
			free(pathname1);
			free(pathname2);
			if (thread_data_a->id != NULL)
				free(thread_data_a->id);
			if (thread_data_b->id != NULL)
				free(thread_data_b->id);
			free(thread_data_a);
			free(thread_data_b);
			destroy_data_structs();

			return 0;
		}
	}
	// copy entire destination directory to the source because it's empty
	else if (full_dir_write == 2) {
		if (options.no_questions == 0) {
			if (options.dont_list_stats != 1)
				list_stats(0,copied);
			printf("Source directory is empty, entire destination directory will be copied. Do you want to write the data? Type yes or no ...\n");
			while (fgets(line,BUF,stdin) != NULL) {
				length = strlen(line);
				line[length-1] = '\0';
				if (strcmp(line,"yes") == 0) {
					read_write_data(NULL,3,pathname2,pathname1);
					copied.full_dir2_copied = 1;
					if (options.dont_list_stats != 1)
						list_stats(2,copied);
					clean_tree(thread_data_b->file_tree_top_dir,0);
					free(pathname1);
					free(pathname2);
					if (thread_data_a->id != NULL)
						free(thread_data_a->id);
					if (thread_data_b->id != NULL)
						free(thread_data_b->id);
					free(thread_data_a);
					free(thread_data_b);
					destroy_data_structs();

					return 0;
				}
				else if (strcmp(line,"no") == 0)
					break;
				else
					printf("Unrecognized answer. Type yes or no.\n");
			}
		}
		else {
			if (options.dont_list_stats != 1)
				list_stats(0,copied);
			read_write_data(NULL,3,pathname2,pathname1);
			if (options.dont_list_stats != 1)
				list_stats(2,copied);
			clean_tree(thread_data_b->file_tree_top_dir,0);
			free(pathname1);
			free(pathname2);
			if (thread_data_a->id != NULL)
				free(thread_data_a->id);
			if (thread_data_b->id != NULL)
				free(thread_data_b->id);
			free(thread_data_a);
			free(thread_data_b);
			destroy_data_structs();

			return 0;
		}
	}
	// prepare/list all the data to copy, overwrite or delete
	file_list = data_copy_info.files_to_copy_list;
	if (file_list != NULL) {
		if (file_list->num != 0) {
			copy_files = 1;
			if (options.dont_list_data_to_copy != 1) {
				printf("\nFiles to copy:\n\n");
				for (file_list_element = file_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
					printf("file: %s\n location: %s\n new location: %s\n  size: %ld\n\n\n", file_list_element->name, file_list_element->dir_location,
					file_list_element->new_location, file_list_element->size);
			}
		}
	}
	dir_list = data_copy_info.dirs_to_copy_list;
	if (dir_list != NULL) {
		if (dir_list->num != 0) {
			copy_dirs = 1;
			if (options.dont_list_data_to_copy != 1) {
				printf("\nDirectories to copy:\n\n");
				for (dir_list_element = dir_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next)
					printf("directory: %s\n location: %s\n new location: %s\n size: %ld\n\n\n", dir_list_element->name, dir_list_element->dir_location, 
					dir_list_element->new_location, dir_list_element->size);
			}
		}
	}
	file_surp_list = data_copy_info.files_surplus_list;
	if (file_surp_list != NULL) {
		if (file_surp_list->num != 0) {
			files_surplus = 1;
			if (options.list_surplus == 1) {
				printf("\nSurplus files:\n\n");
				if (options.delete_surplus != 1) {
					for (file_list_element = file_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
						printf("file: %s\n location: %s\n new location: %s\n size: %ld\n\n\n", file_list_element->name, file_list_element->dir_location, 
						file_list_element->new_location, file_list_element->size);
				}
				else if (options.delete_surplus == 1) {
					for (file_list_element = file_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
						printf("file: %s\n location: %s\n size: %ld\n\n\n", file_list_element->name, file_list_element->dir_location, 
						file_list_element->size);
				}
			}
		}
	}
	dir_surp_list = data_copy_info.dirs_surplus_list;
	if (dir_surp_list != NULL) {
		if (dir_surp_list->num != 0) {
			dirs_surplus = 1;
			if (options.list_surplus == 1) {
				printf("\nSurplus directories\n\n");
				if (options.delete_surplus != 1) {
					for (dir_list_element = dir_surp_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next)
						printf("directory: %s\n location: %s\n new location: %s\n size: %ld\n\n\n", dir_list_element->name, dir_list_element->dir_location, 
						dir_list_element->new_location, dir_list_element->size);
				}
				else if (options.delete_surplus == 1) {
					for (dir_list_element = dir_surp_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next)
						printf("directory: %s\n location: %s\n size: %ld\n\n\n", dir_list_element->name, dir_list_element->dir_location, 
						dir_list_element->size);
				}
			}
		}
	}
	file_ms_list = data_copy_info.diff_size_ms_list;
	if (file_ms_list != NULL) {
		if (file_ms_list->num != 0) {
			if (options.ow_main_smaller == 1)
				ow_main_smaller = 1; // overwrite main smaller
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1) {
				printf("\nFiles to overwrite. (source location files smaller than destination)\n\n");
				for (file_list_element = file_ms_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
					printf("file: %s\n location: %s\n new location: %s\n size: %ld\n\n\n", file_list_element->name, file_list_element->dir_location, 
					file_list_element->new_location, file_list_element->size);
				}
			}
		}
	}
	file_ml_list = data_copy_info.diff_size_ml_list;
	if (file_ml_list != NULL) {
		if (file_ml_list->num != 0) {
			if (options.ow_main_larger == 1)
				ow_main_larger = 1; // overwrite main larger
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1) {
				printf("\nFiles to overwrite. (source location files larger than destination)\n\n");
				for (file_list_element = file_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
					printf("file: %s\n location: %s\n new location: %s\n size: %ld\n\n\n", file_list_element->name, file_list_element->dir_location, 
					file_list_element->new_location, file_list_element->size);
				}
			}
		}
	}
	file_mn_list = data_copy_info.diff_time_mn_list;
	if (file_mn_list != NULL) {
		if (file_mn_list->num != 0) {
			if (options.ow_main_newer == 1)
				ow_main_newer = 1; // overwrite main smaller
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1) {
				printf("\nFiles to overwrite. (source location files newer than destination)\n\n");
				for (file_list_element = file_mn_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
					printf("file: %s\n location: %s\n new location: %s\n size: %ld\n\n\n", file_list_element->name, file_list_element->dir_location, 
					file_list_element->new_location, file_list_element->size);
				}
			}
		}
	}
	file_mo_list = data_copy_info.diff_time_mo_list;
	if (file_mo_list != NULL) {
		if (file_mo_list->num != 0) {
			if (options.ow_main_older == 1)
				ow_main_older = 1; // overwrite main larger
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1) {
				printf("\nFiles to overwrite. (source location files older than destination)\n\n");
				for (file_list_element = file_mo_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
					printf("file: %s\n location: %s\n new location: %s\n size: %ld\n\n\n", file_list_element->name, file_list_element->dir_location, 
					file_list_element->new_location, file_list_element->size);
				}
			}
		}
	}

	// if there is some data, depending on options: copy, overwrite, delete...
	if (copy_files == 1 || copy_dirs == 1 || files_surplus == 1 || dirs_surplus == 1 ||  ow_main_smaller == 1 || ow_main_larger == 1 || ow_main_newer == 1 || ow_main_older == 1) {
		if (options.write_copy_content_file == 1 || options.just_write_copy_content_file == 1) {
			errno = 0;
			copyfile = open(file_loc2, O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
			if (copyfile == -1) {
				perror("open");
				printf("Error opening copy content file: %s\n",file_loc2);
				exit(1);
			}
			else {
				if (copy_files == 1 && options.just_copy_surplus_back != 1) {
					write(copyfile, string1, strlen(string1));
					for (file_list_element = file_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						write(copyfile, file_list_element->dir_location, strlen(file_list_element->dir_location));
						write(copyfile, "\n", 1);
					}
				}
				if (copy_dirs == 1 && options.just_copy_surplus_back != 1) {
					write(copyfile, string2, strlen(string2));
					for (dir_list_element = dir_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next) {
						write(copyfile, dir_list_element->dir_location, strlen(dir_list_element->dir_location));
						write(copyfile, "\n", 1);
					}
				}
				if (options.ow_main_larger == 1 && options.just_copy_surplus_back != 1) {
					if (ow_main_larger == 1) {
						write(copyfile, string3, strlen(string3));
						for (file_list_element = file_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							write(copyfile, file_list_element->dir_location, strlen(file_list_element->dir_location));
							write(copyfile, "\n", 1);
						}
					}
				}
				else if (options.ow_main_smaller == 1 && options.just_copy_surplus_back != 1) {
					if (ow_main_smaller == 1) {
						write(copyfile, string4, strlen(string4));
						for (file_list_element = file_ms_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							write(copyfile, file_list_element->dir_location, strlen(file_list_element->dir_location));
							write(copyfile, "\n", 1);
						}
					}
				}
				if (options.ow_main_newer == 1 && options.just_copy_surplus_back != 1) {
					if (ow_main_newer == 1) {
						write(copyfile, string3, strlen(string3));
						for (file_list_element = file_mn_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							write(copyfile, file_list_element->dir_location, strlen(file_list_element->dir_location));
							write(copyfile, "\n", 1);
						}
					}
				}
				else if (options.ow_main_older == 1 && options.just_copy_surplus_back != 1) {
					if (ow_main_older == 1) {
						write(copyfile, string4, strlen(string4));
						for (file_list_element = file_mo_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							write(copyfile, file_list_element->dir_location, strlen(file_list_element->dir_location));
							write(copyfile, "\n", 1);
						}
					}
				}
				if (options.copy_surplus_back == 1 || options.just_copy_surplus_back == 1) {
					if (files_surplus == 1) {
						write(copyfile, string5, strlen(string5));
						for (file_list_element = file_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							write(copyfile, file_list_element->dir_location, strlen(file_list_element->dir_location));
							write(copyfile, "\n", 1);
						}
					}
					if (dirs_surplus == 1) {
						write(copyfile, string6, strlen(string6));
						for (dir_list_element = dir_surp_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next) {
							write(copyfile, dir_list_element->dir_location, strlen(dir_list_element->dir_location));
							write(copyfile, "\n", 1);
						}
					}
				}
				else if (options.delete_surplus == 1) {
					if (files_surplus == 1) {
						write(copyfile, string7, strlen(string7));
						for (file_list_element = file_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							write(copyfile, file_list_element->dir_location, strlen(file_list_element->dir_location));
							write(copyfile, "\n", 1);
						}
					}
					if (dirs_surplus == 1) {
						write(copyfile, string8, strlen(string8));
						for (dir_list_element = dir_surp_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next) {
							write(copyfile, dir_list_element->dir_location, strlen(dir_list_element->dir_location));
							write(copyfile, "\n", 1);
						}
					}
				}
				errno = 0;
				if (close(copyfile) == -1)
					perror("close");
			}
			if (options.just_write_copy_content_file == 1) {
				clean_tree(thread_data_a->file_tree_top_dir,0);
				clean_tree(thread_data_b->file_tree_top_dir,0);
				free(pathname1);
				free(pathname2);
				if (thread_data_a->id != NULL)
					free(thread_data_a->id);
				if (thread_data_b->id != NULL)
					free(thread_data_b->id);
				free(thread_data_a);
				free(thread_data_b);
				destroy_data_structs();

				return 0;
			}
		} // if (options.write_copy_content_file == 1 ...
		if (options.no_questions == 0) {
			if (options.dont_list_stats != 1)
				list_stats(0,copied);
			if (copy_files == 1 || copy_dirs == 1 && options.just_copy_surplus_back != 1) {
				printf("Do you want to write the missing files and directories? Type yes or no ...\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						if (copy_files == 1) {
							if (read_write_data(data_copy_info.files_to_copy_list,1,NULL,NULL) == 0)
								printf("\nFiles written succesfully.\n");
							else
								printf("\nError writing the files.\n");
						}
						if (copy_dirs == 1) {
							if (read_write_data(data_copy_info.dirs_to_copy_list,2,NULL,NULL) == 0)
								printf("\nDirectories written succesfully.\n");
							else
								printf("\nError writing the directories.\n");
						}
						copied.copied_data = 1;
						printf("\n");
						break;
					}
					else if (strcmp(line,"no") == 0) {
						copied.aborted_copying = 1;
						printf("\n");
						break;
					}
					else
						printf("unrecognized answer. type yes or no.\n");
				}
			}
			if (options.copy_surplus_back == 1 || options.just_copy_surplus_back == 1) {
				if (dirs_surplus == 1 || files_surplus == 1) {
					printf("Do you want to write the surplus data from the destionation directory back to the source? Type yes or no ...\n");
					while (fgets(line,BUF,stdin) != NULL) {
						length = strlen(line);
						line[length-1] = '\0';
						if (strcmp(line,"yes") == 0) {
							if (files_surplus == 1)
								read_write_data(file_surp_list,1,NULL,NULL);
							if (dirs_surplus == 1)
								read_write_data(dir_surp_list,2,NULL,NULL);
							copied.copied_surplus = 1;
							printf("\n");
							break;
						}
						else if (strcmp(line,"no") == 0) {
							printf("\n");
							break;
						}
						else
							printf("Unrecognized answer. Type yes or no.\n");
					}
				}
			}
			else if (options.delete_surplus == 1) {
				if (dirs_surplus == 1 || files_surplus == 1) {
					printf("Do you want to delete the surplus data in the destination directory? Type yes or no...\n");
					while (fgets(line,BUF,stdin) != NULL) {
						length = strlen(line);
						line[length-1] = '\0';
						if (strcmp(line,"yes") == 0) {
							if (files_surplus == 1)
								read_write_data(file_surp_list,5,NULL,NULL);
							if (dirs_surplus == 1)
								read_write_data(dir_surp_list,6,NULL,NULL);
							copied.deleted_surplus = 1;
							printf("\n");
							break;
						}
						else if (strcmp(line,"no") == 0) {
							printf("\n");
							break;
						}
						else
							printf("Unrecognized answer. Type yes or no.\n");
						
					}
				}
			}
			if (ow_main_smaller == 1 && options.just_copy_surplus_back != 1) {
				printf("Scan found two files with the same name, one in the main location being smaller than the file in the secondary location. "); 
				printf("Should the secondary file be overwritten? Answer yes or no.\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						read_write_data(data_copy_info.diff_size_ms_list,4,NULL,NULL);
						copied.ow_smaller = 1;
						printf("\n");
						break;
					}
					else if (strcmp(line,"no") == 0) {
						printf("\n");
						break;
					}
					else
						printf("Unrecognized answer. Type yes or no.\n");
	 			}
			}
			if (ow_main_larger == 1 && options.just_copy_surplus_back != 1) {
				printf("Scan found two files with the same name, one in the main location being larger than the file in the secondary location. ");
				printf("Should the secondary file be overwritten? Answer yes or no.\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						read_write_data(data_copy_info.diff_size_ml_list,4,NULL,NULL);
						copied.ow_larger = 1;
						printf("\n");
						break;
					}
					else if (strcmp(line,"no") == 0) {
						printf("\n");
						break;
					}
					else
						printf("Unrecognized answer. Type yes or no.\n");
			 	}
			}
			if (ow_main_newer == 1 && options.just_copy_surplus_back != 1) {
				printf("Scan found two files with the same name, one in the main location being newer than the file in the secondary location. "); 
				printf("Should the secondary file be overwritten? Answer yes or no.\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						read_write_data(data_copy_info.diff_time_mn_list,4,NULL,NULL);
						copied.ow_newer = 1;
						printf("\n");
						break;
					}
					else if (strcmp(line,"no") == 0) {
						printf("\n");
						break;
					}
					else
						printf("Unrecognized answer. Type yes or no.\n");
	 			}
			}
			if (ow_main_older == 1 && options.just_copy_surplus_back != 1) {
				printf("Scan found two files with the same name, one in the main location being older than the file in the secondary location. ");
				printf("Should the secondary file be overwritten? Answer yes or no.\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						read_write_data(data_copy_info.diff_time_mo_list,4,NULL,NULL);
						copied.ow_older = 1;
						printf("\n");
						break;
					}
					else if (strcmp(line,"no") == 0) {
						printf("\n");
						break;
					}
					else
						printf("Unrecognized answer. Type yes or no.\n");
			 	}
			}
			if (copied.copied_data == 1 || copied.copied_surplus == 1 || copied.deleted_surplus == 1 || copied.ow_smaller == 1 || copied.ow_larger == 1)
				if (options.dont_list_stats != 1)
					list_stats(1,copied);
		} // if (options.no_questions == 0) {
		else if (options.no_questions == 1) {
			if (copy_files == 1 && options.just_copy_surplus_back != 1)
				read_write_data(data_copy_info.files_to_copy_list,1,NULL,NULL);
			if (copy_dirs == 1 && options.just_copy_surplus_back != 1)
				read_write_data(data_copy_info.dirs_to_copy_list,2,NULL,NULL);
			if (options.copy_surplus_back == 1 || options.just_copy_surplus_back == 1) {
				if (files_surplus == 1)
					read_write_data(data_copy_info.files_surplus_list,1,NULL,NULL);
				if (dirs_surplus == 1)
					read_write_data(data_copy_info.dirs_surplus_list,2,NULL,NULL);
			}
			else if (options.delete_surplus == 1) {
				if (files_surplus == 1)
					read_write_data(data_copy_info.files_surplus_list,5,NULL,NULL);
				if (dirs_surplus == 1)
					read_write_data(data_copy_info.dirs_surplus_list,6,NULL,NULL);
			}
			if (options.ow_main_smaller == 1 && options.just_copy_surplus_back != 1)
				read_write_data(data_copy_info.diff_size_ms_list,4,NULL,NULL);
			if (options.ow_main_larger == 1 && options.just_copy_surplus_back != 1)
				read_write_data(data_copy_info.diff_size_ml_list,4,NULL,NULL);
			if (options.ow_main_newer == 1 && options.just_copy_surplus_back != 1)
				read_write_data(data_copy_info.diff_time_mn_list,4,NULL,NULL);
			if (options.ow_main_older == 1 && options.just_copy_surplus_back != 1)
				read_write_data(data_copy_info.diff_time_mo_list,4,NULL,NULL);
			if (options.dont_list_stats != 1)
				list_stats(1,copied);
		}
	} // if (files_to_copy == 1 || dirs_to_copy == 1 || etc...
	else {
		printf("\nNo data to copy.\n");
		list_stats(0,copied);
	}


	// free the data structures used for file trees
	clean_tree(thread_data_a->file_tree_top_dir,0);
	clean_tree(thread_data_b->file_tree_top_dir,0);

	free(pathname1);
	free(pathname2);
	if (thread_data_a->id != NULL)
		free(thread_data_a->id);
	if (thread_data_b->id != NULL)
		free(thread_data_b->id);
	free(thread_data_a);
	free(thread_data_b);
	destroy_data_structs();

	exit(0);
}
void list_stats(int after_c, struct copied_or_not copied)
{
	void calc_size(unsigned long data_size, int other_unit);
	extern struct Data_Copy_Info data_copy_info;
	unsigned long size_to_copy;
	unsigned long after_copying_size;
	unsigned long after_copying_size_surp;
	unsigned long after_copying_file_num;
	unsigned long after_copying_dir_num;

	size_to_copy = 0;

	// dodat opciju da ovo sve napise u fajl? samo redirect stdout u fajl unutar funkcije?
	// before copying
	if (after_c == 0) {
		printf("\n");
		printf("\n");
		printf("SOURCE DIRECTORY\n");
		printf("\n");
		printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
		printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
		printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
		// calc_size(): size of files/directories in the more appropriate or user specified unit
		calc_size(data_copy_info.global_files_size_a,options.other_unit);
		printf("\n");
		printf("\n");
		printf("DESTINATION DIRECTORY\n");
		printf("\n");
		printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
		printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
		printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
		calc_size(data_copy_info.global_files_size_b,options.other_unit);
		printf("\n");
		printf("\n");
		printf("Number of individual files to copy: %ld\n", data_copy_info.global_files_to_copy_num);
		printf("Size of individual files to copy in bytes: %ld\n", data_copy_info.global_files_to_copy_size);
		calc_size(data_copy_info.global_files_to_copy_size,options.other_unit);
		printf("Number of directories to copy: %ld\n", data_copy_info.global_dirs_to_copy_num);
		printf("Size of directories to copy in bytes: %ld\n", data_copy_info.global_dirs_to_copy_size);
		calc_size(data_copy_info.global_dirs_to_copy_size,options.other_unit);
		size_to_copy = data_copy_info.global_files_to_copy_size + data_copy_info.global_dirs_to_copy_size;
		printf("Files and directories to copy: ");
		calc_size(size_to_copy,options.other_unit);
		printf("Number of surplus files: %ld\n", data_copy_info.global_files_surplus_num);
		printf("Size of surplus files in bytes: %ld\n", data_copy_info.global_files_surplus_size);
		calc_size(data_copy_info.global_files_surplus_size,options.other_unit);
		printf("Number of surplus directories: %ld\n", data_copy_info.global_dirs_surplus_num);
		printf("Size of surplus directories in bytes: %ld\n", data_copy_info.global_dirs_surplus_size);
		calc_size(data_copy_info.global_dirs_surplus_size,options.other_unit);
		printf("Same files with different size (main location smaller): %ld\n", data_copy_info.global_diff_size_ms_num);
		printf("Same files with different size (main location larger): %ld\n", data_copy_info.global_diff_size_ml_num);
		printf("Same files with different modification time (main location newer): %ld\n", data_copy_info.global_diff_time_mn_num);
		printf("Same files with different modification time (main location older): %ld\n", data_copy_info.global_diff_time_mo_num);
		printf("\n");
		printf("\n");
	}
	// after copying
	else if (after_c == 1) {
		printf("\n");
		printf("\n");
		printf("SOURCE DIRECTORY\n");
		printf("\n");
		if (options.copy_surplus_back == 1 || options.just_copy_surplus_back == 1) {
			if (copied.copied_surplus == 1) {
				after_copying_file_num = 0;
				after_copying_file_num = data_copy_info.global_file_num_a + data_copy_info.global_files_surplus_num;
				printf("Number of files: %ld\n", after_copying_file_num);
			}
			else if (copied.copied_surplus == 0) {
				printf("Aborted copying surplus data back.\n");
				printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			}
		}
		else
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
		if (options.copy_surplus_back == 1 || options.just_copy_surplus_back == 1) {
			if (copied.copied_surplus == 1) {
				after_copying_dir_num = 0;
				after_copying_dir_num = data_copy_info.global_dir_num_a + data_copy_info.global_dirs_surplus_num;
				printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
			}
			else if (copied.copied_surplus == 0) {
				printf("Aborted copying surplus data back.\n");
				printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			}
		}
		else
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
		if (options.copy_surplus_back == 1 || options.just_copy_surplus_back == 1) {
			if (copied.copied_surplus == 1) {
				after_copying_size_surp = 0;
				after_copying_size_surp = data_copy_info.global_files_size_a + data_copy_info.global_files_surplus_size + data_copy_info.global_dirs_surplus_size;
				printf("Size of directory in bytes after copying: %ld\n", after_copying_size_surp);
				// calc_size(): size of files/directories in the more appropriate or user specified unit
				calc_size(after_copying_size_surp,options.other_unit);
			}
			else if (copied.copied_surplus == 0) {
				printf("Aborted copying surplus data back: ");
				printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
				// calc_size(): size of files/directories in the more appropriate or user specified unit
				calc_size(data_copy_info.global_files_size_a,options.other_unit);
			}
		}
		else {
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a,options.other_unit);
		}
		printf("\n");
		printf("\n");
		printf("DESTINATION DIRECTORY\n");
		printf("\n");
		if (copied.copied_data == 1) {
			after_copying_file_num = 0;
			after_copying_file_num = data_copy_info.global_file_num_b + data_copy_info.global_files_to_copy_num;
			if (options.delete_surplus == 1) {
				if (copied.deleted_surplus == 1) {
					after_copying_file_num -= data_copy_info.global_files_surplus_num;
					printf("Number of files: %ld\n", after_copying_file_num);
				}
				else if (copied.deleted_surplus == 0) {
					printf("Aborted deleting surplus data.\n");
					printf("Number of files: %ld\n", after_copying_file_num);
				}
			}
			else
				printf("Number of files: %ld\n", after_copying_file_num);
			after_copying_dir_num = 0;
			after_copying_dir_num = data_copy_info.global_dir_num_b + data_copy_info.global_dirs_to_copy_num;
			if (options.delete_surplus == 1) {
				if (copied.deleted_surplus == 1) {
					after_copying_dir_num -= data_copy_info.global_dirs_surplus_num;
					printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
				}
				else if (copied.deleted_surplus == 0) {
					printf("Aborted deleting surplus data.\n");
					printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
				}
			}
			else
				printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
			after_copying_size = 0;
			after_copying_size = data_copy_info.global_files_size_b + data_copy_info.global_files_to_copy_size + data_copy_info.global_dirs_to_copy_size;
			if (options.ow_main_larger == 1) {
				if (copied.ow_larger == 1) {
					after_copying_size -= data_copy_info.global_diff_size_ml_orig_size;
					after_copying_size += data_copy_info.global_diff_size_ml_size;
				}
				else if (copied.ow_larger == 0) {
					printf("Aborted overwriting same files with different sizes.\n");
				}
			}
			else if (options.ow_main_smaller == 1) {
				if (copied.ow_smaller == 1) {
					after_copying_size -= data_copy_info.global_diff_size_ms_orig_size;
					after_copying_size += data_copy_info.global_diff_size_ms_size;
				}
				else if (copied.ow_smaller == 0) {
					printf("Aborted overwriting same files with different sizes.\n");
				}
			}
			if (options.delete_surplus == 1) {
				if (copied.deleted_surplus == 1) {
					after_copying_size -= data_copy_info.global_files_surplus_size;
					after_copying_size -= data_copy_info.global_dirs_surplus_size;
					printf("Size of directory in bytes after copying: %ld\n", after_copying_size);
					calc_size(after_copying_size,options.other_unit);
					printf("\n");
					printf("\n");
				}
				else if (copied.deleted_surplus == 0) {
					printf("Aborted deleting surplus data.\n");
					printf("Size of directory in bytes after copying: %ld\n", after_copying_size);
					calc_size(after_copying_size,options.other_unit);
					printf("\n");
					printf("\n");
				}
			}
			else {
				printf("Size of directory in bytes after copying: %ld\n", after_copying_size);
				calc_size(after_copying_size,options.other_unit);
				printf("\n");
				printf("\n");
			}
		}
		else if (copied.aborted_copying == 1) {
			printf("Aborted copying the missing files and directories (from the source to the destination): ");
			after_copying_file_num = data_copy_info.global_file_num_b;
			if (options.delete_surplus == 1) {
				if (copied.deleted_surplus == 1) {
					after_copying_file_num -= data_copy_info.global_files_surplus_num;
					printf("Number of files: %ld\n", after_copying_file_num);
				}
				else if (copied.deleted_surplus == 0) {
					printf("Aborted deleting surplus data.\n");
					printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
				}
			}
			else 
				printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
			if (options.delete_surplus == 1) {
				after_copying_dir_num = data_copy_info.global_dir_num_b;
				if (copied.deleted_surplus == 1) {
					after_copying_dir_num -= data_copy_info.global_dirs_surplus_num;
					printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
				}
				else if (copied.deleted_surplus == 0) {
					printf("Aborted deleting surplus data.\n");
					printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
				}
			}
			else {
				printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
			}
			after_copying_size = data_copy_info.global_files_size_b;
			if (options.ow_main_larger == 1) {
				if (copied.ow_larger == 1) {
					after_copying_size -= data_copy_info.global_diff_size_ml_orig_size;
					after_copying_size += data_copy_info.global_diff_size_ml_size;
				}
				else if (copied.ow_larger == 0) {
					printf("Aborted overwriting same files with different sizes.\n");
				}
			}
			else if (options.ow_main_smaller == 1) {
				if (copied.ow_smaller == 1) {
					after_copying_size -= data_copy_info.global_diff_size_ms_orig_size;
					after_copying_size += data_copy_info.global_diff_size_ms_size;
				}
				else if (copied.ow_smaller == 0) {
					printf("Aborted overwriting same files with different sizes.\n");
				}
			}
			if (options.delete_surplus == 1) {
				if (copied.deleted_surplus == 1) {
					after_copying_size -= data_copy_info.global_files_surplus_size;
					after_copying_size -= data_copy_info.global_dirs_surplus_size;
					printf("Size of directory in bytes after copying: %ld\n", after_copying_size);
					calc_size(after_copying_size,options.other_unit);
					printf("\n");
					printf("\n");
				}
				else if (copied.deleted_surplus == 0) {
					printf("Aborted deleting surplus data.\n");
					printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
					calc_size(after_copying_size,options.other_unit);
					printf("\n");
					printf("\n");
				}
			}
			else {
				printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
				calc_size(after_copying_size,options.other_unit);
				printf("\n");
				printf("\n");
			}
		}
		else if (options.delete_surplus == 1 || options.ow_main_smaller == 1 || options.ow_main_larger == 1) {
			if (options.delete_surplus == 1) {
				after_copying_file_num = data_copy_info.global_file_num_b;
				if (copied.deleted_surplus == 1) {
					after_copying_file_num -= data_copy_info.global_files_surplus_num;
					printf("Number of files: %ld\n", after_copying_file_num);
				}
				else if (copied.deleted_surplus == 0) {
					printf("Aborted deleting surplus data.\n");
					printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
				}
			}
			else
				printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
			if (options.delete_surplus == 1) {
				after_copying_dir_num = data_copy_info.global_dir_num_b;
				if (copied.deleted_surplus == 1) {
					after_copying_dir_num -= data_copy_info.global_dirs_surplus_num;
					printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
				}
				else if (copied.deleted_surplus == 0) {
					printf("Aborted deleting surplus data.\n");
					printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
				}
			}
			else {
				printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
			}
			after_copying_size = data_copy_info.global_files_size_b;
			if (options.ow_main_larger == 1) {
				if (copied.ow_larger == 1) {
					after_copying_size -= data_copy_info.global_diff_size_ml_orig_size;
					after_copying_size += data_copy_info.global_diff_size_ml_size;
				}
				else if (copied.ow_larger == 0) {
					printf("Aborted overwriting same files with different sizes.\n");
				}
			}
			else if (options.ow_main_smaller == 1) {
				if (copied.ow_smaller == 1) {
					after_copying_size -= data_copy_info.global_diff_size_ms_orig_size;
					after_copying_size += data_copy_info.global_diff_size_ms_size;
				}
				else if (copied.ow_smaller == 0) {
					printf("Aborted overwriting same files with different sizes.\n");
				}
			}
			if (options.delete_surplus == 1) {
				if (copied.deleted_surplus == 1) {
					after_copying_size -= data_copy_info.global_files_surplus_size;
					after_copying_size -= data_copy_info.global_dirs_surplus_size;
					printf("Size of directory in bytes after copying: %ld\n", after_copying_size);
					calc_size(after_copying_size,options.other_unit);
					printf("\n");
					printf("\n");
				}
				else if (copied.deleted_surplus == 0) {
					printf("Aborted deleting surplus data.\n");
					printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
					calc_size(after_copying_size,options.other_unit);
					printf("\n");
					printf("\n");
				}
			}
			else {
				printf("Size of directory in bytes: %ld\n", after_copying_size);
				calc_size(after_copying_size,options.other_unit);
				printf("\n");
				printf("\n");
			}
		}
		else {
			printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
			calc_size(data_copy_info.global_files_size_b,options.other_unit);
			printf("\n");
			printf("\n");
		}
	}
	else if (after_c == 2) {
		if (copied.full_dir1_copied == 1) {
			printf("\n");
			printf("\n");
			printf("Full directory copied.\n");
			printf("\n");
			printf("\n");
			printf("SOURCE DIRECTORY\n");
			printf("\n");
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a,options.other_unit);
			printf("\n");
			printf("\n");
			printf("DESTINATION DIRECTORY\n");
			printf("\n");
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a,options.other_unit);
		}
		else if (copied.full_dir1_copied == 0 && copied.full_dir2_copied == 0) {
			printf("\n");
			printf("\n");
			printf("Aborted copying the missing files and directories (from the source to the destination): ");
			printf("\n");
			printf("\n");
			printf("SOURCE DIRECTORY\n");
			printf("\n");
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a,options.other_unit);
			printf("\n");
			printf("\n");
			printf("DESTINATION DIRECTORY\n");
			printf("\n");
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a,options.other_unit);
		}
		if (copied.full_dir2_copied == 1) {
			printf("\n");
			printf("\n");
			printf("Full directory copied.\n");
			printf("\n");
			printf("\n");
			printf("SOURCE DIRECTORY\n");
			printf("\n");
			printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_b,options.other_unit);
			printf("\n");
			printf("\n");
			printf("DESTINATION DIRECTORY\n");
			printf("\n");
			printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_b,options.other_unit);
		}
		else if (copied.full_dir1_copied == 0 && copied.full_dir2_copied == 0) {
			printf("\n");
			printf("\n");
			printf("Aborted copying the missing files and directories (from the destination to the source): ");
			printf("\n");
			printf("\n");
			printf("SOURCE DIRECTORY\n");
			printf("\n");
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a,options.other_unit);
			printf("\n");
			printf("\n");
			printf("DESTINATION DIRECTORY\n");
			printf("\n");
			printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_b,options.other_unit);
		}
	}
}

void calc_size(unsigned long data_size, int other_unit)
{
	unsigned long power1, power2, power3, power4;
	long double unit;

	if (options.si_units == 1) {
		power1 = (unsigned long) 1000;
		power2 = (unsigned long) 1000*1000;
		power3 = (unsigned long) 1000*1000*1000;
		power4 = (unsigned long) 1000*1000*1000*1000;
	}
	else {
		power1 = (unsigned long) 1024;
		power2 = (unsigned long) 1024*1024;
		power3 = (unsigned long) 1024*1024*1024;
		power4 = (unsigned long) 1024*1024*1024*1024;
	}

	if (options.other_unit != 1) {
		if (data_size != 0) {
			if (data_size >= power4) {
				unit = (long double) data_size / power4;
				printf("Size in terabytes: %.2Lf TB\n", unit);
			}
			else if (data_size >= power3) {
				unit = (long double) data_size / power3;
				printf("Size in gigabytes: %.2Lf GB\n", unit);
			}
			else if (data_size >= power2) {
				unit = (long double) data_size / power2;
				printf("Size in megabytes: %.2Lf MB\n", unit);
			}
			else if (data_size >= power1) {
				unit = (long double) data_size / power1;
				printf("Size in kilobytes: %.2Lf KB\n", unit);
			}
		}
	}
	else if (options.other_unit == 1) {
		if (data_size != 0) {
			if (strcmp(options.unit,"TB") == 0) {
				unit = (long double) data_size / power4;
				printf("Size in terabytes: %.2Lf TB\n", unit);
			}
			else if (strcmp(options.unit,"GB") == 0) {
				unit = (long double) data_size / power3;
				printf("Size in gigabytes: %.2Lf GB\n", unit);
			}
			else if (strcmp(options.unit,"MB") == 0) {
				unit = (long double) data_size / power2;
				printf("Size in megabytes: %.2Lf MB\n", unit);
			}
			else if (strcmp(options.unit,"KB") == 0) {
				unit = (long double) data_size / power1;
				printf("Size in kilobytes: %.2Lf KB\n", unit);
			}
		}
	}
}

// free the data structures used to hold file lists to copy, delete, overwrite...
void destroy_data_structs(void) {
	if (data_copy_info.files_to_copy_list != NULL)
		dlist_destroy_2(data_copy_info.files_to_copy_list);
	if (data_copy_info.dirs_to_copy_list != NULL)
		dlist_destroy_2(data_copy_info.dirs_to_copy_list);
	if (data_copy_info.files_surplus_list != NULL)
		dlist_destroy_2(data_copy_info.files_surplus_list);
	if (data_copy_info.dirs_surplus_list != NULL)
		dlist_destroy_2(data_copy_info.dirs_surplus_list);
	if (data_copy_info.diff_size_ms_list != NULL)
		dlist_destroy_3(data_copy_info.diff_size_ms_list);
	if (data_copy_info.diff_size_ml_list != NULL)
		dlist_destroy_3(data_copy_info.diff_size_ml_list);
	if (data_copy_info.diff_time_mn_list != NULL)
		dlist_destroy_3(data_copy_info.diff_time_mn_list);
	if (data_copy_info.diff_time_mo_list != NULL)
		dlist_destroy_3(data_copy_info.diff_time_mo_list);
}
