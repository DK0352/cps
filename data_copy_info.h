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

// linked lists of data to copy and statistical information
struct Data_Copy_Info {
        struct DList_of_lists_  *file_tree_top_dir_a;
        struct DList_of_lists_  *file_tree_top_dir_b;
        DList		*files_to_copy_list;	// linked list of files to copy
        DList		*symlinks_to_copy_list;	// linked list of symbolic links to copy
        DList		*dirs_to_copy_list;	// linked list of directories to copy
        DList		*files_extraneous_list;	// linked list of extraneous files
        DList		*symlinks_extraneous_list;	// linked list of extraneous symbolic links
        DList		*dirs_extraneous_list;	// linked list of extraneous directories
        DList		*diff_size_ms_list;	// linked list of files with different size, one in the main (source directory) location smaller
        DList		*diff_size_ml_list;	// linked list of files with different size, ine in the main (source directory) location larger
	DList		*diff_time_mn_list;	// linked list of files with newer modification time from the main (source) directory
	DList		*diff_time_mo_list;	// linked list of files with older modification time from the main (source) directory
        DList		*symlinks_diff_size_ms_list;	// linked list of files with different size, one in the main (source directory) location smaller
        DList		*symlinks_diff_size_ml_list;	// linked list of files with different size, ine in the main (source directory) location larger
	DList		*symlinks_diff_time_mn_list;	// linked list of files with newer modification time from the main (source) directory
	DList		*symlinks_diff_time_mo_list;	// linked list of files with older modification time from the main (source) directory

        unsigned long   global_files_to_copy_num;       // number of files to copy
        unsigned long	global_files_to_copy_size;      // size of files to copy in bytes
	unsigned long	global_files_within_dirs_to_copy_num;		// number of files within directories to copy 
	unsigned long	global_files_within_dirs_extraneous_num;	// number of files within directories to copy back
	unsigned long	global_symlinks_within_dirs_to_copy_num;	// number of symlinks within directories to copy 
	unsigned long	global_symlinks_within_dirs_extraneous_num;	// number of symlinks within directories to copy back
        unsigned long   global_symlinks_to_copy_num;       // number of symbolic links to copy
        unsigned long	global_symlinks_to_copy_size;      // size of symbolic links to copy in bytes
        unsigned long	global_dirs_to_copy_num; 	   // number of directories to copy
        unsigned long	global_dirs_to_copy_size;      	   // size of directories to copy in bytes
	unsigned long	global_subdirs_to_copy_num;
	unsigned long	global_subdirs_to_copy_size;
        unsigned long	global_files_extraneous_num;       // number of extraneous files
        unsigned long	global_files_extraneous_size;      // size of extraneous files in bytes
        unsigned long	global_symlinks_extraneous_num;    // number of extraneous symbolic links
        unsigned long	global_symlinks_extraneous_size;   // size of extraneous symbolic links in bytes
        unsigned long	global_dirs_extraneous_num;        // number of extraneous directories
        unsigned long	global_dirs_extraneous_size;        // size of extraneous directories
        unsigned long	global_subdirs_extraneous_num;     // number of extraneous subdirectories
        unsigned long	global_subdirs_extraneous_size;    // size of all extraneous subdirectories in bytes

        unsigned long 	global_diff_size_ms_num;        // number of files with the same name, but different size, source location smaller
        unsigned long	global_diff_size_ms_size;       // size of files with the same name, but different size, source location smaller
	unsigned long	global_diff_size_ms_orig_size;	// size of files with the same name, but different size, source location smaller, but this one is for the original file
        unsigned long 	global_diff_size_ml_num;        // number of files with the same name, but different size, source location larger
        unsigned long	global_diff_size_ml_size;       // number of files with the same name, but different size, source location larger
	unsigned long	global_diff_size_ml_orig_size;	// size of files with the same name, but different size, source location larger, but this one is for the original file
	unsigned long	global_diff_time_mn_num;
	unsigned long	global_diff_time_mn_size;
	unsigned long	global_diff_time_mn_orig_size;
	unsigned long	global_diff_time_mo_num;
	unsigned long	global_diff_time_mo_size;
	unsigned long	global_diff_time_mo_orig_size;

        unsigned long 	global_diff_symlinks_size_ms_num;        // number of symlinks with the same name, but different size, source location smaller
        unsigned long	global_diff_symlinks_size_ms_size;       // size of symlinks with the same name, but different size, source location smaller
	unsigned long	global_diff_symlinks_size_ms_orig_size;	// size of symlinks with the same name, but different size, source location smaller, but this one is for the original file
        unsigned long 	global_diff_symlinks_size_ml_num;        // number of symlinks with the same name, but different size, source location larger
        unsigned long	global_diff_symlinks_size_ml_size;       // number of symlinks with the same name, but different size, source location larger
	unsigned long	global_diff_symlinks_size_ml_orig_size;	// size of symlinks with the same name, but different size, source location larger, but this one is for the original file
	unsigned long	global_diff_symlinks_time_mn_num;
	unsigned long	global_diff_symlinks_time_mn_size;
	unsigned long	global_diff_symlinks_time_mn_orig_size;
	unsigned long	global_diff_symlinks_time_mo_num;
	unsigned long	global_diff_symlinks_time_mo_size;
	unsigned long	global_diff_symlinks_time_mo_orig_size;

        unsigned long 	global_dir_num_a;               // complete number of directories in the source directory
        unsigned long 	global_dir_num_b;               // complete number of directories in the destination directory
        unsigned long 	global_file_num_a;              // complete number of files in the source directory
        unsigned long 	global_file_num_b;              // complete number of files in the destinaton directory
        unsigned long	global_files_size_a;            // complete size of files in the source directory
        unsigned long	global_files_size_b;            // complete size of files in the destinaton directory
	unsigned long 	global_symlink_num_a;		// number of symbolic links in the source directory
	unsigned long 	global_symlink_num_b;		// number of symbolic links in the destination directory
	unsigned long 	global_symlink_size_a;		// size of symbolic links in the source directory
	unsigned long 	global_symlink_size_b;		// number of symbolic links in the destination directory

	unsigned long	ac_file_num_a;		// after copying number of files (source)
	unsigned long	ac_file_num_b;		// after copying number of files (destination)
	unsigned long	ac_files_size_a;
	unsigned long	ac_files_size_b;
	unsigned long	ac_symlink_num_a;		// after copying number of symlinks (source)
	unsigned long	ac_symlink_num_b;		// after copying number of symlinks (destination)
	unsigned long	ac_symlinks_size_a;		// after copying size of symlinks (source)
	unsigned long	ac_symlinks_size_b;		// after copying size of symlinks (destination)
	unsigned long	ac_dir_num_a;		// after copying number of directories (source)
	unsigned long	ac_dir_num_b;		// after copying number of directories (destination)
};

struct copied_or_not {
	int copied_files;
	int copied_symlinks;
	int copied_directories;
	int aborted_copying;		// if 1, user aborted copying missing files and dirs
	int copied_files_extraneous;
	int copied_symlinks_extraneous;
	int copied_directories_extraneous;
	int deleted_files_extraneous;
	int deleted_symlinks_extraneous;
	int deleted_directories_extraneous;
	int ow_smaller;
	int ow_larger;
	int ow_newer;
	int ow_older;
	int ow_symlinks_main_smaller;
	int ow_symlinks_main_larger;
	int ow_symlinks_main_newer;
	int ow_symlinks_main_older;
	int full_dir1_copied;		// if 1, add size in stats
	int full_dir2_copied;		// if 1, add size in stats
};
