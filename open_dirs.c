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
#include "errors.h"
#include "options.h"

int open_dirs(struct thread_struct *thread_data)
{
	extern struct Data_Copy_Info data_copy_info;
	extern struct options_menu options;
	DIR *dir;
	struct dirent *dir_entry;
	struct stat *file_t;
	char *name, *location;
	int path_len, name_len, complete_size;
	long size;
	int data5_val;
	int dir_entry_init, file_t_init;
	struct errors_data error;

	dir_entry_init = 0;
	file_t_init = 0;
	data5_val = 0;

	errno = 0;
	dir = opendir(thread_data->directory);
	if (dir == NULL) {
		if (errno == EACCES) {
			if (options.quit_read_errors != 0) {
				printf("open_dirs(): ");
				perror("opendir");
				printf("dirname: %s\n", thread_data->directory);
				printf("exiting the program.\n");
				exit(1);
			}
			else {
				thread_data->directories = NULL;
				thread_data->files = NULL;
				return 0;
			}
		}
		perror("opendir");
		printf("dirname: %s\n", thread_data->directory);
		if (options.quit_read_errors != 0) {
			printf("exiting the program.\n");
			exit(1);
		}
		thread_data->directories = NULL;
		thread_data->files = NULL;
		return 0;
	}
	thread_data->files = malloc(sizeof(DList));
	if (thread_data->files == NULL) {
		printf("open_dirs(): malloc error 1\n");
		exit(1);
	}
	thread_data->directories = malloc(sizeof(DList));
	if (thread_data->directories == NULL) {
		printf("open_dirs(): malloc error 2\n");
		exit(1);
	}

	dlist_init(thread_data->files);
	dlist_init(thread_data->directories);

	for (;;) {
		if (dir_entry_init != 1) {
			dir_entry = malloc(sizeof(struct dirent));
			if (dir_entry == NULL) {
				printf("open_dirs(): malloc() dir_entry list error. exiting.\n");
				exit(1);
			}
			dir_entry_init = 1;
		}
		errno = 0;
		dir_entry = readdir(dir);
		if (dir_entry == NULL) {
			if (errno == 0) {
				break;
			}
		}
		if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
			continue;
		// name_len
		name_len = strlen(dir_entry->d_name)+1; // +1 za '\0'
		name = malloc(name_len);
		if (name == NULL) {
			printf("open_dirs(): malloc error 3\n");
			exit(1);
		}
		strcpy(name,dir_entry->d_name);
		// path_len
		path_len = strlen(thread_data->directory);
		// complete_size (or complete len to be more correct) which is pathname + name of the file or directory
		complete_size = path_len + name_len + 1; // +1 za '/'
		location = malloc(complete_size);
		if (location == NULL) {
			printf("open_dirs(): malloc error 4\n");
			exit(1);
		}
		strcpy(location,thread_data->directory);
		strcat(location,"/");
		strcat(location,name);

		if (file_t_init != 1) {
			file_t = malloc(sizeof(struct stat));
			if (file_t == NULL) {
				printf("malloc file_t error\n");
				exit(1);
			}
			file_t_init = 1;
		}

		errno = 0;
		if (lstat(location,file_t) != 0) {
			perror("lstat");
			printf("opendirs(): lstat(): file: %s\n", location);
			exit(1);
		}

		// if (thread_data->file_function(location,file_t) != 0)
		if (S_ISDIR(file_t->st_mode)) {
			dlist_ins_next(thread_data->directories, thread_data->directories->tail, name, file_t->st_mode, 0, location, 0, NULL);
		}
		else if (S_ISREG(file_t->st_mode)) {
			size = file_t->st_size;
			dlist_ins_next(thread_data->files, thread_data->files->tail, name, file_t->st_mode, size, location, 0, NULL);
		}
		else if (S_ISLNK(file_t->st_mode)) {
			size = file_t->st_size;
			dlist_ins_next(thread_data->files,thread_data->files->tail,name,file_t->st_mode,size,location,data5_val,NULL);
		}
	} // for (;;)

	errno = 0;
	if (closedir(dir) != 0)
		perror("closedir");
	if (dir_entry_init == 1)
		free(dir_entry);
	if (file_t_init == 1)
		free(file_t);

	if (thread_data->files->num == 0) {
		free(thread_data->files);
		thread_data->files = NULL;
	}
	if (thread_data->directories->num == 0) {
		free(thread_data->directories);
		thread_data->directories = NULL;
	}

	return 0;
}
