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
        DList		*dirs_to_copy_list;	// linked list of directories to copy
        DList		*files_surplus_list;	// linked list of surplus files
        DList		*dirs_surplus_list;	// linked list of surplus directories
        DList		*diff_size_ms_list;	// linked list of files with different size, one in the main (source directory) location smaller
        DList		*diff_size_ml_list;	// linked list of files with different size, ine in the main (source directory) location larger
        DList		*diff_type_list_main;	// linked list of files with the same name, but different type, main (source directory) location
        DList		*diff_type_list_secondary;	// linked list of files with the same name, but different type, secondary (destination directory) location
        unsigned long   global_files_to_copy_num;       // number of files to copy
        unsigned long	global_files_to_copy_size;      // size of files to copy in bytes
        unsigned long	global_dirs_to_copy_num;        // number of directories to copy
        unsigned long	global_dirs_to_copy_size;       // size of directories to copy in bytes
        unsigned long	global_files_surplus_num;       // number of surplus files 
        unsigned long	global_files_surplus_size;      // size of surplus files in bytes
        unsigned long	global_dirs_surplus_num;        // number of surplus directories
        unsigned long	global_dirs_surplus_size;       // size of all surplus directories in bytes
        unsigned long 	global_diff_type_num_main;              // number of files with the same name, but different type, source directory
        unsigned long	global_diff_type_size_main;             // size of all files with the same name, but different type in bytes, source directory
        unsigned long 	global_diff_type_num_secondary;         // number of files with the same name, but different type, destination directory
        unsigned long	global_diff_type_size_secondary;        // size of files with the same name, but different type, destination directory
        unsigned long 	global_diff_size_ms_num;        // number of files with the same name, but different size, source location smaller
        unsigned long	global_diff_size_ms_size;       // size of files with the same name, but different size, source location smaller
        unsigned long 	global_diff_size_ml_num;        // number of files with the same name, but different size, source location larger
        unsigned long	global_diff_size_ml_size;       // number of files with the same name, but different size, source location larger
        unsigned long 	global_dir_num_a;               // complete number of directories in the source directory
        unsigned long 	global_dir_num_b;               // complete number of directories in the destination directory
        unsigned long 	global_file_num_a;              // complete number of files in the source directory
        unsigned long 	global_file_num_b;              // complete number of files in the destinaton directory
        unsigned long	global_files_size_a;            // complete size of files in the source directory
        unsigned long	global_files_size_b;            // complete size of files in the destinaton directory
	unsigned long 	global_sym_links_num_a;		// number of symbolic links in the source directory
	unsigned long 	global_sym_links_num_b;		// number of symbolic links in the destination directory
	unsigned long 	global_sym_links_size_a;	// size of symbolic links in the source directory
	unsigned long 	global_sym_links_size_b;	// number of symbolic links in the destination directory
};
