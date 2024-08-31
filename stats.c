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
#define BUF 100
#define SCREEN 101		// output from list_stats() and calc_size() to stdout
#define TO_FILE 102		// output from list_stats() and calc_size() to file
#define PRINT_BOTH 103		// output from list_status() and calc_size() to both stdout and file
#define NORMAL 104
#define AS_RETURN_VAL 105
#define BOTH 106

int clean_tree(DList_of_lists *, short);								// free the dynamically allocated file tree
int write_contents_to_file(DList_of_lists *directory, short opened, int f_descriptor);			// write the file trees to a file
//char *list_stats(int after_c, struct copied_or_not copied, int output, int fd);
char *list_stats(int after_c);
char *calc_size(unsigned long data_size, int other_unit, int output, int fd);
void print_errors(void);

//char *list_stats(int after_c, struct copied_or_not copied, int output, int fd)
char *list_stats(int after_c)
{
	extern struct options_menu options;				// data structure used to set options
	extern struct Data_Copy_Info data_copy_info;
	extern struct copied_or_not copied;

	unsigned long size_to_copy;
	unsigned long after_copying_size;
	unsigned long after_copying_size_surp;
	unsigned long after_copying_file_num;
	unsigned long after_copying_symlinks_num;
	unsigned long after_copying_dir_num;

	size_to_copy = 0;

	// before copying
	if (after_c == 0) {
		printf("\n");
		printf("\n");
		printf("SOURCE DIRECTORY\n");
		printf("\n");
		printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_a + data_copy_info.global_symlink_num_a);
		printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
		printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_a);
		printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
		printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a + data_copy_info.global_symlink_size_a);
		// calc_size(): size of files/directories in the more appropriate or user specified unit
		printf("Size:");
		calc_size(data_copy_info.global_files_size_a + data_copy_info.global_symlink_size_a,options.other_unit,NORMAL,0);
		printf("\n");
		printf("\n");
		printf("DESTINATION DIRECTORY\n");
		printf("\n");
		printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_b + data_copy_info.global_symlink_num_b);
		printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
		printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_b);
		printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
		printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b + data_copy_info.global_symlink_size_b);
		printf("Size:");
		// calc_size(): size of files/directories in the more appropriate or user specified unit
		calc_size(data_copy_info.global_files_size_b + data_copy_info.global_symlink_size_b,options.other_unit,NORMAL,0);
		printf("\n");
		printf("\n");

		// to copy stats
		if (options.ignore_symlinks != 1) {
			size_to_copy = data_copy_info.global_files_to_copy_size + data_copy_info.global_dirs_to_copy_size + data_copy_info.global_symlinks_to_copy_size;
			printf("Size of files and directories to copy: ");
			if (size_to_copy == 0)
				printf("0  ");
			calc_size(size_to_copy,options.other_unit,NORMAL,0);
			if (size_to_copy == 0)
				printf("  0 bytes\n");
			else
				printf("  %ld bytes\n", size_to_copy);
		}
		else if (options.ignore_symlinks == 1) {
			size_to_copy = data_copy_info.global_files_to_copy_size + data_copy_info.global_dirs_to_copy_size;
			printf("Files and directories to copy: ");
			if (size_to_copy == 0)
				printf("0  ");
			calc_size(size_to_copy,options.other_unit,NORMAL,0);
			if (size_to_copy == 0)
				printf("  0 bytes\n");
			else
				printf("  %ld bytes\n", size_to_copy);
		}
		printf("Number of individual files to copy: %ld  ", data_copy_info.global_files_to_copy_num);
		calc_size(data_copy_info.global_files_to_copy_size,options.other_unit,NORMAL,0);
		printf("  %ld bytes\n", data_copy_info.global_files_to_copy_size);
		if (options.ignore_symlinks != 1) {
			printf("Number of individual symbolic links to copy: %ld  ", data_copy_info.global_symlinks_to_copy_num);
			calc_size(data_copy_info.global_symlinks_to_copy_size,options.other_unit,NORMAL,0);
			printf("  %ld bytes\n", data_copy_info.global_symlinks_to_copy_size);
		}
		printf("Number of directories to copy: %ld  ", data_copy_info.global_dirs_to_copy_num);
		calc_size(data_copy_info.global_dirs_to_copy_size,options.other_unit,NORMAL,0);
		printf("  %ld bytes\n", data_copy_info.global_dirs_to_copy_size);
		printf("Number of files within directories to copy: %ld\n", data_copy_info.global_files_within_dirs_to_copy_num);
		printf("Number of symbolic links within directories to copy: %ld\n", data_copy_info.global_symlinks_within_dirs_to_copy_num);
		printf("Number of individual extraneous files: %ld  ", data_copy_info.global_files_extraneous_num);
		calc_size(data_copy_info.global_files_extraneous_size,options.other_unit,NORMAL,0);
		printf("  %ld bytes\n", data_copy_info.global_files_extraneous_size);
		printf("Number of individual extraneous symbolic links: %ld  ", data_copy_info.global_symlinks_extraneous_num);
		calc_size(data_copy_info.global_symlinks_extraneous_size,options.other_unit,NORMAL,0);
		printf("  %ld bytes\n", data_copy_info.global_symlinks_extraneous_size);
		printf("Number of extraneous directories: %ld  ", data_copy_info.global_dirs_extraneous_num);
		calc_size(data_copy_info.global_dirs_extraneous_size,options.other_unit,NORMAL,0);
		printf("  %ld bytes\n", data_copy_info.global_dirs_extraneous_size);
		printf("Number of files within extraneous directories: %ld\n", data_copy_info.global_files_within_dirs_extraneous_num);
		printf("Number of symbolic links within extraneous directories: %ld\n", data_copy_info.global_symlinks_within_dirs_extraneous_num);
		if (options.time_based == 1) {
			printf("Same files with different modification time (main location newer): %ld\n", data_copy_info.global_diff_time_mn_num);
			printf("Same files with different modification time (main location older): %ld\n", data_copy_info.global_diff_time_mo_num);
			if (options.ignore_symlinks != 1) {
				printf("Same symbolic links with different modification time (main location newer): %ld\n", data_copy_info.global_diff_symlinks_time_mn_num);
				printf("Same symbolic with different modification time (main location older): %ld\n", data_copy_info.global_diff_symlinks_time_mo_num);
			}
		}
		else if (options.size_based == 1) {
			printf("Same files with different size (main location larger): %ld\n", data_copy_info.global_diff_size_ml_num);
			printf("Same files with different size (main location smaller): %ld\n", data_copy_info.global_diff_size_ms_num);
			if (options.ignore_symlinks != 1) {
				printf("Same symbolic links with different size (main location larger): %ld\n", data_copy_info.global_diff_symlinks_size_ml_num);
				printf("Same symbolic links with different size (main location smaller): %ld\n", data_copy_info.global_diff_symlinks_size_ms_num);
			}
		}
		printf("\n");
		printf("\n");
	}
	// after copying
	else if (after_c == 1) {
		printf("\n");
		printf("\n");
		printf("Before copying:\n");
		list_stats(0);
		printf("After copying:\n");
		printf("\n");
		printf("\n");
		printf("SOURCE DIRECTORY\n");
		printf("\n");
		// for files and symbolic links
		if (options.copy_extraneous_back == 1 || options.just_copy_extraneous_back == 1) {
			if (copied.copied_extraneous == 1) {
				after_copying_file_num = 0;
				after_copying_file_num = data_copy_info.global_file_num_a + data_copy_info.global_files_extraneous_num 
					+ data_copy_info.global_files_within_dirs_extraneous_num;
				printf("Number of files: %ld\n", after_copying_file_num);
				if (options.ignore_symlinks != 1) {
					after_copying_symlinks_num = 0;
					after_copying_symlinks_num = data_copy_info.global_symlink_num_a + data_copy_info.global_symlinks_extraneous_num 
					+ data_copy_info.global_symlinks_within_dirs_extraneous_num;
					printf("Number of symbolic links: %ld\n", after_copying_symlinks_num);
				}
				else if (options.ignore_symlinks == 1) {
					printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_a);
				}
				printf("Number of files and symbolic links: %ld\n", after_copying_file_num + after_copying_symlinks_num);
			}
			else if (copied.copied_extraneous == 0) {
				printf("Aborted copying the extraneous data back to the source.\n");
				printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_a + data_copy_info.global_symlink_num_a);
				printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
				printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_a);
			}
		}
		else {
			printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_a + data_copy_info.global_symlink_num_a);
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_a);
		}
		// for directories
		if (options.copy_extraneous_back == 1 || options.just_copy_extraneous_back == 1) {
			if (copied.copied_extraneous == 1) {
				after_copying_dir_num = 0;
				after_copying_dir_num = data_copy_info.global_dir_num_a + data_copy_info.global_dirs_extraneous_num;
				printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
			}
			else if (copied.copied_extraneous == 0) {
				printf("Aborted copying the extraneous data back to the source.\n");
				printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			}
		}
		else
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
		// size
		if (options.copy_extraneous_back == 1 || options.just_copy_extraneous_back == 1) {
			if (copied.copied_extraneous == 1) {
				after_copying_size_surp = 0;
				if (options.ignore_symlinks != 1) {
					after_copying_size_surp = data_copy_info.global_files_size_a + data_copy_info.global_symlink_size_a + data_copy_info.global_files_extraneous_size 
					+ data_copy_info.global_symlinks_extraneous_size + data_copy_info.global_dirs_extraneous_size;
					printf("Size of directory in bytes after copying: %ld\n", after_copying_size_surp);
					printf("Size:");
					calc_size(after_copying_size_surp,options.other_unit,NORMAL,0);
				}
				else if (options.ignore_symlinks == 1) {
					after_copying_size_surp = data_copy_info.global_files_size_a + data_copy_info.global_symlink_size_a
					+ data_copy_info.global_files_extraneous_size + data_copy_info.global_dirs_extraneous_size;
					printf("Size of directory in bytes after copying: %ld\n", after_copying_size_surp);
					printf("Size:");
					calc_size(after_copying_size_surp,options.other_unit,NORMAL,0);
				}
			}
			else if (copied.copied_extraneous == 0) {
				printf("Aborted copying the extraneous data back to the source: ");
				printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
				printf("Size:");
				// calc_size(): size of files/directories in the more appropriate or user specified unit
				calc_size(data_copy_info.global_files_size_a,options.other_unit,NORMAL,0);
			}
		}
		else {
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a + data_copy_info.global_symlink_size_a);
			printf("Size:");
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a + data_copy_info.global_symlink_size_a, options.other_unit,NORMAL,0);
		}
		printf("\n");
		printf("\n");
		printf("DESTINATION DIRECTORY\n");
		printf("\n");
		
		printf("copied.copied_data = %d\n", copied.copied_data);
		if (copied.copied_data == 1) {
			after_copying_file_num = 0;
			after_copying_file_num = data_copy_info.global_file_num_b + data_copy_info.global_files_to_copy_num + data_copy_info.global_files_within_dirs_to_copy_num;
			if (options.delete_extraneous == 1) {
				if (copied.deleted_extraneous == 1) {
					after_copying_file_num -= data_copy_info.global_files_extraneous_num;
				}
				else if (copied.deleted_extraneous == 0) {
					printf("Aborted deleting extraneous data (files).\n");
				}
			}
			if (options.ignore_symlinks != 1) {
				after_copying_symlinks_num = 0;
				after_copying_symlinks_num = data_copy_info.global_symlink_num_b + data_copy_info.global_symlinks_to_copy_num + 
				data_copy_info.global_symlinks_within_dirs_to_copy_num;
				if (options.delete_extraneous == 1) {
					if (copied.deleted_extraneous == 1) {
						after_copying_symlinks_num -= data_copy_info.global_symlinks_extraneous_num;
						after_copying_symlinks_num -= data_copy_info.global_symlinks_within_dirs_extraneous_num;
					}
					else if (copied.deleted_extraneous == 0) {
						printf("Aborted deleting extraneous data (symlinks).\n");
					}
				}
			}
			if (options.ignore_symlinks != 1)
				printf("Number of files and symbolic links: %ld\n", after_copying_file_num + after_copying_symlinks_num);
			else if (options.ignore_symlinks == 1)
				printf("Number of files and symbolic links: %ld\n", after_copying_file_num + data_copy_info.global_symlink_num_b);

			printf("Number of files: %ld\n", after_copying_file_num);

			if (options.ignore_symlinks != 1)
				printf("Number of symbolic links: %ld\n", after_copying_symlinks_num);
			else if (options.ignore_symlinks == 1)
				printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_b);

			after_copying_dir_num = 0;
			after_copying_dir_num = data_copy_info.global_dir_num_b + data_copy_info.global_dirs_to_copy_num;
			if (options.delete_extraneous == 1) {
				if (copied.deleted_extraneous == 1) {
					after_copying_dir_num -= data_copy_info.global_dirs_extraneous_num;
					printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
				}
				else if (copied.deleted_extraneous == 0) {
					printf("Aborted deleting extraneous data.\n");
					printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
				}
			}
			else
				printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);

			after_copying_size = 0;
			if (options.ignore_symlinks != 1) {
				after_copying_size = data_copy_info.global_files_size_b + data_copy_info.global_symlink_size_b + data_copy_info.global_files_to_copy_size + 
				data_copy_info.global_symlinks_to_copy_size + data_copy_info.global_dirs_to_copy_size;
			}
			else if (options.ignore_symlinks == 1) {
				after_copying_size = data_copy_info.global_files_size_b + data_copy_info.global_symlink_size_b + 
				data_copy_info.global_files_to_copy_size + data_copy_info.global_dirs_to_copy_size;
			}
			// - errors.file_read_write_errors_size;
			if (options.ow_main_larger == 1) {
				if (copied.ow_larger == 1) {
					after_copying_size -= data_copy_info.global_diff_size_ml_orig_size;
					after_copying_size += data_copy_info.global_diff_size_ml_size;
				}
				else if (copied.ow_larger == 0) {
					printf("Aborted overwriting the same files with different sizes.\n");
				}
			}
			else if (options.ow_main_smaller == 1) {
				if (copied.ow_smaller == 1) {
					after_copying_size -= data_copy_info.global_diff_size_ms_orig_size;
					after_copying_size += data_copy_info.global_diff_size_ms_size;
				}
				else if (copied.ow_smaller == 0) {
					printf("Aborted overwriting the same files with different sizes.\n");
				}
			}
			if (options.ow_main_newer == 1) {
				if (copied.ow_newer == 1) {
					after_copying_size -= data_copy_info.global_diff_time_mn_orig_size;
					after_copying_size += data_copy_info.global_diff_time_mn_size;
				}
				else if (copied.ow_newer == 0) {
					printf("Aborted overwriting the same files with different modification time.\n");
				}
			}
			else if (options.ow_main_older == 1) {
				if (copied.ow_older == 1) {
					after_copying_size -= data_copy_info.global_diff_time_mo_orig_size;
					after_copying_size += data_copy_info.global_diff_time_mo_size;
				}
				else if (copied.ow_older == 0) {
					printf("Aborted overwriting the same files with different modification time.\n");
				}
			}
			if (options.delete_extraneous == 1) {
				if (copied.deleted_extraneous == 1) {
					after_copying_size -= data_copy_info.global_files_extraneous_size;
					if (options.ignore_symlinks != 1)
						after_copying_size -= data_copy_info.global_symlinks_extraneous_size;
					after_copying_size -= data_copy_info.global_dirs_extraneous_size;
					printf("Size of directory in bytes after copying: %ld\n", after_copying_size);
					printf("Size:");
					calc_size(after_copying_size,options.other_unit,NORMAL,0);
					printf("\n");
					printf("\n");
				}
				else if (copied.deleted_extraneous == 0) {
					printf("Aborted deleting extraneous data.\n");
					printf("Size of directory in bytes after copying: %ld\n", after_copying_size);
					printf("Size:");
					calc_size(after_copying_size,options.other_unit,NORMAL,0);
					printf("\n");
					printf("\n");
				}
			}
			else {
				printf("Size of directory in bytes after copying: %ld\n", after_copying_size);
				printf("Size:");
				calc_size(after_copying_size,options.other_unit,NORMAL,0);
				printf("\n");
				printf("\n");
			}
		} // if (copied.copied_data == 1)
		else if ((copied.copied_data == 0 && options.delete_extraneous == 1) || (copied.copied_data == 0 && options.just_delete_extraneous == 1)) {
			after_copying_file_num = data_copy_info.global_file_num_b;
			after_copying_symlinks_num = data_copy_info.global_symlink_num_b;
			after_copying_dir_num = data_copy_info.global_dir_num_b;
			after_copying_size = data_copy_info.global_files_size_b + data_copy_info.global_symlink_size_b;
			if (copied.deleted_extraneous == 1) {
				after_copying_file_num -= data_copy_info.global_files_extraneous_num;
				after_copying_file_num -= data_copy_info.global_files_within_dirs_extraneous_num;
				if (options.ignore_symlinks != 1) {
					after_copying_symlinks_num -= data_copy_info.global_symlinks_extraneous_num;
					after_copying_symlinks_num -= data_copy_info.global_symlinks_within_dirs_extraneous_num;
				}
				printf("Number of files and symbolic links: %ld\n", after_copying_file_num + after_copying_symlinks_num);
				printf("Number of files: %ld\n", after_copying_file_num);
				printf("Number of symbolic links: %ld\n", after_copying_symlinks_num);
				after_copying_dir_num -= data_copy_info.global_dirs_extraneous_num;
				printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
				after_copying_size -= data_copy_info.global_files_extraneous_size;
				if (options.ignore_symlinks != 1)
					after_copying_size -= data_copy_info.global_symlinks_extraneous_size;
				after_copying_size -= data_copy_info.global_dirs_extraneous_size;
				printf("Size of directory in bytes after copying: %ld\n", after_copying_size);
				printf("Size:");
				calc_size(after_copying_size,options.other_unit,NORMAL,0);
				printf("\n");
				printf("\n");
			}
			else if (copied.deleted_extraneous == 0) {
				printf("Aborted deleting extraneous data.\n");
				printf("Number of files and symbolic links: %ld\n", after_copying_file_num + after_copying_symlinks_num);
				printf("Number of files: %ld\n", after_copying_file_num);
				printf("Number of symbolic links: %ld\n", after_copying_symlinks_num);
				printf("Number of directories (excluding the top directory): %ld\n", after_copying_dir_num);
				printf("Size of directory in bytes after copying: %ld\n", after_copying_size);
				printf("Size:");
				calc_size(after_copying_size,options.other_unit,NORMAL,0);
				printf("\n");
				printf("\n");
			}
		}
	}
	// full directory copying
	else if (after_c == 2) {
		if (copied.full_dir1_copied == 1) {
			printf("\n");
			printf("\n");
			printf("Full directory copied.\n");
			printf("\n");
			printf("\n");
			printf("SOURCE DIRECTORY\n");
			printf("\n");
			printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_a + data_copy_info.global_symlink_num_a);
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_a);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
			printf("Size:");
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a,options.other_unit,NORMAL,0);
			printf("\n");
			printf("\n");
			printf("DESTINATION DIRECTORY\n");
			printf("\n");
			printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_a + data_copy_info.global_symlink_num_a);
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_a);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
			printf("Size:");
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a,options.other_unit,NORMAL,0);
		}
		else if (copied.full_dir1_copied == 0 && copied.full_dir2_copied == 0) {
			printf("\n");
			printf("\n");
			printf("Aborted copying the missing files and directories (from the source to the destination): ");
			printf("\n");
			printf("\n");
			printf("SOURCE DIRECTORY\n");
			printf("\n");
			printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_a + data_copy_info.global_symlink_num_a);
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_a);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
			printf("Size:");
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a,options.other_unit,NORMAL,0);
			printf("\n");
			printf("\n");
			printf("DESTINATION DIRECTORY\n");
			printf("\n");
			printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_b + data_copy_info.global_symlink_num_b);
			printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
			printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_b);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
			printf("Size:");
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_b,options.other_unit,NORMAL,0);
		}
		if (copied.full_dir2_copied == 1) {
			printf("\n");
			printf("\n");
			printf("Full directory copied.\n");
			printf("\n");
			printf("\n");
			printf("SOURCE DIRECTORY\n");
			printf("\n");
			printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_b + data_copy_info.global_symlink_num_b);
			printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
			printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_b);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
			printf("Size:");
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_b,options.other_unit,NORMAL,0);
			printf("\n");
			printf("\n");
			printf("DESTINATION DIRECTORY\n");
			printf("\n");
			printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_b + data_copy_info.global_symlink_num_b);
			printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
			printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_b);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
			printf("Size:");
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_b,options.other_unit,NORMAL,0);
		}
		else if (copied.full_dir1_copied == 0 && copied.full_dir2_copied == 0) {
			printf("\n");
			printf("\n");
			printf("Aborted copying the missing files and directories (from the destination to the source): ");
			printf("\n");
			printf("\n");
			printf("SOURCE DIRECTORY\n");
			printf("\n");
			printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_a + data_copy_info.global_symlink_num_a);
			printf("Number of files: %ld\n", data_copy_info.global_file_num_a);
			printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_a);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_a);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_a);
			printf("Size:");
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_a,options.other_unit,NORMAL,0);
			printf("\n");
			printf("\n");
			printf("DESTINATION DIRECTORY\n");
			printf("\n");
			printf("Number of files and symbolic links: %ld\n", data_copy_info.global_file_num_b + data_copy_info.global_symlink_num_b);
			printf("Number of files: %ld\n", data_copy_info.global_file_num_b);
			printf("Number of symbolic links: %ld\n", data_copy_info.global_symlink_num_b);
			printf("Number of directories (excluding the top directory): %ld\n", data_copy_info.global_dir_num_b);
			printf("Size of directory in bytes: %ld\n", data_copy_info.global_files_size_b);
			printf("Size:");
			// calc_size(): size of files/directories in the more appropriate or user specified unit
			calc_size(data_copy_info.global_files_size_b,options.other_unit,NORMAL,0);
		}
	}
	print_errors();
}

