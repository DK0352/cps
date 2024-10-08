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

DList_of_lists *create_dirs(DList *list, DList_of_lists *file_tree_element, DList_of_lists *file_tree_top_dir);
DList_of_lists *create_top_dirs(DList *list, DList_of_lists *file_tree_element);
void extract_size(DList_of_lists *file_tree_element);
void build_rest_of_the_tree(struct thread_struct *thread_data, unsigned long *p_global_file_size, unsigned long *p_global_dir_size, unsigned long *p_global_files_size,
unsigned long *p_global_sym_links_num, unsigned long *p_global_sym_links_size);
int open_dirs(struct thread_struct *thread_data);
void init_to_zero(DList_of_lists *file_tree_element);

int no_files_and_dirs_a = 0; // external variable that signals that the source directory is empty
int no_files_and_dirs_b = 0; // external variable that signals that the destination directory is empty
int no_dirs_a = 0;	// don't start the loop to create/compare directories if there aren't any
int no_dirs_b = 0;	// don't start the loop to create/compare directories if there aren't any

/* builds file trees for the source and destination directory. */
void build_tree(struct thread_struct *thread_data)
{
	DList_of_lists 			*file_tree_top_dir, *file_tree_element;
	DList_of_lists			*one_of_the_top_dirs;
	DList				*files, *directories, *sym_links;

	extern struct Data_Copy_Info 	data_copy_info;

	char				*locate_char;	// to find the last "/" in the pathname with strrchr
	int				locate_char_size; // strlen size for locate char string
	int				len;
	unsigned long			*p_global_file_num, *p_global_dir_num, *p_global_sym_links_num;
	unsigned long			*p_global_files_size, *p_global_sym_links_size;

	file_tree_top_dir = malloc(sizeof(DList_of_lists));
	if (file_tree_top_dir == NULL) {
		printf("build_tree(): malloc() error 1.\n");
		exit(1);
	}
	init_to_zero(file_tree_top_dir);
	thread_data->file_tree_top_dir = file_tree_top_dir;
	file_tree_element = file_tree_top_dir;
	file_tree_element->file_tree_top_dir = file_tree_top_dir;
	file_tree_element->this_is_top_dir = 1;

	// need to separete the pathname from the directory behind the last '/' (slash)
	if (strcmp(thread_data->id,"source") == 0) {
		len = strlen(thread_data->directory) + 1; // +1 for '\0'
		file_tree_element->source_pathname = malloc(len);
		if (file_tree_element->source_pathname == NULL) {
			printf("build_tree(): malloc() error 2.\n");
			exit(1);
		}
		else {
			strcpy(file_tree_element->source_pathname,thread_data->directory);
			locate_char = strrchr(file_tree_element->source_pathname,'/');
			locate_char++;	// move one position behind '/'
			if (locate_char != NULL) {
				locate_char_size = strlen(locate_char);
				file_tree_element->dirname = malloc(locate_char_size);
				if (file_tree_element->dirname == NULL) {
					printf("build_tree(): malloc() error 3.\n");
					exit(1);
				}
				strcpy(file_tree_element->dirname,locate_char);
				file_tree_element->dir_location = strdup(file_tree_element->source_pathname); /* alocirana imena i lokacije za stablo direktorija koje stvaram */
				if (file_tree_element->dir_location == NULL) {
					printf("build_tree(): strdup() error 1.\n");
					exit(1);
				}
			}
		}
		file_tree_element->destination_pathname = NULL;
	}
	else if (strcmp(thread_data->id,"destination") == 0) {
		len = strlen(thread_data->directory) + 1;
		file_tree_element->destination_pathname = malloc(len);
		if (file_tree_element->destination_pathname == NULL) {
			printf("build_tree(): malloc() error 4.\n");
			exit(1);
		}
		else {
			strcpy(file_tree_element->destination_pathname,thread_data->directory);
			locate_char = strrchr(file_tree_element->destination_pathname,'/');
			locate_char++;	// move one position behind '/'
			if (locate_char != NULL) {
				locate_char_size = strlen(locate_char);
				file_tree_element->dirname = malloc(locate_char_size);
				if (file_tree_element->dirname == NULL) {
					printf("build_tree(): malloc() error 5.\n");
					exit(1);
				}
				strcpy(file_tree_element->dirname,locate_char);
				file_tree_element->dir_location = strdup(file_tree_element->destination_pathname); /* alocirana imena i lokacije za stablo direktorija koje stvaram */
				if (file_tree_element->dir_location == NULL) {
					printf("build_tree(): strdup() error 2.\n");
					exit(1);
				}
			}
		}
		file_tree_element->source_pathname = NULL;
	}
	if (strcmp(thread_data->id,"source") == 0) {
		p_global_file_num = &data_copy_info.global_file_num_a;
		p_global_sym_links_num = &data_copy_info.global_symlink_num_a;
		p_global_dir_num = &data_copy_info.global_dir_num_a;
		p_global_files_size = &data_copy_info.global_files_size_a;
		p_global_sym_links_size = &data_copy_info.global_symlink_size_a;
	}
	else if (strcmp(thread_data->id,"destination") == 0) {
		//data_copy_info.file_tree_top_dir_b = file_tree_top_dir; // ?
		p_global_file_num = &data_copy_info.global_file_num_b;
		p_global_sym_links_num = &data_copy_info.global_symlink_num_b;
		p_global_dir_num = &data_copy_info.global_dir_num_b;
		p_global_files_size = &data_copy_info.global_files_size_b;
		p_global_sym_links_size = &data_copy_info.global_symlink_size_b;
	}

	files = thread_data->files;
	sym_links = thread_data->sym_links;
	directories = thread_data->directories;

	/* files only, top dir */
	if (files != NULL && sym_links == NULL && directories == NULL) {
		file_tree_element->files = files;
		file_tree_element->sym_links = NULL;
		file_tree_element->directories = NULL;
		file_tree_element->files_size += files->files_size;
		file_tree_element->file_num += files->num;
		file_tree_element->complete_file_num += files->num;
		file_tree_element->complete_dir_size += files->files_size;
		*p_global_files_size += files->files_size;
		*p_global_file_num += files->num;
		thread_data->file_tree = file_tree_element;
		thread_data->file_tree_top_dir = file_tree_element;
		if (strcmp(thread_data->id,"source") == 0)
			no_dirs_a = 1;
		else if (strcmp(thread_data->id,"destination") == 0)
			no_dirs_b = 1;
	}
	/* symbolic links only, top dir */
	else if (files == NULL && sym_links != NULL && directories == NULL) {
		file_tree_element->files = NULL;
		file_tree_element->sym_links = sym_links;
		file_tree_element->directories = NULL;
		file_tree_element->sym_links_size += sym_links->files_size;
		file_tree_element->sym_links_num += sym_links->num;
		file_tree_element->complete_sym_links_num += sym_links->num;
		file_tree_element->complete_dir_size += sym_links->files_size;
		*p_global_sym_links_num += sym_links->num;
		*p_global_sym_links_size += sym_links->files_size;
		thread_data->file_tree = file_tree_element;
		thread_data->file_tree_top_dir = file_tree_element;
		if (strcmp(thread_data->id,"source") == 0)
			no_dirs_a = 1;
		else if (strcmp(thread_data->id,"destination") == 0)
			no_dirs_b = 1;
	}
	/* directories only, top dir */
	else if (files == NULL && sym_links == NULL && directories != NULL) {
		file_tree_element->files = NULL;
		file_tree_element->sym_links = NULL;
		file_tree_element->directories = thread_data->directories;
		*p_global_dir_num += directories->num;
		file_tree_element->dir_num += directories->num;
		file_tree_element->complete_dir_num += directories->num;
		file_tree_element->subdir_num += directories->num;
		file_tree_element = create_top_dirs(file_tree_element->directories,file_tree_element);
	}
	/* files and sym_links, top dir */
	else if (files != NULL && sym_links != NULL && directories == NULL) {
		file_tree_element->files = files;
		file_tree_element->files_size += files->files_size;
		file_tree_element->file_num += files->num;
		file_tree_element->complete_file_num += files->num;
		file_tree_element->complete_dir_size += files->files_size;
		*p_global_file_num += files->num;
		*p_global_files_size += files->files_size;
		file_tree_element->sym_links = sym_links;
		file_tree_element->sym_links_size += sym_links->files_size;
		file_tree_element->sym_links_num += sym_links->num;
		file_tree_element->complete_sym_links_num += sym_links->num;
		file_tree_element->complete_dir_size += sym_links->files_size;
		*p_global_sym_links_num += sym_links->num;
		*p_global_sym_links_size += sym_links->files_size;
		file_tree_element->directories = NULL;
		thread_data->file_tree = file_tree_element;
		thread_data->file_tree_top_dir = file_tree_element;
		if (strcmp(thread_data->id,"source") == 0)
			no_dirs_a = 1;
		else if (strcmp(thread_data->id,"destination") == 0)
			no_dirs_b = 1;
	}
	/* files and directories, top dir */
	else if (files != NULL && sym_links == NULL && directories != NULL) {
		file_tree_element->files = files;
		file_tree_element->sym_links = NULL;
		file_tree_element->directories = directories;
		file_tree_element->files_size += files->files_size;
		file_tree_element->complete_dir_size += files->files_size;
		file_tree_element->file_num += files->num;
		file_tree_element->complete_file_num += files->num;
		file_tree_element->dir_num += directories->num;
		file_tree_element->complete_dir_num += directories->num;
		file_tree_element->subdir_num += directories->num;
		*p_global_file_num += files->num;
		*p_global_files_size += files->files_size;
		*p_global_dir_num += directories->num;
		file_tree_element = create_top_dirs(file_tree_element->directories,file_tree_element);
	}
	/* sym_links and directories, top dir */
	else if (files == NULL && sym_links != NULL && directories != NULL) {
		file_tree_element->files = NULL;
		file_tree_element->sym_links = sym_links;
		file_tree_element->directories = directories;
		file_tree_element->sym_links_size += sym_links->files_size;
		file_tree_element->complete_dir_size += sym_links->files_size;
		file_tree_element->sym_links_num += sym_links->num;
		file_tree_element->complete_sym_links_num += sym_links->num;
		file_tree_element->dir_num += directories->num;
		file_tree_element->complete_dir_num += directories->num;
		file_tree_element->subdir_num += directories->num;
		*p_global_sym_links_num += sym_links->num;
		*p_global_sym_links_size += sym_links->files_size;
		*p_global_dir_num += directories->num;
		file_tree_element = create_top_dirs(file_tree_element->directories,file_tree_element);
	}
	/* files, sym_links and directories, top dir */
	else if (files != NULL && sym_links != NULL && directories != NULL) {
		file_tree_element->files = files;
		file_tree_element->directories = directories;
		file_tree_element->sym_links = sym_links;
		file_tree_element->files_size += files->files_size;
		file_tree_element->complete_dir_size += files->files_size;
		file_tree_element->sym_links_size += sym_links->files_size;
		file_tree_element->file_num += files->num;
		file_tree_element->sym_links_num += sym_links->num;
		file_tree_element->complete_file_num += files->num;
		file_tree_element->complete_sym_links_num += sym_links->num;
		file_tree_element->dir_num += directories->num;
		file_tree_element->complete_dir_num += directories->num;
		file_tree_element->subdir_num += directories->num;
		*p_global_file_num += files->num;
		*p_global_files_size += files->files_size;
		*p_global_sym_links_num += sym_links->num;
		*p_global_sym_links_size += sym_links->files_size;
		*p_global_dir_num += directories->num;
		file_tree_element = create_top_dirs(file_tree_element->directories,file_tree_element);
	}
	/* empty directory, top dir */
	else if (files == NULL && sym_links == NULL && directories == NULL) {
		if (strcmp(thread_data->id,"source") == 0)
			no_files_and_dirs_a = 1;
		else if (strcmp(thread_data->id,"destination") == 0)
			no_files_and_dirs_b = 1;
	}

	/* loop that builds the file tree, calling the build_rest_of_the_tree function recursively */
	if (strcmp(thread_data->id,"source") == 0) {
		if (no_dirs_a != 1) {
			for (one_of_the_top_dirs = file_tree_element; one_of_the_top_dirs != NULL; one_of_the_top_dirs = one_of_the_top_dirs->next) {
				thread_data->directory = one_of_the_top_dirs->dir_location;
				thread_data->file_tree = one_of_the_top_dirs;
				open_dirs(thread_data);
				build_rest_of_the_tree(thread_data,p_global_file_num,p_global_dir_num,p_global_files_size,p_global_sym_links_num,p_global_sym_links_size);
				if (one_of_the_top_dirs->last_dir == 1)
					extract_size(one_of_the_top_dirs);
			}
		}
	}
	else if (strcmp(thread_data->id,"destination") == 0) {
		if (no_dirs_b != 1) {
			for (one_of_the_top_dirs = file_tree_element; one_of_the_top_dirs != NULL; one_of_the_top_dirs = one_of_the_top_dirs->next) {
				thread_data->directory = one_of_the_top_dirs->dir_location;
				thread_data->file_tree = one_of_the_top_dirs;
				open_dirs(thread_data);
				build_rest_of_the_tree(thread_data,p_global_file_num,p_global_dir_num,p_global_files_size,p_global_sym_links_num,p_global_sym_links_size);
				if (one_of_the_top_dirs->last_dir == 1)
					extract_size(one_of_the_top_dirs);
			}
		}
	}
} // build_tree()

