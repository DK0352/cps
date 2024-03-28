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

#include <time.h>
#include <fcntl.h>

typedef struct DListElmt_ DListElmt;
typedef struct DList_ DList;

/* linked list of linked lists */
typedef struct DList_of_lists_ {
	unsigned long			file_num;		// number of files
	unsigned long			subdir_file_num;	// number of files in all subdirectories
	unsigned long			subdir_sym_links_num;
	unsigned long			sym_links_num;		// number of symbolic links
	unsigned long			complete_file_num;	// complete file number of a directory including the subdirectories
	unsigned long			complete_sym_links_num;	// complete number of a symbolic links of a directory including the subdirectories
	unsigned long 			files_size;		// size of all files in the current directory
	unsigned long			sym_links_size;		// size of all symbolic links in the current directory
	unsigned long 			complete_files_size;	// size of all files in all subdirectories
	unsigned long			complete_sym_links_size; // size of all symbolic links in all subdirectories
	unsigned long			dir_num;		// number of directories in the current directory 
	unsigned long			subdir_num;		// number of all subdirectories in the current directory
	unsigned long			complete_dir_num;	// dir_num + subdir_num
	unsigned long 			subdirs_size;		// size of all subdirectories in the current directory
	unsigned long			complete_dir_size;	// size of all files and subdirectories in the current directory
	int				last_dir;		// last directory in the linked list chain
	int				found_dir_match;	// found two directories with the same name. used in loop_dirs and compare_trees functions.
	int				one_of_the_top_dirs_num; // number of the top subdirectory
	int				this_is_top_dir;	// mark that this is a top subdirectory
	mode_t				st_mode;		// type/permission
	time_t				atime;			// acccess time
	time_t				mtime;			// modification time
	char				*source_pathname;	// source directory -------------?
	char				*destination_pathname;	// destination directory ----------------?
	char				*dirname;		// name of directory
	char				*dir_location;		// directory location
	DList				*files;			// linked list of files
	DList				*directories;		// linked list of directories
	DList				*sym_links;		// linked list of symbolic links
	struct DList_of_lists_		*file_tree_top_dir;	// top directory
	struct DList_of_lists_		*one_of_the_top_dirs;	// points to the top subdirectory of the file tree
	struct DList_of_lists_		*first_dir_in_chain;	// first directory in a list of directories in a file tree
	struct DList_of_lists_		*last_dir_in_chain;	// last directory in a list of directories in a file tree
	struct DList_of_lists_		*up;			// to go up the directory hierarchy
	struct DList_of_lists_		*down;			// to go down the directory hierarchy
	struct DList_of_lists_		*next;			// next directory in a list
	struct DList_of_lists_		*prev;			// previous directory in a list
} DList_of_lists;

/* doubly-linked list elements */
typedef struct DListElmt_ {
	char 				*name;		// name of a file or directory
	mode_t				st_mode;	// type and permissions
	unsigned long 			size;		// size of a file
	char				*dir_location;	// location of a file
	int				match;		// used in loop_files function to signal that file has been compared
	char				*new_location;	// new location for the file
	time_t				atime;		// access time
	time_t				mtime;		// modification time
	struct DList_of_lists_		*tree_position;	// position in the file tree. (dlist of lists). used to refer back to it from the file or dir. lists to copy.
	struct DListElmt_		*prev;
	struct DListElmt_		*next;
} DListElmt;

/* doubly-linked lists */
typedef struct DList_ {
	unsigned long	num;				// number of files in a directory
	unsigned long	files_size;			// size of all files in a directory
	DListElmt	*head;
	DListElmt	*tail;
} DList;

struct thread_struct {
	char			*id;		/* source or destination pathname */
	char			*directory;	/* location */
	DList			*files;		/* list of files */
	DList			*directories;	/* list of directories */
	DList			*sym_links;	/* list of symbolic links */
	int			(*file_function)(void *data1, void *data2); /* stat or lstat function depending on option*/
	DList_of_lists		*file_tree, *file_tree_top_dir;	/* file tree elements used in build_tree() and compare_trees() functions */
	struct thread_struct 	*other_thread_data;
};

void dlist_init(DList *list);
void dlist_destroy(DList *list);
void dlist_destroy_2(DList *list);
void dlist_destroy_3(DList *list);
int dlist_ins_next(DList *list, DListElmt *element, char *name, mode_t perm, long size, char *dir_location, int match, char *new_location, time_t atime, time_t ctime, DList_of_lists *tree_position);
int dlist_remove(DList *list, DListElmt *element, char **name, char **dir_location, char **new_location);
