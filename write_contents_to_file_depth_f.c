#include "main.h"
#include "dlist.h"

int write_contents_to_file(DList_of_lists *directory, short opened, int f_descriptor)
{
	DListElmt		*list;
	int			file;
	extern char 		file_loc1[PATH_MAX];

	if (opened == 0) {
		if (directory == NULL)
			return -1;
		if (directory->this_is_top_dir == 1)
			;
		else {
			printf("Error writing the tree to the file. This is not a top directory.\n");
			return -1;
		}
		file = open(file_loc1,O_CREAT|O_RDWR|O_APPEND,S_IRWXU);
		if (file == -1) {
			printf("write_contents_to_file(): error opening the file %s\n", file_loc1);
			return -1;
		}
		write(file,directory->dir_location,strlen(directory->dir_location));
		write(file,"\n",1);
		if (directory->files != NULL) {
			list = directory->files->head;
			while (list != NULL) {
				write(file,list->dir_location,strlen(list->dir_location));
				write(file,"\n",1);
				list = list->next;
			}
		}
		if (directory->first_dir_in_chain != NULL)
			directory = directory->first_dir_in_chain;
		else
			return 0;

		while (directory->last_dir != 1) {
			write(file,directory->dir_location,strlen(directory->dir_location));
			write(file,"\n",1);
			if (directory->files != NULL) {
				list = directory->files->head;
				while (list != NULL) {
					write(file,list->dir_location,strlen(list->dir_location));
					write(file,"\n",1);
					list = list->next;
				}
			}
			if (directory->down != NULL)
				write_contents_to_file(directory->down,1,file);
			directory = directory->next;
		}
		if (directory->last_dir == 1) {
			write(file,directory->dir_location,strlen(directory->dir_location));
			write(file,"\n",1);
			if (directory->files != NULL) {
				list = directory->files->head;
				while (list != NULL) {
					write(file,list->dir_location,strlen(list->dir_location));
					write(file,"\n",1);
					list = list->next;
				}
			}
			if (directory->down != NULL)
				write_contents_to_file(directory->down,1,file);
		}
		errno = 0;
		if (close(file) == -1)
			perror("close");
		return 0;
	} // if (opened == 0)
	else if (opened == 1) {
		if (directory == directory->this_directory) {
			if (directory->first_dir_in_chain != NULL)
				directory = directory->first_dir_in_chain;
			else
				return 0;
		}
		while (directory->last_dir != 1) {
			write(f_descriptor,directory->dir_location,strlen(directory->dir_location));
			write(f_descriptor,"\n",1);
			if (directory->files != NULL) {
				list = directory->files->head;
				while (list != NULL) {
					write(f_descriptor,list->dir_location,strlen(list->dir_location));
					write(f_descriptor,"\n",1);
					list = list->next;
				}
			}
			if (directory->down != NULL)
				write_contents_to_file(directory->down,1,f_descriptor);
			directory = directory->next;
		}
		if (directory->last_dir == 1) {
			write(f_descriptor,directory->dir_location,strlen(directory->dir_location));
			write(f_descriptor,"\n",1);
			if (directory->files != NULL) {
				list = directory->files->head;
				while (list != NULL) {
					write(f_descriptor,list->dir_location,strlen(list->dir_location));
					write(f_descriptor,"\n",1);
					list = list->next;
				}
			}
			if (directory->down != NULL)
				write_contents_to_file(directory->down,1,f_descriptor);
		}
		return 0;
	} // if (opened == 1)
}
