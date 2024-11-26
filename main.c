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
#define SCREEN 101		// output from list_stats() and calc_size() to stdout
#define TO_FILE 102		// output from list_stats() and calc_size() to file
#define PRINT_BOTH 103		// output from list_status() and calc_size() to both stdout and file
#define NORMAL 104
#define AS_RETURN_VAL 105
#define BOTH 106
#define BEFORE 107
#define AFTER 108
#define REGULAR 109
#define FULL_DIR 110

int open_dirs(struct thread_struct *thread_data);	// function to open and read directories
void build_tree(struct thread_struct *thread_data);	// build (file) tree
int compare_trees(struct thread_struct *thread_data_a, struct thread_struct *thread_data_b); 		// compare file trees a and b
int read_write_data(DList *, int choose, char *source, char *destination);		// read and write files and directories that should be copied, or delete them if specified
int clean_tree(DList_of_lists *, short);								// free the dynamically allocated file tree
int write_contents_to_file(DList_of_lists *directory, short opened, int f_descriptor);			// write the file trees to a file
char *calc_stats(int type);
char *calc_size(unsigned long data_size, int other_unit, int output, int fd);
char *detailed_output(DList *to_copy_list, int output, char *what_is_copied, int fd);
void print_results(int when, int where, int fd);
void destroy_data_structs(void);
void clean_up_exit(struct thread_struct *, struct thread_struct *);
void show_help();

int full_dir_write = 0;		// if set to 1, copy the complete source directory, if set to 2, copy the complete destination directory
char file_loc1[PATH_MAX];	// file tree content file location
char *pathname1, *pathname2;	

struct options_menu options;				// data structure used to set options
struct Data_Copy_Info data_copy_info;			// data structure with statistical info, holds lists of files and dirs to copy...
struct copied_or_not copied;				// info about what was copied
struct errors_data errors;				// errors info

