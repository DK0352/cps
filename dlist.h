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

/* doubly-linked list elements */
typedef struct DListElmt_ {
	char 				*name;		// name of a file or directory
	mode_t				st_mode;	// type and permissions
	unsigned long 			size;		// size of a file
	char				*dir_location;	// location of a file
	int				match;		// used in loop_files and loop_dirs function to signal that files has been compared
	char				*new_location;	// new location for a file in case it needs to be copied to a new location
	time_t				atime;		// access time
	time_t				mtime;		// modification time
	struct DList_of_lists_		*tree_position;	// position in the file tree. (dlist of lists). used to refer back to it from the file or dir. lists to copy.
	struct DListElmt_		*prev;
	struct DListElmt_		*next;
} DListElmt;

/* doubly-linked lists */
typedef struct DList_ {
	int		num;				// number of files in a directory
	long		files_size;			// size of all files in a directory
	DListElmt	*head;
	DListElmt	*tail;
} DList;

/* linked list of linked lists */
typedef struct DList_of_lists_ {
	unsigned long			file_num;		// number of files
	unsigned long			subdir_file_num;	// number of files in all subdirectories
	unsigned long			complete_file_num;	// complete file number for a directory including subdirectories
	unsigned long 			files_size;		// size of all files in the current directory 
	unsigned long 			complete_files_size;	// size of all files in all subdirectories
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
	char				*source_pathname;	// source directory - is this needed?
	char				*destination_pathname;	// destination directory - is this needed?
	char				*dirname;		// name of directory
	char				*dir_location;		// directory location
	DList				*files;			// linked list of files
	DList				*directories;		// linked list of directories
	// hej, pa ovo je sad u data_copy_info??? obrisat? ili nešt korisno izvuć iz toga?
	/*DList				*files_to_copy;		// files to copy
	DList				*dirs_to_copy;		// directories to copy
	DList				*files_surplus;		// file surplus
	DList				*dirs_surplus;		// directories surplus
	DList				*diff_type;		// same name, different type
	DList				*diff_size;		// same name, different size
	DList				*diff_size_ml;		// same name, different size, main larger
	DList				*diff_size_ms;		// same name, different size, main smaller
	// odavde
	// */
	struct DList_of_lists_		*file_tree_top_dir;	// top directory
	struct DList_of_lists_		*one_of_the_top_dirs;	// points to the top subdirectory of the file tree
	struct DList_of_lists_		*first_dir_in_chain;	// first directory in a list of directories in a file tree
	struct DList_of_lists_		*last_dir_in_chain;	// last directory in a list of directories in a file tree
	struct DList_of_lists_		*up;			// to go up the directory hierarchy
	struct DList_of_lists_		*down;			// to go down the directory hierarchy
	struct DList_of_lists_		*next;			// next directory in a list
	struct DList_of_lists_		*prev;			// previous directory in a list
} DList_of_lists;

struct thread_struct {
	char			*id;		/* source or destination pathname */
	char			*directory;	/* location */
	DList			*files;		/* list of files */
	DList			*directories;	/* list of directories */
	DList			*sym_links;
	int			(*file_function)(void *data1, void *data2); /* stat or lstat function depending on option*/
	DList_of_lists		*file_tree, *file_tree_top_dir;	/* file tree elements used in build_tree() and compare_trees() functions */
	struct thread_struct 	*other_thread_data;
};

void dlist_init(DList *list);
void dlist_destroy(DList *list);
int dlist_ins_next(DList *list, DListElmt *element, char *name, mode_t perm, long size, char *dir_location, int match, char *new_location, time_t atime, time_t ctime);
int dlist_remove(DList *list, DListElmt *element, char **name, char **dir_location, char **new_location);