char *calc_size(unsigned long data_size, int other_unit, int output, int fd)
{
	extern struct options_menu options;				// data structure used to set options

	unsigned long power1, power2, power3, power4;
	long double unit;
	static char return_val[1024];

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
		if (output == NORMAL) {
			if (data_size != 0) {
				if (data_size >= power4) {
					unit = (long double) data_size / power4;
					printf(" %.2Lf TB", unit);
				}
				else if (data_size >= power3) {
					unit = (long double) data_size / power3;
					printf(" %.2Lf GB", unit);
				}
				else if (data_size >= power2) {
					unit = (long double) data_size / power2;
					printf(" %.2Lf MB", unit);
				}
				else if (data_size >= power1) {
					unit = (long double) data_size / power1;
					printf(" %.2Lf KB", unit);
				}
				else if (data_size < power1) {
					printf(" %ld bytes", data_size);
				}
			}
			else if (data_size == 0)
				printf(" 0 bytes");
		}
		else if (output == AS_RETURN_VAL) {
			if (data_size != 0) {
				if (data_size >= power4) {
					unit = (long double) data_size / power4;
					sprintf(return_val, " %.2Lf TB", unit);
					return return_val;
				}
				else if (data_size >= power3) {
					unit = (long double) data_size / power3;
					sprintf(return_val, " %.2Lf GB", unit);
					return return_val;
				}
				else if (data_size >= power2) {
					unit = (long double) data_size / power2;
					sprintf(return_val, " %.2Lf MB", unit);
					return return_val;
				}
				else if (data_size >= power1) {
					unit = (long double) data_size / power1;
					sprintf(return_val, " %.2Lf KB", unit);
					return return_val;
				}
				else if (data_size < power1) {
					sprintf(return_val, " %ld bytes", data_size);
					return return_val;
				}
			}
			else if (data_size == 0)
				printf(" 0 bytes");
		}
	}
	else if (options.other_unit == 1) {
		if (output == NORMAL) {
			if (data_size != 0) {
				if (strcmp(options.unit,"TB") == 0) {
					unit = (long double) data_size / power4;
					printf(" %.2Lf TB", unit);
				}
				else if (strcmp(options.unit,"GB") == 0) {
					unit = (long double) data_size / power3;
					printf(" %.2Lf GB", unit);
				}
				else if (strcmp(options.unit,"MB") == 0) {
					unit = (long double) data_size / power2;
					printf(" %.2Lf MB", unit);
					}
				else if (strcmp(options.unit,"KB") == 0) {
					unit = (long double) data_size / power1;
					printf(" %.2Lf KB", unit);
				}
				else if (data_size < power1) {
					printf(" %ld bytes", data_size);
				}
			}
			else if (data_size == 0)
				printf(" 0 bytes");
		}
		else if (output == AS_RETURN_VAL) {
			if (data_size != 0) {
				if (strcmp(options.unit,"TB") == 0) {
					unit = (long double) data_size / power4;
					sprintf(return_val, " %.2Lf TB", unit);
					return return_val;
				}
				else if (strcmp(options.unit,"GB") == 0) {
					unit = (long double) data_size / power3;
					sprintf(return_val, " %.2Lf GB", unit);
					return return_val;
				}
				else if (strcmp(options.unit,"MB") == 0) {
					unit = (long double) data_size / power2;
					sprintf(return_val, " %.2Lf MB", unit);
					return return_val;
					}
				else if (strcmp(options.unit,"KB") == 0) {
					unit = (long double) data_size / power1;
					sprintf(return_val, " %.2Lf KB", unit);
					return return_val;
				}
				else if (data_size < power1) {
					sprintf(return_val, " %ld bytes", data_size);
					return return_val;
				}
			}
			else if (data_size == 0)
				printf(" 0 bytes");
		}
	}
}