/* called from the each top directory to build the file tree underneath */
void build_rest_of_the_tree(struct thread_struct *thread_data, unsigned long *p_global_file_num, unsigned long *p_global_dir_num, unsigned long *p_global_files_size, 
unsigned long *p_global_sym_links_num, unsigned long *p_global_sym_links_size)
{
	DList_of_lists 		*file_tree_top_dir, *file_tree_element, *alloc, *save_up_position;
	DList			*files, *sym_links, *directories;

	files = thread_data->files;
	sym_links = thread_data->sym_links;
	directories = thread_data->directories;
	file_tree_element = thread_data->file_tree;

	/* files only */
	if (files != NULL && sym_links == NULL && directories == NULL) {
		save_up_position = file_tree_element;
		file_tree_element->files = files;
		file_tree_element->sym_links = NULL;
		file_tree_element->directories = NULL;
		*p_global_file_num += files->num;
		*p_global_files_size += files->files_size;
		file_tree_element->file_num += files->num;
		file_tree_element->complete_file_num += files->num;
		file_tree_element->files_size += files->files_size;
		file_tree_element->complete_dir_size += files->files_size;
	}
	/* sym_links only */
	else if (files == NULL && sym_links != NULL && directories == NULL) {
		save_up_position = file_tree_element;
		file_tree_element->files = NULL;
		file_tree_element->sym_links = sym_links;
		file_tree_element->directories = NULL;
		*p_global_sym_links_num += sym_links->num;
		*p_global_sym_links_size += sym_links->files_size;
		file_tree_element->sym_links_num += sym_links->num;
		file_tree_element->complete_sym_links_num += sym_links->num;
		file_tree_element->sym_links_size += sym_links->files_size;
		file_tree_element->complete_dir_size += sym_links->files_size;
	}
	/* directories only */
	else if (files == NULL && sym_links == NULL && directories != NULL) {
		save_up_position = file_tree_element;
		file_tree_element->files = NULL;
		file_tree_element->sym_links = NULL;
		file_tree_element->directories = directories;
		file_tree_element->dir_num += directories->num;
		file_tree_element->complete_dir_num += directories->num;
		*p_global_dir_num += directories->num;
		alloc = malloc(sizeof(DList_of_lists));
		if (alloc == NULL) {
			printf("built_the_rest_of_the_tree(): malloc() error 2.\n");
			exit(1);
		}
		init_to_zero(alloc);
		alloc->dirname = file_tree_element->dirname;
		alloc->dir_location = file_tree_element->dir_location;
		alloc->file_tree_top_dir = file_tree_element->file_tree_top_dir;
		file_tree_element->down = alloc;
		file_tree_element = file_tree_element->down;
		file_tree_element->up = save_up_position;
		file_tree_element->one_of_the_top_dirs = save_up_position->one_of_the_top_dirs;
		file_tree_element = create_dirs(directories,file_tree_element,file_tree_top_dir);
		for ( ; file_tree_element != NULL; file_tree_element = file_tree_element->next) {
			thread_data->directory = file_tree_element->dir_location;
			thread_data->file_tree = file_tree_element;
			open_dirs(thread_data);
			build_rest_of_the_tree(thread_data,p_global_file_num,p_global_dir_num,p_global_files_size,p_global_sym_links_num,p_global_sym_links_size);
			if (file_tree_element->last_dir == 1)
				extract_size(file_tree_element);
		}
	}
	/* files and sym_links */
	else if (files != NULL && sym_links != NULL && directories == NULL) {
		save_up_position = file_tree_element;
		file_tree_element->files = files;
		file_tree_element->sym_links = sym_links;
		file_tree_element->directories = NULL;
		*p_global_file_num += files->num;
		*p_global_files_size += files->files_size;
		file_tree_element->file_num += files->num;
		file_tree_element->complete_file_num += files->num;
		file_tree_element->files_size += files->files_size;
		file_tree_element->complete_dir_size += files->files_size;
		file_tree_element->sym_links = sym_links;
		*p_global_sym_links_num += sym_links->num;
		*p_global_sym_links_size += sym_links->files_size;
		file_tree_element->sym_links_num += sym_links->num;
		file_tree_element->complete_sym_links_num += sym_links->num;
		file_tree_element->sym_links_size += sym_links->files_size;
		file_tree_element->complete_dir_size += sym_links->files_size;
	}
	/* directories and files */
	else if (files != NULL && sym_links == NULL && directories != NULL) {
		save_up_position = file_tree_element;
		file_tree_element->files = files;	
		file_tree_element->sym_links = NULL;
		file_tree_element->directories = directories;	
		file_tree_element->files_size += files->files_size;
		file_tree_element->complete_dir_size += files->files_size;
		file_tree_element->file_num += files->num;
		file_tree_element->complete_file_num += files->num;
		file_tree_element->dir_num += directories->num;
		file_tree_element->complete_dir_num += directories->num;
		*p_global_file_num += files->num;
		*p_global_files_size += files->files_size;
		*p_global_dir_num += directories->num;
		alloc = malloc(sizeof(DList_of_lists));
		if (alloc == NULL) {
			printf("built_the_rest_of_the_tree(): malloc() error 3.\n");
			exit(1);
		}
		init_to_zero(alloc);
		alloc->dirname = file_tree_element->dirname;
		alloc->dir_location = file_tree_element->dir_location;
		file_tree_element->down = alloc;
		file_tree_element = file_tree_element->down;
		file_tree_element->up = save_up_position;
		file_tree_element->one_of_the_top_dirs = save_up_position->one_of_the_top_dirs;
		file_tree_element = create_dirs(directories,file_tree_element,file_tree_top_dir);
		for ( ; file_tree_element != NULL; file_tree_element = file_tree_element->next) {
			thread_data->directory = file_tree_element->dir_location;
			thread_data->file_tree = file_tree_element;
			open_dirs(thread_data);
			build_rest_of_the_tree(thread_data,p_global_file_num,p_global_dir_num,p_global_files_size,p_global_sym_links_num,p_global_sym_links_size);
			if (file_tree_element->last_dir == 1)
				extract_size(file_tree_element);
		}
	}
	/* directories and sym_links */
	else if (files == NULL && sym_links != NULL && directories != NULL) {
		save_up_position = file_tree_element;
		file_tree_element->files = NULL;
		file_tree_element->sym_links = sym_links;	
		file_tree_element->directories = directories;	
		file_tree_element->sym_links_size += sym_links->files_size;
		file_tree_element->complete_dir_size += sym_links->files_size;
		file_tree_element->sym_links_num += sym_links->num;
		file_tree_element->complete_sym_links_num += sym_links->num;
		file_tree_element->dir_num += directories->num;
		file_tree_element->complete_dir_num += directories->num;
		*p_global_sym_links_num += sym_links->num;
		*p_global_sym_links_size += sym_links->files_size;
		*p_global_dir_num += directories->num;
		alloc = malloc(sizeof(DList_of_lists));
		if (alloc == NULL) {
			printf("built_the_rest_of_the_tree(): malloc() error 3.\n");
			exit(1);
		}
		init_to_zero(alloc);
		alloc->dirname = file_tree_element->dirname;
		alloc->dir_location = file_tree_element->dir_location;
		file_tree_element->down = alloc;
		file_tree_element = file_tree_element->down;
		file_tree_element->up = save_up_position;
		file_tree_element->one_of_the_top_dirs = save_up_position->one_of_the_top_dirs;
		file_tree_element = create_dirs(directories,file_tree_element,file_tree_top_dir);
		for ( ; file_tree_element != NULL; file_tree_element = file_tree_element->next) {
			thread_data->directory = file_tree_element->dir_location;
			thread_data->file_tree = file_tree_element;
			open_dirs(thread_data);
			build_rest_of_the_tree(thread_data,p_global_file_num,p_global_dir_num,p_global_files_size,p_global_sym_links_num,p_global_sym_links_size);
			if (file_tree_element->last_dir == 1)
				extract_size(file_tree_element);
		}
	}
	/* files, sym_links and directories */
	else if (files != NULL && sym_links != NULL && directories != NULL) {
		save_up_position = file_tree_element;
		file_tree_element->files = files;	
		file_tree_element->sym_links = sym_links;	
		file_tree_element->directories = directories;	
		file_tree_element->files_size += files->files_size;
		file_tree_element->sym_links_size += sym_links->files_size;
		file_tree_element->complete_dir_size += files->files_size;
		file_tree_element->complete_dir_size += sym_links->files_size;
		file_tree_element->file_num += files->num;
		file_tree_element->sym_links_num += sym_links->num;
		file_tree_element->complete_file_num += files->num;
		file_tree_element->complete_sym_links_num += sym_links->num;
		file_tree_element->dir_num += directories->num;
		file_tree_element->complete_dir_num += directories->num;
		*p_global_file_num += files->num;
		*p_global_sym_links_num += sym_links->num;
		*p_global_files_size += files->files_size;
		*p_global_sym_links_size += sym_links->files_size;
		*p_global_dir_num += directories->num;
		alloc = malloc(sizeof(DList_of_lists));
		if (alloc == NULL) {
			printf("built_the_rest_of_the_tree(): malloc() error 3.\n");
			exit(1);
		}
		init_to_zero(alloc);
		alloc->dirname = file_tree_element->dirname;
		alloc->dir_location = file_tree_element->dir_location;
		file_tree_element->down = alloc;
		file_tree_element = file_tree_element->down;
		file_tree_element->up = save_up_position;
		file_tree_element->one_of_the_top_dirs = save_up_position->one_of_the_top_dirs;
		file_tree_element = create_dirs(directories,file_tree_element,file_tree_top_dir);
		for ( ; file_tree_element != NULL; file_tree_element = file_tree_element->next) {
			thread_data->directory = file_tree_element->dir_location;
			thread_data->file_tree = file_tree_element;
			open_dirs(thread_data);
			build_rest_of_the_tree(thread_data,p_global_file_num,p_global_dir_num,p_global_files_size,p_global_sym_links_num,p_global_sym_links_size);
			if (file_tree_element->last_dir == 1)
				extract_size(file_tree_element);
		}
	}
	/* empty directory */
	else if (files == NULL && sym_links == NULL && directories == NULL) {
		file_tree_element->down = NULL;
	}
} /* build_the_rest_of_the_tree */
