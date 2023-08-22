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
	int ow_type_main;
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
	DList *diff_type_main;						// same name, different type. overwrite with main.
	DListElmt *file_list_element, *dir_list_element;		// used to loop through file and directory lists to display files and diretories to copy, etc...
	struct copied_or_not copied;					// determine if user enterted yes to copy data
	extern struct options_menu options;				// options for program
	extern struct errors_data error;				// errors information
	extern int full_dir_write;					// if 1, the destination is empty, so copy the complete source. if 2, the source is empty, so copy the complete destination if you wish
	int use_threads = 0;						// in case source and destination directories are on different disk, use threads is set to 1
	int open_linearly = 0;
	int len;
	char *pathname1, *pathname2;
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
	int ow_type_main;			// there are files with the name, but of different type. overwrite with the one in the main location 
	struct stat buf1, buf2; 		// used to test source and destination arguments for program, whether arguments are directories and devices they are located on.
	int copyfile;				// file descriptor for copy content file
	int c;					// for getopt_long
	int ind1 = 0;				// used for checking argv arguments
	int ind2 = 0;				// used for checking argv arguments
	int index;				// used for checking argv arguments

	char *string1 = "Files copied:\n";
	char *string2 = "Directories copied:\n";
	char *string3 = "Files overwritten (smaller)\n";
	char *string4 = "Files overwritten (larger)\n";
	char *string5 = "Surplus files copied back:\n";
	char *string6 = "Surplus directories copied back:\n";
	char *string7 = "Surplus files deleted:\n";
	char *string8 = "Surplus directories deleted:\n";

	char *help_string1 = "--copy-surplus-back or -a";
	char *help_string2 = "Copy the surplus data from the secondary (directory 2) location into the main location (directory 1) while synchronizing the directories.";
	char *help_string3 = "--delete-surplus or -b";
	char *help_string4 = "Delete the surplus data from the secondary (directory 2) location while synchronizing the with the directories from the main location (directory 1).";
	char *help_string43 = "--just-copy-surplus-back or -t";
	char *help_string44 = "Just copy the surplus data from the secondary (directory 2) location into the main location (directory 1), but don't synchronize directories.";
	char *help_string5 = "--overwrite-with-smaller or -c";
	char *help_string6 = "If two files with the same name are found, overwrite the larger file in the secondary location with the smaller from the main location.";
	char *help_string7 = "--overwrite-with-larger or -d";
	char *help_string8 = "If two files with the same name are found, overwrite the smaller file in the secondary location with the larger file from the main location.";
	char *help_string9 = "--overwrite-type or -e";
	char *help_string10 = "Overwrite the secondary location file type with the main location file type.";
	char *help_string11 = "--list-surplus or -f";
	char *help_string12 = "Just list surplus files and directories, but dont copy them.";
	char *help_string13 = "--dont-list-data-to-copy or -g";
	char *help_string14 = "Don't list the files and directories, their sizes and location, to copy.";
	char *help_string15 = "--help or -h";
	char *help_string16 = "Show help and options.";
	char *help_string17 = "--content-file=[FILE] or -i";
	char *help_string18 = "Write the file trees of both directories before copying.";
	char *help_string19 = "--just-content-file=[FILE] or -j";
	char *help_string20 = "Just write the file trees of both directories, but don't copy anything.";
	char *help_string21 = "--copy-content-file=[FILE] or -k";
	char *help_string22 = "Write files and directories to copy into the file.";
	char *help_string23 = "--just-copy-content-file=[FILE] or -l";
	char *help_string24 = "Just write files and directories to copy into the file, but don't copy anything.";
	char *help_string25 = "--dont-list-stats or -m";
	char *help_string26 = "Don't list statistics about file and directory size, number, etc.";
	char *help_string27 = "--dont-quit-read-errors or -n";
	char *help_string28 = "Don't quit on read errors. Useful for unexpected permissions on a directory.";
	char *help_string29 = "--dont-quit-write-errors or -o";
	char *help_string30 = "Don't quit on write errors. Useful for unexpected permissions on a directory.";
	char *help_string31 = "--dont-quit-delete-errors or -p";
	char *help_string32 = "Don't quit if deleting file or directory fails. Useful for unexpected permissions on a directory.";
	char *help_string33 = "--no-questions or -q";
	char *help_string34 = "Don't ask for confirmation to write the data.";
	char *help_string35 = "--unit=OPTION";
	char *help_string36 = "Show sizes in unit of choice (KB, MB, GB, TB, example: --unit=MB) insted of the default unit appropriate for the size.";
	char *help_string37 = "--si-units";
	char *help_string38 = "Use powers of 1000 instead of the default 1024";
	char *help_string39 = "--dont-show-read-process or -r";
	char *help_string40 = "Don't list files and directories currently reading.";
	char *help_string41 = "--dont-show-write-process or -w";
	char *help_string42 = "Don't list files and directories currently writing.";

	// 0 option is inactive, 1 option is active
	options.quit_read_errors = 1;		// on by default
	options.quit_write_errors = 1;		// on by default
	options.quit_delete_errors = 1;		// on by default
	options.copy_surplus_back = 0;
	options.just_copy_surplus_back = 0;
	options.delete_surplus = 0;
	options.ow_main_smaller = 0;
	options.ow_main_larger = 0;
	options.ow_type_main = 0;
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
	
	copied.copied_data = 0;		// if 1, add size in stats
	copied.aborted_copying;		// if 1, user aborted copying missing files and dirs
	copied.copied_surplus = 0;	// if 1, add size in stats
	copied.deleted_surplus = 0;	// if 1, subtract size in stats
	copied.ow_smaller = 0;
	copied.ow_larger = 0;
	copied.ow_type_main = 0;
	copied.full_dir1_copied = 0;	// if 1, add size in stats
	copied.full_dir2_copied = 0;	// if 1, add size in stats

	data_copy_info.files_to_copy_list = NULL;
	data_copy_info.dirs_to_copy_list = NULL;
	data_copy_info.files_surplus_list = NULL;
	data_copy_info.dirs_surplus_list = NULL;
	data_copy_info.diff_size_ms_list = NULL;
	data_copy_info.diff_size_ml_list = NULL;
	data_copy_info.diff_type_list_main = NULL;
	data_copy_info.diff_type_list_secondary = NULL;
	data_copy_info.global_files_to_copy_num = 0;
	data_copy_info.global_files_to_copy_size = 0;
	data_copy_info.global_dirs_to_copy_num = 0;
	data_copy_info.global_dirs_to_copy_size = 0;
	data_copy_info.global_files_surplus_num = 0;
	data_copy_info.global_files_surplus_size = 0;
	data_copy_info.global_dirs_surplus_num = 0;
	data_copy_info.global_dirs_surplus_size = 0;
	data_copy_info.global_diff_type_num_main = 0;
	data_copy_info.global_diff_type_num_secondary = 0;
	data_copy_info.global_diff_type_size_main = 0;
	data_copy_info.global_diff_type_size_secondary = 0;
	data_copy_info.global_diff_size_ms_num = 0;
	data_copy_info.global_diff_size_ms_size = 0;
	data_copy_info.global_diff_size_ms_orig_size = 0;
	data_copy_info.global_diff_size_ml_num = 0;
	data_copy_info.global_diff_size_ml_size = 0;
	data_copy_info.global_diff_size_ml_orig_size = 0;
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

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"copy-surplus-back", no_argument, 0, 'a' },
			{"delete-surplus", no_argument, 0, 'b' },
			{"just-copy-surplus-back", no_argument, 0, 't' },
			{"overwrite-with-smaller", no_argument, 0, 'c' },
			{"overwrite-with-larger", no_argument, 0, 'd' },
			{"overwrite-type", no_argument, 0, 'e' },
			{"just-list-surplus", no_argument, 0, 'f' },
			{"dont-list-data-to-copy", no_argument, 0, 'g' },
			{"help", no_argument, 0, 'h' },
			{"write-content-file", required_argument, 0, 'i' },
			{"just-content-file", required_argument, 0, 'j' },
			{"copy-content-file", required_argument, 0, 'k' },
			{"just-copy-content-file", required_argument, 0, 'l' },
			{"dont-list-stats", no_argument, 0, 'm' },
			{"dont-quit-read-errors", no_argument, 0, 'n' },
			{"dont-quit-write-errors", no_argument, 0, 'o' },
			{"dont-quit-delete-errors", no_argument, 0, 'p' },
			{"no-questions", no_argument, 0, 'q' },
			{"unit", required_argument, 0, 0 },
			{"si-units", no_argument, &options.si_units, 1 },
			{"dont-show-read-process", no_argument, 0, 'r' },
			{"dont-show-write-process", no_argument, 0, 'w' },
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "abcdefghi:j:k:l:mnopqrwt", long_options, &option_index);
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
			case 'a':
				options.copy_surplus_back = 1;
				break;
			case 'b':
				options.delete_surplus = 1;
				break;
			case 'c':
				options.ow_main_smaller = 1;
				break;
			case 'd':
				options.ow_main_larger = 1;
				break;
			case 'e':
				options.ow_type_main = 1;
				break;
			case 'f':
				options.list_surplus = 1;
				break;
			case 'g':
				options.dont_list_data_to_copy = 1;
				break;
			case 'h':
				options.help = 1;
				break;
			case 'i':
				options.write_content_file = 1;
				strcpy(file_loc1,optarg);
				break;
			case 'j':
				options.just_write_content_file = 1;
				strcpy(file_loc1,optarg);
				break;
			case 'k':
				options.write_copy_content_file = 1;
				strcpy(file_loc2,optarg);
				break;
			case 'l':
				options.just_write_copy_content_file = 1;
				strcpy(file_loc2,optarg);
				break;
			case 'm':
				options.dont_list_stats = 1;
				break;
			case 'n':
				options.quit_read_errors = 0;	// on by default.
				break;
			case 'o':
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
			case 't':
				options.just_copy_surplus_back = 1;
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
		printf("\n");
		exit(1);
	}

	if (options.copy_surplus_back == 1 && options.delete_surplus == 0)
		options.list_surplus = 1;
	if (options.copy_surplus_back == 0 && options.delete_surplus == 1)
		options.delete_surplus = 1;
	// or maybe allow this, one after the other?
	if (options.copy_surplus_back == 1 && options.delete_surplus == 1) {
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
	ow_type_main = 0;

	// optind is the first non-option argument, so it should be a pathname for the first directory. 
	// then increment it to point to a second directory if there are more arguments, or exit with error if there aren't
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
		perror("realpath");
		printf("Exiting.\n");
		exit(1);
	}
	errno = 0;
	if (lstat(pathname1, &buf1) < 0) {
		if (errno == EFAULT)
			printf("Specified directory1 does not exist. Exiting.\n");
		else
			perror("stat");
		exit(1);
	}
	// test whether pathname arguments are directories
	if (!S_ISDIR(buf1.st_mode)) {
		printf("Pathname 1 is not a directory. Exiting.\n");
		exit(1);
	}
	major1 = major(buf1.st_dev);
	minor1 = minor(buf1.st_dev);

	errno = 0;
	pathname2 = realpath(argv[ind2],NULL);
	if (pathname2 == NULL) {
		perror("realpath");
		printf("Exiting.\n");
		exit(1);
	}
	errno = 0;
	if (lstat(pathname2, &buf2) < 0) {
		if (errno == EFAULT)
			printf("Specified directory2 does not exist. Exiting.\n");
		else
			perror("stat");
		exit(1);
	}
	// test whether pathname arguments are directories
	if (!S_ISDIR(buf2.st_mode)) {
		printf("Pathname 2 is not a directory. Exiting.\n");
		exit(1);
	}

	// test whether directories are on the same or different disks
	major2 = major(buf2.st_dev);
	minor2 = minor(buf2.st_dev);

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
		if (th1_status != 0)
			exit(1);
		th2_status = pthread_create(&th2,NULL,(void *)open_dirs,(void *)thread_data_b);
		if (th2_status != 0)
			exit(1);

		th1_status = pthread_join(th1,(void *)&th1_retval);
		if (th1_status != 0) {
			exit(1);
		}
		th2_status = pthread_join(th2,(void *)&th2_retval);
		if (th2_status != 0)
			exit(1);
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
		if (th3_status != 0)
			exit(1);
		th4_status = pthread_create(&th4,NULL,(void *)build_tree,(void *)thread_data_b);
		if (th4_status != 0)
			exit(1);
		th3_status = pthread_join(th3,NULL);
		if (th3_status != 0)
			exit(1);
		th4_status = pthread_join(th4,NULL);
		if (th4_status != 0)
			exit(1);
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
			return 0;
		}
	}

	// prepare/list all the data to copy, overwrite or delete
	file_list = data_copy_info.files_to_copy_list;
	if (file_list != NULL) {
		if (file_list->num != 0) {
			copy_files = 1;
			if (options.dont_list_data_to_copy != 1) {
				printf("Files to copy:\n\n");
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
				printf("Directories to copy:\n\n");
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
				printf("Surplus files:\n\n");
				if (options.copy_surplus_back == 1) {
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
				printf("Surplus directories\n\n");
				for (dir_list_element = dir_surp_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next)
					printf("directory: %s\n location: %s\n new location: %s\n size: %ld\n\n\n", dir_list_element->name, dir_list_element->dir_location, 
					dir_list_element->new_location, dir_list_element->size);
			}
		}
	}
	file_ms_list = data_copy_info.diff_size_ms_list;
	if (file_ms_list != NULL) {
		if (file_ms_list->num != 0) {
			if (options.ow_main_smaller == 1)
				ow_main_smaller = 1; // overwrite main smaller
			if (options.dont_list_data_to_copy != 1) {
				printf("Files to overwrite. (source location files smaller than the destination)\n\n");
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
			if (options.dont_list_data_to_copy != 1) {
				printf("Files to overwrite. (source location files larger than the destination)\n\n");
				for (file_list_element = file_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
					printf("file: %s\n location: %s\n new location: %s\n size: %ld\n\n\n", file_list_element->name, file_list_element->dir_location, 
					file_list_element->new_location, file_list_element->size);
				}
			}
		}
	}
	diff_type_main = data_copy_info.diff_type_list_main;
	if (diff_type_main != NULL) {
		if (diff_type_main->num != 0) {
			if (options.ow_type_main == 1)
				ow_type_main = 1; // overwrite the location secondary file with the main location file
			if (options.dont_list_data_to_copy != 1) {
				printf("Files to overwrite. (source location type will overwrite the destination file type)\n\n");
				for (file_list_element = diff_type_main->head; file_list_element != NULL; file_list_element = file_list_element->next) {
					printf("file: %s\n location %s\n new location: %s\n size: %ld\n\n\n", file_list_element->name, file_list_element->dir_location,
					file_list_element->new_location, file_list_element->size);
				}
			}
		}
	}

	// if there is some data, depending on options: copy, overwrite, delete...
	if (copy_files == 1 || copy_dirs == 1 || files_surplus == 1 || dirs_surplus == 1 ||  ow_main_smaller == 1 || ow_main_larger == 1 || ow_type_main == 1) {
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
								printf("Files written succesfully.\n");
							else
								printf("Error writing the files.\n");
						}
						if (copy_dirs == 1) {
							if (read_write_data(data_copy_info.dirs_to_copy_list,2,NULL,NULL) == 0)
								printf("Directories written succesfully.\n");
							else
								printf("Error writing the directories.\n");
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
			if (ow_type_main == 1 && options.just_copy_surplus_back != 1) {
				printf("Scan found two files with the same name, but different type. Should the file in the main location overwrite "); 
				printf("the file in the secondary location? Answer yes or no.\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						read_write_data(data_copy_info.diff_type_list_main,4,NULL,NULL);
						copied.ow_type_main = 1;
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
				/*printf("Scan found two files with the same name, but different type. Should the file in the secondary location overwrite 
				the file in the main location? Answer yes or no.\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						read_write_data(data_copy_info->diff_type_list_secondary,4,NULL,NULL);
						break;
					}
					else if (strcmp(line,"no") == 0) {
						break;
					}
					else
						printf("Unrecognized answer. Type yes or no.\n");
			 	}*/
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
			if (options.ow_type_main == 1 && options.just_copy_surplus_back != 1)
				read_write_data(data_copy_info.diff_type_list_main,4,NULL,NULL);
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

	// free the data structures used to hold file lists to copy, delete, overwrite...
	if (data_copy_info.files_to_copy_list != NULL)
		dlist_destroy(data_copy_info.files_to_copy_list);
	if (data_copy_info.dirs_to_copy_list != NULL)
		dlist_destroy(data_copy_info.dirs_to_copy_list);
	if (data_copy_info.files_surplus_list != NULL)
		dlist_destroy(data_copy_info.files_surplus_list);
	if (data_copy_info.dirs_surplus_list != NULL)
		dlist_destroy(data_copy_info.dirs_surplus_list);
	if (data_copy_info.diff_size_ms_list != NULL)
		dlist_destroy(data_copy_info.diff_size_ms_list);
	if (data_copy_info.diff_size_ml_list != NULL)
		dlist_destroy(data_copy_info.diff_size_ml_list);
	if (data_copy_info.diff_type_list_main != NULL)
		dlist_destroy(data_copy_info.diff_type_list_main);
	if (data_copy_info.diff_type_list_secondary != NULL)
		dlist_destroy(data_copy_info.diff_type_list_secondary);

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

	// dodat opciju da ovo sve napise u fajl? samo redirect stdou u fajl unutar funkcije?
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
		printf("Files and directories to copy, ");
		calc_size(size_to_copy,options.other_unit);
		printf("Number of surplus files: %ld\n", data_copy_info.global_files_surplus_num);
		printf("Size of surplus files in bytes: %ld\n", data_copy_info.global_files_surplus_size);
		calc_size(data_copy_info.global_files_surplus_size,options.other_unit);
		printf("Number of surplus directories: %ld\n", data_copy_info.global_dirs_surplus_num);
		printf("Size of surplus directories in bytes: %ld\n", data_copy_info.global_dirs_surplus_size);
		calc_size(data_copy_info.global_dirs_surplus_size,options.other_unit);
		printf("Same files with different size (main location smaller): %ld\n", data_copy_info.global_diff_size_ms_num);
		printf("Same files with different size (main location larger): %ld\n", data_copy_info.global_diff_size_ml_num);
		printf("Files with the same name, different type (main): %ld\n", data_copy_info.global_diff_type_num_main);
		printf("Files with the same name, different type (secondary): %ld\n", data_copy_info.global_diff_type_num_secondary);
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