char *detailed_output(DList *to_copy_list, int output, char *what_is_copied, int fd)
{
	struct options_menu options;				// data structure used to set options
	extern struct Data_Copy_Info data_copy_info;
	struct copied_or_not copied;
	char *bytes = "bytes";
	char *location = "location";
	char *new_location = "new_location";

#define FP_SPECIAL 1

	DListElmt *to_copy;

	if (output == SCREEN) {
		printf("\n%s\n", what_is_copied);
		for (to_copy = to_copy_list->head; to_copy != NULL; to_copy = to_copy->next) {
			printf("%c%c%c%c%c%c%c%c%c     ", (to_copy->st_mode & S_IRUSR) ? 'r' : '-', (to_copy->st_mode & S_IWUSR) ? 'w' : '-', (to_copy->st_mode & S_IXUSR) ?
					(((to_copy->st_mode & S_ISUID) && (to_copy->st_mode & FP_SPECIAL)) ? 's' : 'x') : 
					(((to_copy->st_mode & S_ISUID) && (to_copy->st_mode & FP_SPECIAL)) ? 'S' : '-'),
					(to_copy->st_mode & S_IRGRP) ? 'r' : '-', (to_copy->st_mode & S_IWGRP) ? 'w' : '-', (to_copy->st_mode & S_IXGRP) ?
					(((to_copy->st_mode & S_ISGID) && (to_copy->st_mode & FP_SPECIAL)) ? 's' : 'x') : 
					(((to_copy->st_mode & S_ISGID) && (to_copy->st_mode & FP_SPECIAL)) ? 'S' : '-'),
					(to_copy->st_mode & S_IROTH) ? 'r' : '-', (to_copy->st_mode & S_IWOTH) ? 'w' : '-', (to_copy->st_mode & S_IXOTH) ?
					(((to_copy->st_mode & S_ISVTX) && (to_copy->st_mode & FP_SPECIAL)) ? 't' : 'x') : 
					(((to_copy->st_mode & S_ISVTX) && (to_copy->st_mode & FP_SPECIAL)) ? 'T' : '-'));
			printf("%s:     %s     %ld bytes \nlocation: %s     new location: %s\n\n", 
				to_copy->name, to_copy->size ? calc_size(to_copy->size, options.other_unit, AS_RETURN_VAL, 0) : 
				" ", to_copy->size, to_copy->dir_location, to_copy->new_location);
		}
	}
	else if (output == TO_FILE) {
		dprintf(fd, "\n%s\n", what_is_copied);
		for (to_copy = to_copy_list->head; to_copy != NULL; to_copy = to_copy->next) {
			dprintf(fd, "%c%c%c%c%c%c%c%c%c     ", (to_copy->st_mode & S_IRUSR) ? 'r' : '-', (to_copy->st_mode & S_IWUSR) ? 'w' : '-', (to_copy->st_mode & S_IXUSR) ?
					(((to_copy->st_mode & S_ISUID) && (to_copy->st_mode & FP_SPECIAL)) ? 's' : 'x') : 
					(((to_copy->st_mode & S_ISUID) && (to_copy->st_mode & FP_SPECIAL)) ? 'S' : '-'),
					(to_copy->st_mode & S_IRGRP) ? 'r' : '-', (to_copy->st_mode & S_IWGRP) ? 'w' : '-', (to_copy->st_mode & S_IXGRP) ?
					(((to_copy->st_mode & S_ISGID) && (to_copy->st_mode & FP_SPECIAL)) ? 's' : 'x') : 
					(((to_copy->st_mode & S_ISGID) && (to_copy->st_mode & FP_SPECIAL)) ? 'S' : '-'),
					(to_copy->st_mode & S_IROTH) ? 'r' : '-', (to_copy->st_mode & S_IWOTH) ? 'w' : '-', (to_copy->st_mode & S_IXOTH) ?
					(((to_copy->st_mode & S_ISVTX) && (to_copy->st_mode & FP_SPECIAL)) ? 't' : 'x') : 
					(((to_copy->st_mode & S_ISVTX) && (to_copy->st_mode & FP_SPECIAL)) ? 'T' : '-'));
			dprintf(fd, "%s:     %s     %ld bytes \nlocation: %s     new location: %s\n\n", 
				to_copy->name, to_copy->size ? calc_size(to_copy->size, options.other_unit, AS_RETURN_VAL, 0) : 
				" ", to_copy->size, to_copy->dir_location, to_copy->new_location);
		}
	}
}