int main(int argc, char *argv[])
{
	dev_t major1, major2, minor1, minor2;		// used to determine whether the two locations to compare are on the same disk, or on a separate disk.
	pthread_t th1, th2, th3, th4;			// threads used to read directories with build_tree() function concurently if two locations to compare are on separate disks.
	int th1_status, th2_status;					// if concurent threads access is used, these are return values for the threads
	int th3_status, th4_status;					// if concurent threads access is used, these are return values for the threads
	void *th1_retval, *th2_retval;
	void *th3_retval, *th4_retval;

	struct thread_struct *thread_data_a, *thread_data_b;		// main data structure for reading directories a (source) and b (destination). 
									// despite it's name, it is used even if threads are not used.

	DList *file_list, *dir_list, *file_surp_list, *dir_surp_list;	// file and directory lists, file and directory extraneous lists
	DList *symlinks_list, *symlinks_surp_list;
	DList *file_ms_list, *file_ml_list;				// file main smaller and main larger lists
	DList *file_mn_list, *file_mo_list;				// file main newer and main smaller lists
	DList *symlinks_ms_list, *symlinks_ml_list;
	DList *symlinks_mn_list, *symlinks_mo_list;
	DListElmt *file_list_element, *dir_list_element;		// used to loop through file and directory lists to display files and diretories to copy, etc...

	int use_threads = 0;						// in case source and destination directories are on different disk, use threads is set to 1
	int open_linearly = 0;
	int len;
	char file_loc2[PATH_MAX];					// location of the text file with the data to copy content file location
	char file_location[PATH_MAX];					// copy/content file location + the newline char to avoid using write() sys call just for '\n'
	const char *src = "source";
	const char *dst = "destination";
	char line[BUF];				// get yes or no answer from the user
	int length;

	int copy_files = 0;			// there are files to copy if 1
	int copy_symlinks = 0;			// there are symbolic links to copy if 1
	int copy_dirs = 0;			// there are directories to copy if 1
	int files_extraneous = 0;		// there are extraneous files to copy or delete if 1
	int symlinks_extraneous = 0;		// there are extraneous symlinks if 1
	int dirs_extraneous = 0;		// there are extraneous directories to copy or delete if 1
	int ow_main_smaller = 0;		// there are files with the same name, smaller in the main location to overwrite if 1
	int ow_main_larger = 0;			// there are files with the same name, larger in the main location to overwrite if 1
	int ow_main_newer = 0;			// there are files with the same name, newer in the main location to overwrite if 1
	int ow_main_older = 0;			// there are files with the same name, older in the main location to overwrite if 1
	int ow_symlinks_main_smaller = 0;	// there are symbolic links with the same name, smaller in the main location to overwrite if 1
	int ow_symlinks_main_larger = 0;	// there are symbolic links with the same name, larger in the main location to overwrite if 1
	int ow_symlinks_main_newer = 0;		// there are symbolic links with the same name, newer in the main location to overwrite if 1
	int ow_symlinks_main_older = 0;		// there are symbolic links with the same name, older in the main location to overwrite if 1

	struct stat buf1, buf2; 		// used to test source and destination arguments for program, whether arguments are directories and devices they are located on.
	int copyfile;				// file descriptor for copy content file
	int c;					// for getopt_long
	int ind1 = 0;				// used for checking argv arguments
	int ind2 = 0;				// used for checking argv arguments
	int index;				// used for checking argv arguments
	int nice_val;
	mode_t umask_val;
	int read_write_data_res;		// read_write_data() return result

	int i, ignore_len = 0, finish = 0, word_len = 0, words = 0;
	char *beginning, *mark_word, *ignore_name;

	char *version = "1.4";		// cps version number

	char *string1 = "Files to copy:\n";
	char *string1_1 = "Symbolic links to copy:\n";
	char *string2 = "Directories to copy:\n";
	char *string3 = "Files to overwrite (smaller)\n";
	char *string4 = "Files to overwrite (larger)\n";
	char *string5 = "Extraneous files to copy back:\n";
	char *string5_1 = "Extraneous symbolic links to copy back:\n";
	char *string6 = "Extraneous directories to copy back:\n";
	char *string7 = "Extraneous files to delete:\n";
	char *string7_1 = "Extraneous symbolic links to delete:\n";
	char *string8 = "Extraneous directories to delete:\n";
	char *string9 = "Files to overwrite (newer)\n";
	char *string10 = "Files to overwrite (older)\n";
	char *string11 = "Symbolic links to overwrite (smaller)\n";
	char *string12 = "Symbolic links to overwrite (larger)\n";
	char *string13 = "Symbolic links to overwrite (newer)\n";
	char *string14 = "Symbolic links to overwrite (older)\n";
	char *string15 = "Extraneous files:\n";
	char *string16 = "Extraneous symbolic links:\n";
	char *string17 = "Extraneous directories:\n";

	// 0 if option is inactive, 1 if option is active by default
	options.quit_read_errors = 1;		// on by default
	options.quit_write_errors = 1;		// on by default
	options.quit_delete_errors = 1;		// on by default
	options.copy_extraneous_back = 0;
	options.just_copy_extraneous_back = 0;
	options.delete_extraneous = 0;
	options.just_delete_extraneous = 0;
	options.ow_main_smaller = 0;
	options.ow_main_larger = 0;
	options.follow_sym_links = 0;
	options.list_extraneous = 0;
	options.dont_list_data_to_copy = 0;
	options.show_help = 0;
	options.show_version = 0;
	options.copy_content_file = 0;
	options.just_copy_content_file = 0;
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
	options.time_based = 1;			// on by default
	options.size_based = 0;	
	options.ow_main_newer = 0;
	options.ow_main_older = 0;
	options.list_conflicting = 0;
	options.less_detailed = 0;
	options.preserve_perms = 0;
	options.acls = 0;
	options.xattrs = 0;
	options.ignore_symlinks = 0;
	options.ignore = 0;
	options.ignore_list = NULL;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"copy-extraneous", no_argument, 0, 'b' },
			{"delete-extraneous", no_argument, 0, 'x' },
			{"just-copy-extraneous", no_argument, 0, 'B' },
			{"just-delete-extraneous", no_argument, 0, 'X' },
			{"overwrite-with-smaller", no_argument, 0, 's' },
			{"overwrite-with-larger", no_argument, 0, 'l' },
			{"list-extraneous", no_argument, 0, 'e' },
			{"dont-list-data-to-copy", no_argument, 0, 'g' },
			{"less-detailed", no_argument, 0, 'D' },
			{"help", no_argument, 0, 'h' },
			{"version", no_argument, 0, 'v' },
			{"copy-content-file", required_argument, 0, 'k' },
			{"just-copy-content-file", required_argument, 0, 'K' },
			{"dont-list-stats", no_argument, 0, 'G' },
			{"dont-quit-read-errors", no_argument, 0, 'y' },
			{"dont-quit-write-errors", no_argument, 0, 'z' },
			{"dont-quit-delete-errors", no_argument, 0, 'p' },
			{"no-questions", no_argument, 0, 'q' },
			{"unit", required_argument, 0, 0 },
			{"si-units", no_argument, &options.si_units, 1 },
			{"dont-show-read-process", no_argument, 0, 'r' },
			{"dont-show-write-process", no_argument, 0, 'w' },
			{"follow-sym-links", no_argument, 0, 'F' },
			{"no-access-time", no_argument, 0, 'a' },
			{"preserve-atime", no_argument, 0, 'A' },
			{"preserve-mtime", no_argument, 0, 'M' },
			{"overwrite-with-newer", no_argument, 0, 'N' },
			{"overwrite-with-older", no_argument, 0, 'O' },
			{"size-mode", no_argument, 0, 'S'}, 
			{"list-conflicting", no_argument, 0, 'L'},
			{"preserve-perms", no_argument, 0, 'P'},
			{"acls", no_argument, &options.acls, 1 },
			{"xattrs", no_argument, &options.xattrs, 1 },
			{"ignore-symlinks", no_argument, &options.ignore_symlinks, 'i'},
			{"ignore", required_argument, 0, 'I' },
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "abdeghik:lopqrstuvwxyzABDFGI:K:LMNOPSX", long_options, &option_index);
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
				options.copy_extraneous_back = 1;
				break;
			case 'B':
				options.just_copy_extraneous_back = 1;
				break;
			case 'x':
				options.delete_extraneous = 1;
				break;
			case 'X':
				options.just_delete_extraneous = 1;
				break;
			case 's':
				options.ow_main_smaller = 1;
				break;
			case 'l':
				options.ow_main_larger = 1;
				break;
			case 'e':
				options.list_extraneous = 1;
				break;
			case 'g':
				options.dont_list_data_to_copy = 1;
				break;
			case 'h':
				options.show_help = 1;
				break;
			case 'I':
				options.ignore = 1;
				ignore_len = strlen(optarg);
				if (ignore_len == 0) {
					printf("error: ignore_len == 0.\n");
					exit(1);
				}
				else if (ignore_len > 0) {
					words = 1;
					options.ignore_list = malloc(sizeof(DList));
					if (options.ignore_list != NULL)
						dlist_init(options.ignore_list);	// dodaj free u glavnu funkciju za oslobađanje
					else {
						printf("Error allocating memory for ignore_list. Exiting.\n");
						exit(1);
					}
				}
				for (i = 0; i < ignore_len; i++) {
					if (optarg[i] == ',')
						words++;
				}
				if (words == 1) {
					beginning = &optarg[0];
					word_len = strlen(beginning);
					ignore_name = malloc(word_len+1);
					if (ignore_name == NULL) {
						printf("Error 1 allocating memory for ignore option. Exiting.\n");
						exit(1);
					}
					strcpy(ignore_name,beginning);
					dlist_ins_next(options.ignore_list,options.ignore_list->tail,ignore_name,0,0,NULL,0,NULL,0,0,NULL);
				}
				else if (words > 0) {
					beginning = &optarg[0];
					finish = 0;
					while (finish != 1) {	// probaj testirat sa dva zareza sta se desi -i=test1,test2,,test3
						mark_word = strchr(beginning,',');
						if (mark_word == NULL) {
							if (*beginning != '\0') {
								word_len = strlen(beginning);
								ignore_name = malloc(word_len+1);
								if (ignore_name == NULL) {
									printf("Error 2 allocating memory for ignore option. Exiting.\n");
									exit(1);
								}
								strcpy(ignore_name,beginning);
								dlist_ins_next(options.ignore_list,options.ignore_list->tail,ignore_name,0,0,NULL,0,NULL,0,0,NULL);
								finish = 1;
							}
							else
								finish = 1;
						}
						else if (*mark_word == ',') {
							*mark_word = '\0';
							word_len = strlen(beginning);
							ignore_name = malloc(word_len+1);
							if (ignore_name == NULL) {
								printf("Error 1 allocating memory for ignore option. Exiting.\n");
								exit(1);
							}
							strcpy(ignore_name,beginning);
							dlist_ins_next(options.ignore_list,options.ignore_list->tail,ignore_name,0,0,NULL,0,NULL,0,0,NULL);
							*mark_word++;
							if (*mark_word != '\0')
								beginning = mark_word;
							else {
								finish = 1;
							}
						}
					}
				}
				break;
			case 'k':
				options.copy_content_file = 1;
				strcpy(file_loc2,optarg);
				break;
			case 'K':
				options.just_copy_content_file = 1;
				strcpy(file_loc2,optarg);
				break;
			case 'G':
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
			case 'F':
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
			case 'S':
				options.size_based = 1;
				break;
			case 'L':
				options.list_conflicting = 1;
				break;
			case 'D':
				options.less_detailed = 1;
				break;
			case 'P':
				options.preserve_perms = 1;
				umask(0);
				break;
			case 'i':
				options.ignore_symlinks = 1;
				break;
			case 'v':
				options.show_version = 1;
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

	if (argc < 2) {
		printf("USAGE: cps directory1 directory2 or cps OPTIONS directory1 directory2\n");
		printf("For help type man cps or cps -h\n");
		exit(0);
	}
	if (options.show_version == 1 && options.show_help == 0) {
		printf("cps version: %s\n", version);
		exit(0);
	}
	else if (options.show_help == 1 && options.show_version == 0) {
		show_help();
		exit(0);
	}
	else if (options.show_help == 1 && options.show_version == 1) {
		printf("cps version: %s\n", version);
		show_help();
		exit(0);
	}

	if (options.ow_main_smaller == 1 && options.ow_main_larger == 1) {
		printf("Error: two contradicting options: --overwrite-with-smaller and --overwrite-with-larger. Specify either one or the other.\n");
		exit(1);
	}

	if (options.size_based == 1) {
		options.time_based = 0;
		if (options.ow_main_newer == 1 && options.ow_main_older == 1) {
			printf("Error: Conflicting options. Both overwrite newer and overwrite older files options enabled. Exiting.\n");
			exit(1);
		}
		if (options.ow_main_smaller == 1 || options.ow_main_larger == 1) {
			printf("Error: Conflicting options: --overwrite-with-smaller and --overwrite-with-larger cannot be used with time based options.\n");
			exit(1);
		}
	}
	if (options.acls == 1)
		umask(0);

	if (options.follow_sym_links == 0) {
		options.open_flags |= O_NOFOLLOW;
		options.stat_f = lstat;
		if (options.xattrs == 1) {
			options.setxattr_func = lsetxattr;
			options.getxattr_func = lgetxattr;
			options.listxattr_func = llistxattr;
		}
	}

	else if (options.follow_sym_links == 1) {
		options.stat_f = stat;
		if (options.xattrs == 1) {
			options.setxattr_func = setxattr;
			options.getxattr_func = getxattr;
			options.listxattr_func = listxattr;
		}
	}
	else if (options.follow_sym_links == 1 && options.ignore_symlinks == 1) {
		printf("Error: Conflicting options: follow symbolic links and ignore symbolic links. Exiting.\n");
		exit(1);
	}

	if ((options.copy_extraneous_back == 1 || options.just_copy_extraneous_back == 1) && (options.delete_extraneous == 1 || options.just_delete_extraneous == 1)) {
		printf("Error: Conflicting options. You are attempting to delete the extraneous data and to copy it back to the main directory.\n");
		exit(1);
	}

	if (options.copy_extraneous_back == 1 || options.just_copy_extraneous_back == 1)
		if (options.dont_list_data_to_copy != 1)
			options.list_extraneous = 1;
	if (options.delete_extraneous == 1 || options.just_delete_extraneous == 1)
		if (options.dont_list_data_to_copy != 1)
			options.list_extraneous = 1;

	if (options.ow_main_newer == 1 && options.ow_main_older == 1) {
		printf("Error: Conflicting options: --overwrite-with-newer or -N and --overwrite-with-older or -O. Specify either one or the other.\n");
		exit(1);
	}
	if (options.ow_main_smaller == 1 && options.ow_main_larger == 1) {
		printf("Error: Conflicting options: --overwrite-with-smaller or -s and --overwrite-with-larger or -l. Specify either one or the other.\n");
		exit(1);
	}
	if ((options.copy_content_file == 1 && options.just_copy_content_file == 0) || (options.copy_content_file == 0 && options.just_copy_content_file == 1)) {
		errno = 0;
		copyfile = open(file_loc2, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
		if (copyfile == -1) {
			perror("open");
			fprintf(stderr, "Error opening the copy content file: %s\n",file_loc2);
			exit(1);
		}
	}
	else if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
		printf("Error: Conflicting options write copy content file and write only copy content file. Specify either one or the other.\n");
		exit(1);
	}

	// optind is the first non-option argument, so it should be a pathname for the first directory. 
	// then increment it to point to a second directory if there are more arguments, or exit with an error if there aren't any
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
	/*if (options.copy_content_file == 1) {
		write_contents_to_file(thread_data_a->file_tree_top_dir,0,0);
		write_contents_to_file(thread_data_b->file_tree_top_dir,0,0);
	}
	else if (options.just_copy_content_file == 1) {
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
	}*/

	// copy entire source directory to the destination because it's empty
	if (full_dir_write == 1) {
		if (options.no_questions == 0) {
			if (options.dont_list_stats != 1) {
				if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
					print_results(BEFORE,SCREEN,copyfile);
				else if (options.copy_content_file == 1)
					print_results(BEFORE,PRINT_BOTH,copyfile);
				else if (options.just_copy_content_file == 1)
					print_results(BEFORE,TO_FILE,copyfile);
			}
			printf("\nDestination directory is empty, entire source directory will be copied. Do you want to write the data? Type yes or no ...\n");
			while (fgets(line,BUF,stdin) != NULL) {
				length = strlen(line);
				line[length-1] = '\0';
				if (strcmp(line,"yes") == 0) {
					read_write_data_res = read_write_data(NULL,3,pathname1,pathname2);
					if (read_write_data_res == 0) {
						printf("\nFull directory copied successfully.\n");
						copied.full_dir1_copied = 1;
						calc_stats(FULL_DIR);
						if (options.dont_list_stats != 1) {
							if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
								print_results(AFTER,SCREEN,copyfile);
							else if (options.copy_content_file == 1)
								print_results(AFTER,PRINT_BOTH,copyfile);
							else if (options.just_copy_content_file == 1)
								print_results(AFTER,TO_FILE,copyfile);
						}
					}
					else if (read_write_data_res == 1) 
						printf("\nFull directory copied partially with some errors.\n");
					else if (read_write_data_res == -1) {
						printf("\nError copying full directory. Exiting.\n");
					}	
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
			if (options.dont_list_stats != 1) {
				if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
					print_results(BEFORE,SCREEN,copyfile);
				else if (options.copy_content_file == 1)
					print_results(BEFORE,PRINT_BOTH,copyfile);
				else if (options.just_copy_content_file == 1)
					print_results(BEFORE,TO_FILE,copyfile);
			}
			read_write_data_res = read_write_data(NULL,3,pathname1,pathname2);
			if (read_write_data_res == 0) {
				printf("\nFull directory copied successfully.\n");
				copied.full_dir1_copied = 1;
				calc_stats(FULL_DIR);
				if (options.dont_list_stats != 1)
					if (options.dont_list_stats != 1) {
						if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
							print_results(AFTER,SCREEN,copyfile);
						else if (options.copy_content_file == 1)
							print_results(AFTER,PRINT_BOTH,copyfile);
						else if (options.just_copy_content_file == 1)
							print_results(AFTER,TO_FILE,copyfile);
					}
			}
			else if (read_write_data_res == 1) 
				printf("\nFull directory copied partially with some errors.\n");
			else if (read_write_data_res == -1) {
				printf("\nError copying full directory. Exiting.\n");
			}	

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
			if (options.dont_list_stats != 1) {
				if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
					print_results(BEFORE,SCREEN,copyfile);
				else if (options.copy_content_file == 1)
					print_results(BEFORE,PRINT_BOTH,copyfile);
				else if (options.just_copy_content_file == 1)
					print_results(BEFORE,TO_FILE,copyfile);
			}
			printf("Source directory is empty, entire destination directory will be copied. Do you want to write the data? Type yes or no ...\n");
			while (fgets(line,BUF,stdin) != NULL) {
				length = strlen(line);
				line[length-1] = '\0';
				if (strcmp(line,"yes") == 0) {
					read_write_data_res = read_write_data(NULL,3,pathname2,pathname1);
					if (read_write_data_res == 0) {
						printf("\nFull directory copied successfully.\n");
						copied.full_dir2_copied = 1;
						calc_stats(FULL_DIR);
						if (options.dont_list_stats != 1) {
							if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
								print_results(AFTER,SCREEN,copyfile);
							else if (options.copy_content_file == 1)
								print_results(AFTER,PRINT_BOTH,copyfile);
							else if (options.just_copy_content_file == 1)
								print_results(AFTER,TO_FILE,copyfile);
						}
					}
					else if (read_write_data_res == 1) 
						printf("\nFull directory copied partially with some errors.\n");
					else if (read_write_data_res == -1) {
						printf("\nError copying full directory. Exiting.\n");
					}	

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
			if (options.dont_list_stats != 1) {
				if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
					print_results(BEFORE,SCREEN,copyfile);
				else if (options.copy_content_file == 1)
					print_results(BEFORE,PRINT_BOTH,copyfile);
				else if (options.just_copy_content_file == 1)
					print_results(BEFORE,TO_FILE,copyfile);
			}
			read_write_data_res = read_write_data(NULL,3,pathname2,pathname1);
			if (read_write_data_res == 0) {
				printf("\nFull directory copied successfully.\n");
				copied.full_dir2_copied = 1;
				calc_stats(FULL_DIR);
				if (options.dont_list_stats != 1)
					if (options.dont_list_stats != 1) {
						if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
							print_results(AFTER,SCREEN,copyfile);
						else if (options.copy_content_file == 1)
							print_results(AFTER,PRINT_BOTH,copyfile);
						else if (options.just_copy_content_file == 1)
							print_results(AFTER,TO_FILE,copyfile);
					}
			}
			else if (read_write_data_res == 1) 
				printf("\nFull directory copied partially with some errors.\n");
			else if (read_write_data_res == -1) {
				printf("\nError copying full directory. Exiting.\n");
			}	

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
	file_list = data_copy_info.files_to_copy_list;
	if (file_list != NULL) {
		if (file_list->num != 0) {
			copy_files = 1;
			if (options.dont_list_data_to_copy != 1) {
				if (options.less_detailed != 1)
					detailed_output(file_list,SCREEN,string1,0);
				if (options.less_detailed == 1) {
					printf("\nFiles to copy:\n\n");
					for (file_list_element = file_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
						printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
				}
			}
		}
	}
	if (options.ignore_symlinks != 1) {
		symlinks_list = data_copy_info.symlinks_to_copy_list;
		if (symlinks_list != NULL) {
			if (symlinks_list->num != 0) {
				copy_symlinks = 1;
				if (options.dont_list_data_to_copy != 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_list,SCREEN,string1_1,0);
					else if (options.less_detailed == 1) {
						printf("\nSymbolic links to copy:\n\n");
						for (file_list_element = file_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
				}
			}
		}
	}
	dir_list = data_copy_info.dirs_to_copy_list;
	if (dir_list != NULL) {
		if (dir_list->num != 0) {
			copy_dirs = 1;
			if (options.dont_list_data_to_copy != 1) {
				if (options.less_detailed != 1)
					detailed_output(dir_list,SCREEN,string2,0);
				else if (options.less_detailed == 1) {
					printf("\nDirectories to copy:\n\n");
					for (dir_list_element = dir_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next)
						printf("directory: %s\n location: %s\n", dir_list_element->name, dir_list_element->dir_location);
				}
			}
		}
	}
	file_surp_list = data_copy_info.files_extraneous_list;
	if (file_surp_list != NULL) {
		if (file_surp_list->num != 0) {
			files_extraneous = 1;
			if (options.dont_list_data_to_copy != 1) {
				if (options.list_extraneous == 1) {
					if (options.less_detailed != 1)
						detailed_output(file_surp_list,SCREEN,string15,0);
					else if (options.less_detailed == 1) {
						printf("\nExtraneous files:\n\n");
						if (options.delete_extraneous != 1) {
							for (file_list_element = file_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
								printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
						else if (options.delete_extraneous == 1) {
							for (file_list_element = file_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
								printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
					}
				}
			}
		}
	}
	if (options.ignore_symlinks != 1) {
		symlinks_surp_list = data_copy_info.symlinks_extraneous_list;
		if (symlinks_surp_list != NULL) {
			if (symlinks_surp_list->num != 0) {
				symlinks_extraneous = 1;
				if (options.dont_list_data_to_copy != 1) {
					if (options.list_extraneous == 1) {
						if (options.less_detailed != 1)
							detailed_output(symlinks_surp_list,SCREEN,string16,0);
						else if (options.less_detailed == 1) {
							printf("\nExtraneous symbolic links:\n\n");
							if (options.delete_extraneous != 1) {
								for (file_list_element = symlinks_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
									printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
							}
							else if (options.delete_extraneous == 1) {
								for (file_list_element = symlinks_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
									printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
							}
						}
					}
				}
			}
		}
	}
	dir_surp_list = data_copy_info.dirs_extraneous_list;
	if (dir_surp_list != NULL) {
		if (dir_surp_list->num != 0) {
			dirs_extraneous = 1;
			if (options.list_extraneous == 1) {
				if (options.dont_list_data_to_copy != 1) {
					if (options.less_detailed != 1)
						detailed_output(dir_surp_list,SCREEN,string17,0);
					else if (options.less_detailed == 1) {
						printf("\nExtraneous directories\n\n");
						if (options.delete_extraneous != 1) {
							for (dir_list_element = dir_surp_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next)
								printf("directory: %s\n location: %s\n", dir_list_element->name, dir_list_element->dir_location);
						}
						else if (options.delete_extraneous == 1) {
							for (dir_list_element = dir_surp_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next)
								printf("directory: %s\n location: %s\n", dir_list_element->name, dir_list_element->dir_location);
						}
					}
				}
			}
		}
	}
	file_ms_list = data_copy_info.diff_size_ms_list;
	if (file_ms_list != NULL) {
		if (file_ms_list->num != 0) {
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_smaller == 1) {
				if (options.ow_main_smaller == 1)
					ow_main_smaller = 1; // overwrite main smaller
				if (options.less_detailed != 1)
					detailed_output(file_ms_list,SCREEN,string4,0);
				else if (options.less_detailed == 1) {
					printf("\nFiles to overwrite. (source location files smaller than destination)\n\n");
					for (file_list_element = file_ms_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
				}
			}
		}
	}
	file_ml_list = data_copy_info.diff_size_ml_list;
	if (file_ml_list != NULL) {
		if (file_ml_list->num != 0) {
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_larger == 1) {
				if (options.ow_main_larger == 1)
					ow_main_larger = 1; // overwrite main larger
				if (options.less_detailed != 1)
					detailed_output(file_ml_list,SCREEN,string3,0);
				else if (options.less_detailed == 1) {
					printf("\nFiles to overwrite. (source location files larger than destination)\n\n");
					for (file_list_element = file_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
				}
			}
		}
	}
	file_mn_list = data_copy_info.diff_time_mn_list;
	if (file_mn_list != NULL) {
		if (file_mn_list->num != 0) {
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_newer == 1) {
				if (options.ow_main_newer == 1)
					ow_main_newer = 1; // overwrite main newer
				if (options.less_detailed != 1)
					detailed_output(file_mn_list,SCREEN,string9,0);
				else if (options.less_detailed == 1) {
					printf("\nFiles to overwrite. (source location files newer than destination)\n\n");
					for (file_list_element = file_mn_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
				}
			}
		}
	}
	file_mo_list = data_copy_info.diff_time_mo_list;
	if (file_mo_list != NULL) {
		if (file_mo_list->num != 0) {
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_newer == 1) {
				if (options.ow_main_older == 1)
					ow_main_older = 1; // overwrite main older
				if (options.less_detailed != 1)
					detailed_output(file_mo_list,SCREEN,string10,0);
				else if (options.less_detailed == 1) {
					printf("\nFiles to overwrite. (source location files older than destination)\n\n");
					for (file_list_element = file_mo_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
				}
			}
		}
	}
	if (options.ignore_symlinks != 1) {
		symlinks_ms_list = data_copy_info.symlinks_diff_size_ms_list;
		if (symlinks_ms_list != NULL) {
			if (symlinks_ms_list->num != 0) {
				if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_smaller == 1) {
					if (options.ow_main_smaller == 1)
						ow_symlinks_main_smaller = 1; // overwrite main smaller
					if (options.less_detailed != 1)
						detailed_output(symlinks_ms_list,SCREEN,string11,0);
					else if (options.less_detailed == 1) {
						printf("\nSymbolic links to overwrite. (source location files smaller than destination)\n\n");
						for (file_list_element = file_ms_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
					}
				}
			}
		}
	}
	if (options.ignore_symlinks != 1) {
		symlinks_ml_list = data_copy_info.symlinks_diff_size_ml_list;
		if (symlinks_ml_list != NULL) {
			if (symlinks_ml_list->num != 0) {
				if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_larger == 1) {
					if (options.ow_main_larger == 1)
						ow_symlinks_main_larger = 1; // overwrite main larger
					if (options.less_detailed != 1)
						detailed_output(symlinks_ml_list,SCREEN,string12,0);
					else if (options.less_detailed == 1) {
						printf("\nSymbolic links to overwrite. (source location files larger than destination)\n\n");
						for (file_list_element = file_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
					}
				}
			}
		}
	}
	if (options.ignore_symlinks != 1) {
		symlinks_mn_list = data_copy_info.symlinks_diff_time_mn_list;
		if (symlinks_mn_list != NULL) {
			if (symlinks_mn_list->num != 0) {
				if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_newer == 1) {
					if (options.ow_main_newer == 1)
						ow_symlinks_main_newer = 1; // overwrite main smaller
					if (options.less_detailed != 1)
						detailed_output(symlinks_mn_list,SCREEN,string13,0);
					else if (options.less_detailed == 1) {
						printf("\nSymbolic links to overwrite. (source location files newer than destination)\n\n");
						for (file_list_element = file_mn_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
					}
				}
			}
		}
	}
	if (options.ignore_symlinks != 1) {
		symlinks_mo_list = data_copy_info.symlinks_diff_time_mo_list;
		if (symlinks_mo_list != NULL) {
			if (file_mo_list->num != 0) {
				if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_older == 1) {
					if (options.ow_main_older == 1)
						ow_symlinks_main_older = 1; // overwrite main larger
					if (options.less_detailed != 1)
						detailed_output(symlinks_mo_list,SCREEN,string14,0);
					else if (options.less_detailed == 1) {
						printf("\nSymbolic links to overwrite. (source location files older than destination)\n\n");
						for (file_list_element = file_mo_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
					}
				}
			}
		}
	}
	// if there is some data to copy, depending on options: create copy file list, then copy/overwrite/delete the data
	if (copy_files == 1 || copy_dirs == 1 || files_extraneous == 1 || dirs_extraneous == 1 ||  ow_main_smaller == 1 || ow_main_larger == 1 || ow_main_newer == 1 
		|| ow_main_older == 1 || copy_symlinks == 1 || symlinks_extraneous == 1 ||  ow_symlinks_main_smaller == 1 || ow_symlinks_main_larger == 1 
		|| ow_symlinks_main_newer == 1 || ow_symlinks_main_older == 1) {
		if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
			if (copy_files == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				if (options.less_detailed != 1)
					detailed_output(file_list,TO_FILE,string1,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string1, strlen(string1));
					for (file_list_element = file_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						strcpy(file_location,file_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
			if (options.ignore_symlinks != 1) {
				if (copy_symlinks == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_list,TO_FILE,string1_1,copyfile);
					else if (options.less_detailed == 1) {
						write(copyfile, string1_1, strlen(string1_1));
						for (file_list_element = symlinks_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							strcpy(file_location,file_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
			}
			if (copy_dirs == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				if (options.less_detailed != 1)
					detailed_output(dir_list,TO_FILE,string2,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string2, strlen(string2));
					for (dir_list_element = dir_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next) {
						strcpy(file_location,dir_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
			if (options.ow_main_larger == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				if (options.less_detailed != 1)
					detailed_output(file_ml_list,TO_FILE,string3,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string3, strlen(string3));
					for (file_list_element = file_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						strcpy(file_location,file_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
			else if (options.ow_main_smaller == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				if (options.less_detailed != 1)
					detailed_output(file_ms_list,TO_FILE,string4,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string4, strlen(string4));
					for (file_list_element = file_ms_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						strcpy(file_location,file_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
			if (options.ow_main_newer == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				if (options.less_detailed != 1)
					detailed_output(file_mn_list,TO_FILE,string9,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string9, strlen(string9));
					for (file_list_element = file_mn_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						strcpy(file_location,file_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
			else if (options.ow_main_older == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				if (options.less_detailed != 1)
					detailed_output(file_mo_list,TO_FILE,string10,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string10, strlen(string10));
					for (file_list_element = file_mo_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						strcpy(file_location,file_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
			if (options.ignore_symlinks != 1) {
				if (options.ow_main_larger == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_ml_list,TO_FILE,string12,copyfile);
					else if (options.less_detailed == 1) {
						write(copyfile, string12, strlen(string12));
						for (file_list_element = symlinks_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							strcpy(file_location,file_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
				else if (options.ow_main_smaller == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_ms_list,TO_FILE,string11,copyfile);
					else if (options.less_detailed == 1) {
						write(copyfile, string11, strlen(string11));
						for (file_list_element = symlinks_ms_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							strcpy(file_location,file_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
			}
			if (options.ignore_symlinks != 1) {
				if (options.ow_main_newer == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_mn_list,TO_FILE,string13,copyfile);
					else if (options.less_detailed == 1) {
						write(copyfile, string13, strlen(string13));
						for (file_list_element = symlinks_mn_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							strcpy(file_location,file_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
				else if (options.ow_main_older == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_mo_list,TO_FILE,string14,copyfile);
					else if (options.less_detailed == 1) {
						write(copyfile, string14, strlen(string14));
						for (file_list_element = symlinks_mo_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							strcpy(file_location,file_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
			}
			if (options.copy_extraneous_back == 1 || options.just_copy_extraneous_back == 1) {
				if (options.less_detailed != 1)
					detailed_output(file_surp_list,TO_FILE,string5,copyfile);
				else if (options.less_detailed == 1) {
					if (files_extraneous == 1) {
						write(copyfile, string5, strlen(string5));
						for (file_list_element = file_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							strcpy(file_location,file_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
				if (options.ignore_symlinks != 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_surp_list,TO_FILE,string5_1,copyfile);
					else if (options.less_detailed == 1) {
						if (symlinks_extraneous == 1) {
							write(copyfile, string5_1, strlen(string5_1));
							for (file_list_element = symlinks_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
								strcpy(file_location,file_list_element->dir_location);
								strcat(file_location,"\n");
								write(copyfile, file_location, strlen(file_location));
							}
						}
					}
				}
				if (dirs_extraneous == 1) {
					if (options.less_detailed != 1)
						detailed_output(dir_surp_list,TO_FILE,string6,copyfile);
					else if (options.less_detailed == 1) {
						write(copyfile, string6, strlen(string6));
						for (dir_list_element = dir_surp_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next) {
							strcpy(file_location,dir_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
			}
			else if (options.delete_extraneous == 1 || options.just_delete_extraneous == 1) {
				if (options.less_detailed != 1)
					detailed_output(file_surp_list,TO_FILE,string7,copyfile);
				else if (options.less_detailed == 1) {
					if (files_extraneous == 1) {
						write(copyfile, string7, strlen(string7));
						for (file_list_element = file_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							strcpy(file_location,file_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
				if (options.ignore_symlinks != 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_surp_list,TO_FILE,string7_1,copyfile);
					else if (options.less_detailed == 1) {
						if (symlinks_extraneous == 1) {
							write(copyfile, string7_1, strlen(string7_1));
							for (file_list_element = symlinks_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
								strcpy(file_location,file_list_element->dir_location);
								strcat(file_location,"\n");
								write(copyfile, file_location, strlen(file_location));
							}
						}
					}
				}
				if (dirs_extraneous == 1) {
					if (options.less_detailed != 1)
						detailed_output(dir_surp_list,TO_FILE,string8,copyfile);
					else if (options.less_detailed == 1) {
						write(copyfile, string8, strlen(string8));
						for (dir_list_element = dir_surp_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next) {
							strcpy(file_location,dir_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
				if (options.dont_list_stats == 1 || options.just_copy_content_file == 1) {
					errno = 0;
					if (close(copyfile) == -1)
						perror("close");
				}	
			}
		} // if (options.copy_content_file == 1 ||
		if (options.just_copy_content_file == 1) {
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
		if (options.no_questions == 0) {
			if (options.dont_list_stats != 1) {
				if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
					print_results(BEFORE,SCREEN,copyfile);
				else if (options.copy_content_file == 1)
					print_results(BEFORE,PRINT_BOTH,copyfile);
				else if (options.just_copy_content_file == 1)
					print_results(BEFORE,TO_FILE,copyfile);
			}
			if (options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				if (copy_files == 1 || copy_dirs == 1 || copy_symlinks == 1) {
					printf("Do you want to write the missing files and directories? Type yes or no ...\n");
					while (fgets(line,BUF,stdin) != NULL) {
						length = strlen(line);
						line[length-1] = '\0';
						if (strcmp(line,"yes") == 0) {
							if (copy_files == 1) {
								read_write_data_res = read_write_data(data_copy_info.files_to_copy_list,1,NULL,NULL);
								if (read_write_data_res == 0) {
									printf("\nFiles were written successfully.\n");
									copied.copied_files = 1;
								}
								else if (read_write_data_res == 1) {
									printf("\nSome files were written successfully, but there were some errors.\n");
									copied.copied_files = 1;
								}
								else if (read_write_data_res == -1) {
									printf("\nError writing the files. Exiting.\n");
									clean_tree(thread_data_a->file_tree_top_dir,0);
									clean_tree(thread_data_b->file_tree_top_dir,0);
									clean_up_exit(thread_data_a, thread_data_b);;
									destroy_data_structs();
									exit(1);
								}
							}
							if (copy_symlinks == 1) {
								read_write_data_res = read_write_data(data_copy_info.symlinks_to_copy_list,1,NULL,NULL);
								if (read_write_data_res == 0) {
									printf("\nSymbolic links were written successfully.\n");
									copied.copied_symlinks = 1;
								}
								else if (read_write_data_res == 1) {
									printf("\nSome symbolic links were written successfully, but there were some errors.\n");
									copied.copied_symlinks = 1;
								}
								else if (read_write_data_res == -1) {
									printf("\nError writing the symbolic links. Exiting.\n");
									clean_tree(thread_data_a->file_tree_top_dir,0);
									clean_tree(thread_data_b->file_tree_top_dir,0);
									clean_up_exit(thread_data_a, thread_data_b);;
									destroy_data_structs();
									exit(1);
								}
							}
							if (copy_dirs == 1) {
								read_write_data_res = read_write_data(data_copy_info.dirs_to_copy_list,2,NULL,NULL);
								if (read_write_data_res == 0) {
									printf("\nDirectories were written succesfully.\n");
									copied.copied_directories = 1;
								}
								else if (read_write_data_res == 1) {
									printf("\nSome directories written successfully, but there were some errors.\n");
									copied.copied_directories = 1;
								}
								else if (read_write_data_res == -1) {
									printf("\nError writing the directories. Exiting.\n");
									clean_tree(thread_data_a->file_tree_top_dir,0);
									clean_tree(thread_data_b->file_tree_top_dir,0);
									clean_up_exit(thread_data_a, thread_data_b);;
									destroy_data_structs();
									exit(1);
								}
							}
							printf("\n");
							break;
						}
						else if (strcmp(line,"no") == 0) {
							copied.aborted_copying = 1;
							printf("\n");
							break;
						}
						else
							printf("Unrecognized answer. Type yes or no.\n");
					}
				}
			}
			if (options.copy_extraneous_back == 1 || options.just_copy_extraneous_back == 1) {
				if (dirs_extraneous == 1 || files_extraneous == 1) {
					printf("Do you want to write the extraneous data from the destionation directory back to the source? Type yes or no ...\n");
					while (fgets(line,BUF,stdin) != NULL) {
						length = strlen(line);
						line[length-1] = '\0';
						if (strcmp(line,"yes") == 0) {
							if (files_extraneous == 1) {
								read_write_data_res = read_write_data(file_surp_list,1,NULL,NULL);
								if (read_write_data_res == 0) {
									printf("\nExtraneous files were written successfully.\n");
									copied.copied_files_extraneous = 1;
								}
								else if (read_write_data_res == 1) {
									printf("\nSome extraneous files were written successfully, but there were some errors.\n");
									copied.copied_files_extraneous = 1;
								}
								else if (read_write_data_res == -1) {
									printf("\nError writing the extraneous files. Exiting.\n");
									clean_tree(thread_data_a->file_tree_top_dir,0);
									clean_tree(thread_data_b->file_tree_top_dir,0);
									clean_up_exit(thread_data_a, thread_data_b);;
									destroy_data_structs();
									exit(1);
								}
							}
							if (symlinks_extraneous == 1) {
								read_write_data_res = read_write_data(symlinks_surp_list,1,NULL,NULL);
								if (read_write_data_res == 0) {
									printf("\nExtraneous symbolic links were written successfully.\n");
									copied.copied_files_extraneous = 1;
								}
								else if (read_write_data_res == 1) {
									printf("\nSome extraneous symbolic links were written successfully, but there were some errors.\n");
									copied.copied_files_extraneous = 1;
								}
								else if (read_write_data_res == -1) {
									printf("\nError writing the extraneous symbolic links. Exiting.\n");
									clean_tree(thread_data_a->file_tree_top_dir,0);
									clean_tree(thread_data_b->file_tree_top_dir,0);
									clean_up_exit(thread_data_a, thread_data_b);;
									destroy_data_structs();
									exit(1);
								}
							}
							if (dirs_extraneous == 1) {
								read_write_data_res = read_write_data(dir_surp_list,2,NULL,NULL);
								if (read_write_data_res == 0) {
									printf("\nExtraneous directories were written successfully.\n");
									copied.copied_directories_extraneous = 1;
								}
								else if (read_write_data_res == 1) {
									printf("\nSome extraneous directories were written successfully, but there were also some errors.\n");
									copied.copied_directories_extraneous = 1;
								}
								else if (read_write_data_res == -1) {
									printf("\nError writing the extraneous directories. Exiting\n");
									clean_tree(thread_data_a->file_tree_top_dir,0);
									clean_tree(thread_data_b->file_tree_top_dir,0);
									clean_up_exit(thread_data_a, thread_data_b);;
									destroy_data_structs();
									exit(1);
								}
							}
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
			else if (options.delete_extraneous == 1 || options.just_delete_extraneous == 1) {
				if (dirs_extraneous == 1 || files_extraneous == 1) {
					printf("Do you want to delete the extraneous data in the destination directory? Type yes or no...\n");
					while (fgets(line,BUF,stdin) != NULL) {
						length = strlen(line);
						line[length-1] = '\0';
						if (strcmp(line,"yes") == 0) {
							if (files_extraneous == 1) {
								read_write_data_res = read_write_data(file_surp_list,5,NULL,NULL);
								if (read_write_data_res == 0) {
									printf("\nDeleting the extraneous files was successful.\n");
									copied.deleted_files_extraneous = 1;
								}
								else if (read_write_data_res == 1) {
									printf("\nDeleting some extraneous files was successful, but there were also some errors.\n");
									copied.deleted_files_extraneous = 1;
								}
								else if (read_write_data_res == -1) {
									printf("\nError deleting the extraneous files. Exiting.\n");
									clean_tree(thread_data_a->file_tree_top_dir,0);
									clean_tree(thread_data_b->file_tree_top_dir,0);
									clean_up_exit(thread_data_a, thread_data_b);;
									destroy_data_structs();
									exit(1);
								}
							}
							if (symlinks_extraneous == 1) {
								read_write_data_res = read_write_data(symlinks_surp_list,5,NULL,NULL);
								if (read_write_data_res == 0) {
									printf("\nDeleting the extraneous symbolic links was successful.\n");
									copied.deleted_symlinks_extraneous = 1;
								}
								else if (read_write_data_res == 1) {
									printf("\nDeleting some extraneous symbolic links was successful, but there were also some errors.\n");
									copied.deleted_symlinks_extraneous = 1;
								}
								else if (read_write_data_res == -1) {
									printf("\nError deleting the extraneous symbolic links. Exiting.\n");
									clean_tree(thread_data_a->file_tree_top_dir,0);
									clean_tree(thread_data_b->file_tree_top_dir,0);
									clean_up_exit(thread_data_a, thread_data_b);;
									destroy_data_structs();
									exit(1);
								}
							}
							if (dirs_extraneous == 1) {
								read_write_data_res = read_write_data(dir_surp_list,6,NULL,NULL);
								if (read_write_data_res == 0) {
									printf("\nDeleting the extraneous directories was successful.\n");
									copied.deleted_directories_extraneous = 1;
								}
								else if (read_write_data_res == 1) {
									printf("\nDeleting some extraneous directories was successfull, but there were also some errors.\n");
									copied.deleted_directories_extraneous = 1;
								}
								else if (read_write_data_res == -1) {
									printf("\nError deleting the extraneous directories. Exiting.\n");
									clean_tree(thread_data_a->file_tree_top_dir,0);
									clean_tree(thread_data_b->file_tree_top_dir,0);
									clean_up_exit(thread_data_a, thread_data_b);;
									destroy_data_structs();
									exit(1);
								}
							}

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
			if (ow_main_smaller == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				printf("Scan found the files with the same name, files in the main location being smaller than the files in the secondary location. "); 
				printf("Should the files in the secondary location be overwritten? Answer yes or no.\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						read_write_data_res = read_write_data(data_copy_info.diff_size_ms_list,4,NULL,NULL);
						if (read_write_data_res == 0) {
							printf("Larger files overwritten successfully.\n");
							copied.ow_smaller = 1;
						}
						else if (read_write_data_res == 1) {
							printf("There were some errors while overwriting the larger files.\n");
							copied.ow_smaller = 1;
						}
						else if (read_write_data_res == -1) {
							printf("There were some errors while overwriting the larger files. Exiting.\n");
							clean_tree(thread_data_a->file_tree_top_dir,0);
							clean_tree(thread_data_b->file_tree_top_dir,0);
							clean_up_exit(thread_data_a, thread_data_b);;
							destroy_data_structs();
							exit(1);
						}
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
			if (ow_main_larger == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				printf("Scan found the files with the same name, files in the main location being larger than the files in the secondary location. ");
				printf("Should the files in the secondary location be overwritten? Answer yes or no.\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						read_write_data_res = read_write_data(data_copy_info.diff_size_ml_list,4,NULL,NULL);
						if (read_write_data_res == 0) {
							printf("Smaller files were overwritten successfully\n");
							copied.ow_larger = 1;
						}
						else if (read_write_data_res == 1) {
							printf("There were some errors overwriting the smaller files.\n");
							copied.ow_larger = 1;
						}
						else if (read_write_data_res == -1) {
							printf("There were some errors while overwriting the smaller files. Exiting.\n");
							clean_tree(thread_data_a->file_tree_top_dir,0);
							clean_tree(thread_data_b->file_tree_top_dir,0);
							clean_up_exit(thread_data_a, thread_data_b);;
							destroy_data_structs();
							exit(1);
						}
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
			if (ow_main_newer == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				printf("Scan found the files with the same name, files in the main location being newer than the files in the secondary location. "); 
				printf("Should the secondary location files be overwritten? Answer yes or no.\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						read_write_data_res = read_write_data(data_copy_info.diff_time_mn_list,4,NULL,NULL);
						if (read_write_data_res == 0) {
							printf("Older files were overwritten successfully.\n");
							copied.ow_newer = 1;
						}
						else if (read_write_data_res == 1) {
							printf("There were some errors overwriting the older files.\n");
							copied.ow_newer = 1;
						}
						else if (read_write_data_res == -1) {
							printf("There were some errors while overwriting the older files. Exiting.\n");
							clean_tree(thread_data_a->file_tree_top_dir,0);
							clean_tree(thread_data_b->file_tree_top_dir,0);
							clean_up_exit(thread_data_a, thread_data_b);;
							destroy_data_structs();
							exit(1);
						}
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
			if (ow_main_older == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				printf("Scan found the files with the same name, files in the main location being older than the files in the secondary location. ");
				printf("Should the secondary location files be overwritten? Answer yes or no.\n");
				while (fgets(line,BUF,stdin) != NULL) {
					length = strlen(line);
					line[length-1] = '\0';
					if (strcmp(line,"yes") == 0) {
						read_write_data_res = read_write_data(data_copy_info.diff_time_mo_list,4,NULL,NULL);
						if (read_write_data_res == 0) {
							printf("Newer files were overwritten successfully.\n");
							copied.ow_older = 1;
						}
						else if (read_write_data_res == 1) {
							printf("There were some errors while overwriting the newer files.\n");
							copied.ow_older = 1;
						}
						else if (read_write_data_res == -1) {
							printf("There were some errors while overwriting the newer files. Exiting.\n");
							clean_tree(thread_data_a->file_tree_top_dir,0);
							clean_tree(thread_data_b->file_tree_top_dir,0);
							clean_up_exit(thread_data_a, thread_data_b);;
							destroy_data_structs();
							exit(1);
						}
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
			if (copied.copied_files == 1 || copied.copied_symlinks || copied.copied_directories == 1 || copied.copied_files_extraneous == 1 || 
			copied.copied_symlinks_extraneous == 1 || copied.copied_directories_extraneous == 1 || copied.deleted_files_extraneous == 1 || 
			copied.deleted_symlinks_extraneous || copied.deleted_directories_extraneous || copied.ow_smaller == 1 || copied.ow_larger == 1 || 
			copied.ow_newer == 1 || copied.ow_older == 1) {
				calc_stats(REGULAR);
				if (options.dont_list_stats != 1) {
					if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
						print_results(AFTER,SCREEN,copyfile);
					else if (options.copy_content_file == 1)
						print_results(AFTER,PRINT_BOTH,copyfile);
					else if (options.just_copy_content_file == 1)
						print_results(AFTER,TO_FILE,copyfile);
				}
			}
		} // if (options.no_questions == 0) {
		else if (options.no_questions == 1) {
			if (copy_files == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				read_write_data_res = read_write_data(data_copy_info.files_to_copy_list,1,NULL,NULL);
				if (read_write_data_res == 0) {
					printf("\nFiles were written successfully.\n");
					copied.copied_files = 1;
				}
				else if (read_write_data_res == 1) {
					printf("\nSome files written successfully, but there were some errors.\n");
					copied.copied_files = 1;
				}
				else if (read_write_data_res == -1) {
					printf("\nError writing the files. Exiting.\n");
					clean_tree(thread_data_a->file_tree_top_dir,0);
					clean_tree(thread_data_b->file_tree_top_dir,0);
					clean_up_exit(thread_data_a, thread_data_b);;
					destroy_data_structs();
					exit(1);
				}
			}
			if (copy_dirs == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				read_write_data_res = read_write_data(data_copy_info.dirs_to_copy_list,2,NULL,NULL);
				if (read_write_data_res == 0) {
					printf("\nDirectories were written succesfully.\n");
					copied.copied_directories = 1;
				}
				else if (read_write_data_res == 1) {
					printf("\nSome directories written successfully, but there were some errors.\n");
					copied.copied_directories = 1;
				}
				else if (read_write_data_res == -1) {
					printf("\nError writing the directories. Exiting.\n");
					clean_tree(thread_data_a->file_tree_top_dir,0);
					clean_tree(thread_data_b->file_tree_top_dir,0);
					clean_up_exit(thread_data_a, thread_data_b);;
					destroy_data_structs();
					exit(1);
				}
			}
			if (options.copy_extraneous_back == 1 || options.just_copy_extraneous_back == 1) {
				if (files_extraneous == 1) {
					read_write_data_res = read_write_data(data_copy_info.files_extraneous_list,1,NULL,NULL);
					if (read_write_data_res == 0) {
						printf("\nExtraneous files were written successfully.\n");
						copied.copied_files_extraneous = 1;
					}
					else if (read_write_data_res == 1) {
						printf("\nSome extraneous files were written successfully, but there were also some errors.\n");
						copied.copied_files_extraneous = 1;
					}
					else if (read_write_data_res == -1) {
						printf("\nError writing the extraneous files. Exiting.\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
					}
				}
				if (symlinks_extraneous == 1) {
					read_write_data_res = read_write_data(data_copy_info.symlinks_extraneous_list,1,NULL,NULL);
					if (read_write_data_res == 0) {
						printf("\nExtraneous symbolic links were written successfully.\n");
						copied.copied_symlinks_extraneous = 1;
					}
					else if (read_write_data_res == 1) {
						printf("\nSome extraneous symbolic links were written successfully, but there were also some errors.\n");
						copied.copied_symlinks_extraneous = 1;
					}
					else if (read_write_data_res == -1) {
						printf("\nError writing the extraneous symbolic links. Exiting.\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
					}
				}
				if (dirs_extraneous == 1) {
					read_write_data_res = read_write_data(data_copy_info.dirs_extraneous_list,2,NULL,NULL);
					if (read_write_data_res == 0) {
						printf("\nExtraneous directories were written successfully.\n");
						copied.copied_directories_extraneous = 1;
					}
					else if (read_write_data_res == 1) {
						printf("\nSome extraneous directories were written successfully, but there were also some errors.\n");
						copied.copied_directories_extraneous = 1;
					}
					else if (read_write_data_res == -1) {
						printf("\nError writing the extraneous directories. Exiting\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
					}
				}
			}
			else if (options.delete_extraneous == 1 || options.just_delete_extraneous == 1) {
				if (files_extraneous == 1) {
					read_write_data_res = read_write_data(data_copy_info.files_extraneous_list,5,NULL,NULL);
					if (read_write_data_res == 0) {
						printf("\nDeleting the extraneous files was successful.\n");
						copied.deleted_files_extraneous = 1;
					}
					else if (read_write_data_res == 1) {
						printf("\nDeleting some extraneous files was successful, but there were some errors.\n");
						copied.deleted_files_extraneous = 1;
					}
					else if (read_write_data_res == -1) {
						printf("\nError deleting the extraneous files. Exiting.\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
					}
				}
				if (symlinks_extraneous == 1) {
					read_write_data_res = read_write_data(data_copy_info.symlinks_extraneous_list,5,NULL,NULL);
					if (read_write_data_res == 0) {
						printf("\nDeleting the extraneous symbolic links was successful.\n");
						copied.deleted_symlinks_extraneous = 1;
					}
					else if (read_write_data_res == 1) {
						printf("\nDeleting some extraneous symbolic links was successful, but there were some errors.\n");
						copied.deleted_symlinks_extraneous = 1;
					}
					else if (read_write_data_res == -1) {
						printf("\nError deleting the extraneous symbolic links. Exiting.\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
					}
				}
				if (dirs_extraneous == 1) {
					read_write_data_res = read_write_data(data_copy_info.dirs_extraneous_list,6,NULL,NULL);
					if (read_write_data_res == 0) {
						printf("\nDeleting the extraneous directories was successful.\n");
						copied.deleted_directories_extraneous = 1;
					}
					else if (read_write_data_res == 1) {
						printf("\nDeleting some extraneous directories was successfull, but there were also some errors.\n");
						copied.deleted_directories_extraneous = 1;
					}
					else if (read_write_data_res == -1) {
						printf("\nError deleting the extraneous directories. Exiting.\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
					}
				}
			}
			/*if (options.just_copy_extraneous_back == 1) {
				if (files_extraneous == 1) {
					read_write_data_res = read_write_data(data_copy_info.files_extraneous_list,1,NULL,NULL);
					if (read_write_data_res == 0)
						printf("\nExtraneous files were written successfully.\n");
					else if (read_write_data_res == 1)
						printf("\nSome extraneous files were written successfully, but there were also some errors.\n");
					else if (read_write_data_res == -1) {
						printf("\nError writing the extraneous files. Exiting.\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
					}
				}
				if (dirs_extraneous == 1) {
					read_write_data(data_copy_info.dirs_extraneous_list,2,NULL,NULL);
					if (read_write_data_res == 0)
						printf("\nExtraneous directories were written successfully.\n");
					else if (read_write_data_res == 1)
						printf("\nSome extraneous directories were written successfully, but there were also some errors.\n");
					else if (read_write_data_res == -1) {
						printf("\nError writing the extraneous directories. Exiting\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
					}
				}
			}
			else if (options.just_delete_extraneous == 1) {
				if (files_extraneous == 1) {
					read_write_data_res = read_write_data(data_copy_info.files_extraneous_list,5,NULL,NULL);
					if (read_write_data_res == 0)
						printf("\nDeleting the extraneous files was successful.\n");
					else if (read_write_data_res == 1) 
						printf("\nDeleting some extraneous files was successful, but there were also some errors.\n");
					else if (read_write_data_res == -1) {
						printf("\nError deleting the extraneous files. Exiting.\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
					}
				}
				if (dirs_extraneous == 1) {
					read_write_data_res = read_write_data(data_copy_info.dirs_extraneous_list,6,NULL,NULL);
					if (read_write_data_res == 0)
						printf("\nDeleting the extraneous directories was successful.\n");
					else if (read_write_data_res == 1)
						printf("\nDeleting some extraneous directories was successfull, but there were also some errors.\n");
					else if (read_write_data_res == -1) {
						printf("\nError deleting the extraneous directories. Exiting.\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
					}
				}
			}*/
			if (options.ow_main_smaller == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				read_write_data_res = read_write_data(data_copy_info.diff_size_ms_list,4,NULL,NULL);
				if (read_write_data_res == 0) {
					printf("Larger files overwritten successfully.\n");
					copied.ow_smaller = 1;
				}
				else if (read_write_data_res == 1) {
					printf("There were some errors while overwriting the larger files.\n");
					copied.ow_smaller = 1;
				}
				else if (read_write_data_res == -1) {
					printf("There were some errors while overwriting the larger files. Exiting.\n");
						clean_tree(thread_data_a->file_tree_top_dir,0);
						clean_tree(thread_data_b->file_tree_top_dir,0);
						clean_up_exit(thread_data_a, thread_data_b);;
						destroy_data_structs();
						exit(1);
				}
			}
			if (options.ow_main_larger == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				read_write_data_res = read_write_data(data_copy_info.diff_size_ml_list,4,NULL,NULL);
				if (read_write_data_res == 0) {
					printf("Smaller files were overwritten successfully\n");
					copied.ow_larger = 1;
				}
				else if (read_write_data_res == 1) {
					printf("There were some errors overwriting the smaller files.\n");
					copied.ow_larger = 1;
				}
				else if (read_write_data_res == -1) {
					printf("There were some errors while overwriting the smaller files. Exiting.\n");
					clean_tree(thread_data_a->file_tree_top_dir,0);
					clean_tree(thread_data_b->file_tree_top_dir,0);
					clean_up_exit(thread_data_a, thread_data_b);;
					destroy_data_structs();
					exit(1);
				}
			}
			if (options.ow_main_newer == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				read_write_data_res = read_write_data(data_copy_info.diff_time_mn_list,4,NULL,NULL);
				if (read_write_data_res == 0) {
					printf("Older files were overwritten successfully.\n");
					copied.ow_newer = 1;
				}
				else if (read_write_data_res == 1) {
					printf("There were some errors overwriting the older files.\n");
					copied.ow_newer = 1;
				}
				else if (read_write_data_res == -1) {
					printf("There were some errors while overwriting the older files. Exiting.\n");
					clean_tree(thread_data_a->file_tree_top_dir,0);
					clean_tree(thread_data_b->file_tree_top_dir,0);
					clean_up_exit(thread_data_a, thread_data_b);;
					destroy_data_structs();
					exit(1);
				}
			}
			if (options.ow_main_older == 1 && options.just_copy_extraneous_back != 1 && options.just_delete_extraneous != 1) {
				read_write_data_res = read_write_data(data_copy_info.diff_time_mo_list,4,NULL,NULL);
				if (read_write_data_res == 0) {
					printf("Newer files were overwritten successfully.\n");
					copied.ow_older = 1;
				}
				else if (read_write_data_res == 1) {
					printf("There were some errors while overwriting the newer files.\n");
					copied.ow_older = 1;
				}
				else if (read_write_data_res == -1) {
					printf("There were some errors while overwriting the newer files. Exiting.\n");
					clean_tree(thread_data_a->file_tree_top_dir,0);
					clean_tree(thread_data_b->file_tree_top_dir,0);
					clean_up_exit(thread_data_a, thread_data_b);;
					destroy_data_structs();
					exit(1);
				}
			}
			if (copied.copied_files == 1 || copied.copied_symlinks || copied.copied_directories == 1 || copied.copied_files_extraneous == 1 || 
			copied.copied_symlinks_extraneous == 1 || copied.copied_directories_extraneous == 1 || copied.deleted_files_extraneous == 1 || 
			copied.deleted_symlinks_extraneous || copied.deleted_directories_extraneous || copied.ow_smaller == 1 || copied.ow_larger == 1 || 
			copied.ow_newer == 1 || copied.ow_older == 1) {
				calc_stats(REGULAR);
				if (options.dont_list_stats != 1) {
					if (options.copy_content_file != 1 && options.just_copy_content_file != 1)
						print_results(AFTER,SCREEN,copyfile);
					else if (options.copy_content_file == 1)
						print_results(AFTER,PRINT_BOTH,copyfile);
					else if (options.just_copy_content_file == 1)
						print_results(AFTER,TO_FILE,copyfile);
				}
			}
		}  // else if (no_questions == 1
	} // if (files_to_copy == 1 || dirs_to_copy == 1 || etc...
	else {
		printf("\nNo data to copy.\n");
		if (options.dont_list_stats != 1)
			print_results(BEFORE,SCREEN,0);
	}
	if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
		errno = 0;
		if (close(copyfile) == -1)
			perror("close");
	}	

	// free the data structures used for file trees
	clean_tree(thread_data_a->file_tree_top_dir,0);
	clean_tree(thread_data_b->file_tree_top_dir,0);

	/*free(pathname1);
	free(pathname2);
	if (thread_data_a->id != NULL)
		free(thread_data_a->id);
	if (thread_data_b->id != NULL)
		free(thread_data_b->id);
	free(thread_data_a);
	free(thread_data_b);
	*/
	clean_up_exit(thread_data_a, thread_data_b);
	destroy_data_structs();

	exit(0);
}

// free the data structures used to hold file lists to copy, delete, overwrite...
void destroy_data_structs(void) {
	if (data_copy_info.files_to_copy_list != NULL)
		dlist_destroy_2(data_copy_info.files_to_copy_list);
	if (data_copy_info.symlinks_to_copy_list != NULL)
		dlist_destroy_2(data_copy_info.symlinks_to_copy_list);
	if (data_copy_info.dirs_to_copy_list != NULL)
		dlist_destroy_2(data_copy_info.dirs_to_copy_list);
	if (data_copy_info.files_extraneous_list != NULL)
		dlist_destroy_2(data_copy_info.files_extraneous_list);
	if (data_copy_info.symlinks_extraneous_list != NULL)
		dlist_destroy_2(data_copy_info.symlinks_extraneous_list);
	if (data_copy_info.dirs_extraneous_list != NULL)
		dlist_destroy_2(data_copy_info.dirs_extraneous_list);
	if (data_copy_info.diff_size_ms_list != NULL)
		dlist_destroy_3(data_copy_info.diff_size_ms_list);
	if (data_copy_info.diff_size_ml_list != NULL)
		dlist_destroy_3(data_copy_info.diff_size_ml_list);
	if (data_copy_info.diff_time_mn_list != NULL)
		dlist_destroy_3(data_copy_info.diff_time_mn_list);
	if (data_copy_info.diff_time_mo_list != NULL)
		dlist_destroy_3(data_copy_info.diff_time_mo_list);
	if (data_copy_info.symlinks_diff_size_ms_list != NULL)
		dlist_destroy_3(data_copy_info.symlinks_diff_size_ms_list);
	if (data_copy_info.symlinks_diff_size_ml_list != NULL)
		dlist_destroy_3(data_copy_info.symlinks_diff_size_ml_list);
	if (data_copy_info.symlinks_diff_time_mn_list != NULL)
		dlist_destroy_3(data_copy_info.symlinks_diff_time_mn_list);
	if (data_copy_info.symlinks_diff_time_mo_list != NULL)
		dlist_destroy_3(data_copy_info.symlinks_diff_time_mo_list);
}

void clean_up_exit(struct thread_struct *thread_data_a, struct thread_struct *thread_data_b)
{
	free(pathname1);
	free(pathname2);
	if (thread_data_a->id != NULL)
		free(thread_data_a->id);
	if (thread_data_b->id != NULL)
		free(thread_data_b->id);
	free(thread_data_a);
	free(thread_data_b);
}

#include "help.h"
void show_help()
{
	printf("OPTIONS:\n");
	printf("%-37s  %s\n", help_string11, help_string12); // --list-extraneous
	printf("%-37s  %s\n", help_string1, help_string2); // --copy-extraneous
	printf("%-37s  %s\n", help_string3, help_string4); // --delete-extraneous
	printf("%-37s  %s\n", help_string43, help_string44); // --just-copy-extraneous-back
	printf("%-37s  %s\n", help_string61, help_string62); // --just-delete-extraneous
	printf("%-37s  %s\n", help_string9, help_string10); // --list-conflicting
	printf("%-37s  %s\n", help_string5, help_string6); // --overwrite-with-smaller
	printf("%-37s  %s\n", help_string7, help_string8); // --overwrite-with-larger
	printf("%-37s  %s\n", help_string53, help_string54); // --overwrite-with-newer
	printf("%-37s  %s\n", help_string55, help_string56); // --overwrite-with-older
	printf("%-37s  %s\n", help_string57, help_string58); // --ignore
	printf("%-37s  %s\n", help_string45, help_string46); // follow-sym-links
	printf("%-37s  %s\n", help_string69, help_string70); // ignore-symlinks
	printf("%-37s  %s\n", help_string71, help_string72); // less-detailed
	printf("%-37s  %s\n", help_string25, help_string26); // --dont-list-stats
	printf("%-37s  %s\n", help_string13, help_string14); // --dont-list-data-to-copy
	printf("%-37s  %s\n", help_string39, help_string40); // --dont-show-read-process
	printf("%-37s  %s\n", help_string41, help_string42); // --dont-show-write-process
	printf("%-37s  %s\n", help_string27, help_string28); // --dont-quit-read-errors
	printf("%-37s  %s\n", help_string29, help_string30); // --dont-quit-write-errors
	printf("%-37s  %s\n", help_string31, help_string32); // --dont-quit-delete-errors
	printf("%-37s  %s\n", help_string15, help_string16); // --help
	printf("%-37s  %s\n", help_string17, help_string18); // --version
	printf("%-37s  %s\n", help_string21, help_string22); // --copy-content-file
	printf("%-37s  %s\n", help_string23, help_string24); // --just-copy-content-file
	printf("%-37s  %s\n", help_string33, help_string34); // --no-questions
	printf("%-37s  %s\n", help_string47, help_string48); // --no-access-time
	printf("%-37s  %s\n", help_string49, help_string50); // --preserve-atime
	printf("%-37s  %s\n", help_string51, help_string52); // --preserve-mtime
	printf("%-37s  %s\n", help_string63, help_string64); // --preserve-perms
	printf("%-37s  %s\n", help_string59, help_string60); // --size-mode
	printf("%-37s  %s\n", help_string65, help_string66); // --acls
	printf("%-37s  %s\n", help_string67, help_string68); // --xattrs
	printf("%-37s  %s\n", help_string35, help_string36); // --unit=OPTION
	printf("%-37s  %s\n", help_string37, help_string38); // --si-units
}