void print_errors(void)
{
	extern struct errors_data errors;

	printf("Errors: ");
	printf("file open: %ld file create: %ld file close: %ld file delete: %ld"
		" directory open: %ld directory close: %ld directory delete: %ld"
		" file read: %ld file write: %ld file overwrite: %ld\n directory read: %ld directory create: %ld"
		"file attribute: %ld directory attribute: %ld symlink read: %ld symlink write: %ld symlink overwrite: %ld symlink delete: %ld"
		"access timestamp: %ld modification timestamp: %ld extended attribute: %ld mac attribute: %ld\n",
	errors.file_open_error_count,
	errors.file_create_error_count,
	errors.file_close_error_count,
	errors.file_delete_error_count,
	errors.dir_open_error_count,
	errors.dir_close_error_count,
	errors.dir_delete_error_count,
	errors.file_read_error_count,
	errors.file_write_error_count,
	errors.file_overwrite_error_count,
	errors.dir_read_error_count,
	errors.dir_create_error_count,
	errors.file_attr_error_count,
	errors.dir_attr_error_count,
	errors.symlink_read_error_count,
	errors.symlink_write_error_count,
	errors.symlink_overwrite_error_count,
	errors.symlink_delete_error_count,
	errors.atimestamp_error_count,
	errors.mtimestamp_error_count,
	errors.xattr_error_count,
	errors.mac_error_count);
}
