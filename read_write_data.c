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

#define BUF_SIZE 4096
#define XATTR_SIZE 10000
#define LIST 300	// use linked lists in set_xattrs()
#define ARRAY 301	// use char array in set_xattrs()

int set_xattrs(DListElmt *, char source[], char destination[], int use);

/* Function that writes data; data argument is linked list or NULL, choose is explained before each if/else, source and destination are used when data argument is NULL, and function dives into directory tree by itself, reading and writing files and directories, or overwriting, deleting, depending on the choose argument number. */
int read_write_data(DList *data, int choose, char *source, char *destination)
{
	extern struct options_menu options;
	extern struct errors_data errors;

	DIR		*dir;
	DListElmt	*read_file_list, *read_dir_list;
	DList 		*file_list, *dir_list;

	int		read_descriptor, write_descriptor; // file descriptors + flags
	ssize_t 	num_read;
	char 		buf[BUF_SIZE];
	struct 		dirent *direntry;
	struct 		stat *file_t;
	char 		source_path[PATH_MAX];
	char 		destination_path[PATH_MAX];
	char 		new_source[PATH_MAX];
	char 		new_destination[PATH_MAX];
	char		linkpath[PATH_MAX];
	char		test[PATH_MAX];
	char		deeper[PATH_MAX];
	int		link_len;
	int		link_del;
	int		file_t_init, direntry_init;
	int		len1, len2;
	int		i;
	mode_t		file_perms, dir_perms;

	acl_t		acl;
	int		acl_res; // return value for acl function
				 // ///////////////
	char		xattr_list[XATTR_SIZE], xattr_value[XATTR_SIZE];
	char		newf_xattr_list[XATTR_SIZE], newf_xattr_value[XATTR_SIZE];
	int		xattr_len, xattr_val_len, xattr_res, setxattr_res;
	int		j;
	///
	char		*xbuf, *key, *val;
	ssize_t		buflen, keylen, vallen;
	int		setxattr_status;

	int		some_errors = 0;
	int		read_write_data_res = 0;

	file_t_init = 0;
	direntry_init = 0;

	/* copy and write files */
	if (choose == 1) {
		file_list = data;
		for (read_file_list = file_list->head; read_file_list != NULL; read_file_list = read_file_list->next) {
			errno = 0;
			read_descriptor = open(read_file_list->dir_location, O_RDONLY | options.open_flags);
			if (read_descriptor == -1) {
				if (errno == ELOOP) {
					errno = 0;
					if (readlink(read_file_list->dir_location,linkpath,PATH_MAX) != -1) {
						link_len = strlen(linkpath)+1;
						linkpath[link_len] = '\0';
					}
					else {
						perror("readlink");
						fprintf(stderr, "read_write_data() 1: %s\n", read_file_list->dir_location);
						errors.symlink_read_error_count++;
						if (options.quit_read_errors == 1)
							return -1;
						else if (options.quit_read_errors == 0) {
							some_errors++;
							continue;
						}
					}
					errno = 0;
					if (symlink(linkpath,read_file_list->new_location) != 0) {
						perror("symlink");
						fprintf(stderr, "read_write_data() 1: %s\n", read_file_list->new_location);
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_read_errors == 0) {
							some_errors++;
							continue;
						}
					}
					if (options.time_mods == 1) {
						if (options.preserve_a_time == 1) {
							options.times[0].tv_sec = read_file_list->atime;
							errno = 0;
							if (utimensat(0, read_file_list->new_location, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
								perror("utimensat");
								errors.atimestamp_error_count++;
								if (options.quit_write_errors == 1)
									return -1;
								else if (options.quit_write_errors == 0) {
									some_errors++;
									continue;
								}
							}
						}
						if (options.preserve_m_time == 1) {
							options.times[1].tv_sec = read_file_list->mtime;
							errno = 0;
							if (utimensat(0, read_file_list->new_location, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
								perror("utimensat");
								errors.mtimestamp_error_count++;
								if (options.quit_write_errors == 1)
									return -1;
								else if (options.quit_write_errors == 0) {
									some_errors++;
									continue;
								}
							}
						}
					}
					if (options.xattrs == 1) {
						setxattr_res = set_xattrs(read_file_list,0,0,LIST);
						if (setxattr_res != 0)
							return -1;
					}
					if (options.show_write_proc != 0)
						printf("%s\n", read_file_list->new_location);
					for (link_del = 0; link_del < link_len; link_del++)
						linkpath[link_del] = '\0';
					continue;
				}
				else {
					perror("open");
					fprintf(stderr, "read_write_data() 1: %s\n", read_file_list->dir_location);
					errors.file_open_error_count++;
					if (options.quit_read_errors == 1)
						return -1;
					else if (options.quit_read_errors == 0) {
						some_errors++;
						continue;
					}
				}
			}
			errno = 0;
			file_perms = 0;
			file_perms |= read_file_list->st_mode;
			write_descriptor = open(read_file_list->new_location, O_WRONLY | O_CREAT | O_EXCL, file_perms);	
			if (write_descriptor == -1) {
				perror("open");
				fprintf(stderr, "read_write_data() 1: %s\n", read_file_list->new_location);
				errors.file_open_error_count++;	
				if (options.quit_write_errors == 1)
					return -1;
				else if (options.quit_write_errors == 0)
					some_errors++;
			}
			else {
				errno = 0;
				while ((num_read = read(read_descriptor,buf,BUF_SIZE)) > 0) {
					if (write(write_descriptor, buf, num_read) != num_read) {
						fprintf(stderr, "read_write_data() 1: coudn't write the whole buffer.\n");
						errors.file_write_error_count++;
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0)
							break;
					}
				}
				if (num_read == -1) {
					perror("read");
					fprintf(stderr, "read_write_data() 1: error reading the data.\n");
					if (options.quit_read_errors == 1)
						return -1;
					else if (options.quit_read_errors == 0)
						break;
				}
			}
			if (options.time_mods == 1) {
				if (options.preserve_a_time == 1) {
					options.times[0].tv_sec = read_file_list->atime;
					errno = 0;
					if (utimensat(0, read_file_list->new_location, options.times, 0) == -1) {
						perror("utimensat");
						if (options.quit_write_errors == 1)
							exit(1);
					}
					printf("atime = %s\n", ctime(&read_file_list->atime));
					printf("options.preserve_m_time = %d\n", options.preserve_m_time);
				}
				if (options.preserve_m_time == 1) {
					options.times[1].tv_sec = read_file_list->mtime;
					errno = 0;
					if (utimensat(0, read_file_list->new_location, options.times, 0) == -1) {
						perror("utimensat");
						if (options.quit_write_errors == 1)
							exit(1);
					}
				}
			}
			if (options.acls == 1 && options.xattrs == 0) {
				errno = 0;
				acl = acl_get_file(read_file_list->dir_location,ACL_TYPE_ACCESS);
				if (acl == NULL) {
					perror("acl_get_file");
				}
				errno = 0;
				acl_res = acl_set_file(read_file_list->new_location,ACL_TYPE_ACCESS,acl);
				if (acl_res != 0) {
					perror("acl_set_file");
				}
				acl_free(acl);
			}
			if (options.xattrs == 1) {
				setxattr_res = set_xattrs(read_file_list,0,0,LIST);
				if (setxattr_res != 0)
					return -1;
			}
			
			if (options.show_write_proc != 0)
				printf("%s\n", read_file_list->new_location);
			if (close(read_descriptor) == -1) {
				fprintf(stderr, "read_write_data() 1: error closing the read descriptor.\n");
				if (options.quit_read_errors == 1) {
					errors.file_close_error_count++;
					return -1;
				}
			}
			if (close(write_descriptor) == -1) {
				fprintf(stderr, "read_write_data: error closing the write descriptor.\n");
				if (options.quit_write_errors == 1) {
					errors.file_close_error_count++;
					return -1;
				}
			}
		} // for (read_file_list = file_list->head...
		if (some_errors > 0)
			return 1;
		return 0;
	} // if (choose == 1) {

	/* full dirs to copy */
	else if (choose == 2) {
		dir_list = data;
		for (read_dir_list = dir_list->head; read_dir_list != NULL; read_dir_list = read_dir_list->next) {
			dir_perms = 0;
			dir_perms |= read_dir_list->st_mode;
			errno = 0;
			if (mkdir(read_dir_list->new_location,dir_perms) != 0) {
				perror("mkdir");
				fprintf(stderr, "read_write_data() 2: %s\n", read_dir_list->new_location);
				if (options.quit_write_errors == 1)
					return -1;
				else if (options.quit_write_errors == 0) {
					some_errors++;
					continue;
				}
			}
			if (options.time_mods == 1) {
				if (options.preserve_a_time == 1) {
					options.times[0].tv_sec = read_dir_list->atime;
					errno = 0;
					if (utimensat(0, read_dir_list->new_location, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
						perror("utimensat");
						errors.atimestamp_error_count++;
						some_errors++;
						if (options.quit_write_errors == 1)
							return -1;
					}
				}
				if (options.preserve_m_time == 1) {
					options.times[1].tv_sec = read_dir_list->mtime;
					errno = 0;
					if (utimensat(0, read_dir_list->new_location, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
						perror("utimensat");
						errors.mtimestamp_error_count++;
						some_errors++;
						if (options.quit_write_errors == 1)
							return -1;
					}
				}
			}
			if (options.acls == 1 && options.xattrs == 0) {
				errno = 0;
				acl = acl_get_file(read_dir_list->dir_location,ACL_TYPE_ACCESS);
				if (acl == NULL) {
					perror("acl_get_file");
				}
				errors.mac_error_count++;
				if (options.quit_read_errors == 1) {
					acl_free(acl);
					return -1;
				}
				errno = 0;
				acl_res = acl_set_file(read_dir_list->new_location,ACL_TYPE_ACCESS,acl);
				if (acl_res != 0) {
					perror("acl_set_file");
				}
				errors.mac_error_count++;
				if (options.quit_read_errors == 1) {
					acl_free(acl);
					return -1;
				}
				acl_free(acl);
			}
			if (options.xattrs == 1) {
				setxattr_res = set_xattrs(read_dir_list,0,0,LIST);
				if (setxattr_res != 0) {
					errors.xattr_error_count++;
					if (options.quit_write_errors == 1)
						return -1;
					else if (options.quit_write_errors == 1)
						some_errors++;
				}
			}
			if (options.show_write_proc != 0)
				printf("Directory: %s\n", read_dir_list->new_location);

			// read and copy directories and files and paste them to the destination
			read_write_data_res = read_write_data(NULL,3,read_dir_list->dir_location,read_dir_list->new_location);
			if (read_write_data_res == -1) {
				// mozda da ovo postane jedna opcija?
				if (options.quit_read_errors == 1 || options.quit_write_errors == 1)
					return -1;
			}
			else if (read_write_data_res == 1) {
				if (options.quit_read_errors == 0 || options.quit_write_errors == 0)
					some_errors++;
			}

		} // for ()
		if (some_errors > 0)
			return 1;
		return 0;
	}

	/* read and copy directories and files and write them to the destination */
	else if (choose == 3) {
		errno = 0;
		if ((dir = opendir(source)) != NULL)
			;
		else {
			if (errno != 0) {
				perror("opendir");
				fprintf(stderr, "read_write_data() 3: %s\n", source);
			}
			errors.dir_open_error_count++;
			if (options.quit_read_errors == 1)
				return -1;
			else if (options.quit_read_errors == 0) 
				return 1;
		}
		strcpy(source_path,source);
		strcpy(destination_path,destination);
		for (;;) {
			if (direntry_init != 1) {
				direntry = malloc(sizeof(struct dirent));
				if (direntry == NULL) {
					fprintf(stderr, "read_write_data() 3 malloc_error: direntry\n");
						exit(1); 
				}
				direntry_init = 1;
			}
			errno = 0;
			direntry = readdir(dir);
			if (direntry == NULL) {
				if (errno != 0) {
					perror("readdir");
					fprintf(stderr, "read_write_data() 3: %s\n", source);
					if (options.quit_read_errors == 1)
						return -1;
					else if (options.quit_read_errors == 0) {
						some_errors++;
						return 1;
					}
				}
				else
					break;
			}
			if (strcmp(direntry->d_name, ".") == 0 || strcmp(direntry->d_name, "..") == 0)
				continue;
			if (file_t_init != 1) {
				file_t = malloc(sizeof(struct stat));
				if (file_t == NULL) {
					fprintf(stderr, "read_write_data() 3: malloc_error: file_t_init\n");
					return -1;
				}
				file_t_init = 1;
			}
			len1 = strlen(test) + 1;
			len2 = strlen(direntry->d_name) + 1;
			if ((len1 + len2) > PATH_MAX) {
				fprintf(stderr, "read_write_data() 3: pathname exceeds PATH_MAX.\n");
				return -1;
			}
			strcpy(test,source_path);
			strcat(test,"/");
			strcat(test,direntry->d_name);
			errno = 0;
			if (options.stat_f(test,file_t) != 0) {
				if (options.follow_sym_links == 1)
					perror("stat");
				else if (options.follow_sym_links == 0)
					perror("lstat");
				if (options.quit_read_errors == 1)
					return -1;
				else if (options.quit_read_errors == 0) {
					some_errors++;
					return 1;
				}
			}
			if (S_ISDIR(file_t->st_mode)) {
				strcpy(new_source,source_path);
				strcat(new_source,"/");
				strcat(new_source,direntry->d_name);
				strcpy(new_destination,destination_path);
				strcat(new_destination,"/");
				strcat(new_destination,direntry->d_name);

				dir_perms = 0;
				dir_perms |= file_t->st_mode;
				errno = 0;
				if (mkdir(new_destination,dir_perms) != 0) {
					perror("mkdir");
					printf("read_write_data() 3: %s\n", new_destination);
					if (options.quit_write_errors == 1)
						return -1;
					else if (options.quit_write_errors == 0) {
						errors.dir_create_error_count++;
						some_errors++;
						continue;
					}
				}
				if (options.show_write_proc != 0)
					printf("Directory: %s\n", new_destination);
				read_write_data(NULL,3,new_source,new_destination);
				if (options.time_mods == 1) {
					if (options.preserve_a_time == 1) {
						options.times[0].tv_sec = file_t->st_atime;
						errno = 0;
						if (utimensat(0, new_destination, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
							perror("utimensat");
							errors.atimestamp_error_count++;
							if (options.quit_write_errors == 1)
								return -1;
							else if (options.quit_write_errors == 0)
								some_errors++;
						}
					}
					if (options.preserve_m_time == 1) {
						options.times[1].tv_sec = file_t->st_mtime;
						errno = 0;
						if (utimensat(0, new_destination, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
							perror("utimensat");
							errors.mtimestamp_error_count++;
							if (options.quit_write_errors == 1)
								return -1;
							else if (options.quit_write_errors == 0)
								some_errors++;
						}
					}
				}
				if (options.acls == 1 && options.xattrs == 0) {
					errno = 0;
					acl = acl_get_file(new_source,ACL_TYPE_ACCESS);
					if (acl == NULL) {
						perror("acl_get_file");
					}
					errors.mac_error_count++;
					if (options.quit_read_errors == 1) {
						acl_free(acl);
						return -1;
					}
					else if (options.quit_read_errors == 0) {
						acl_free(acl);
						some_errors++;
					}
					errno = 0;
					acl_res = acl_set_file(new_destination,ACL_TYPE_ACCESS,acl);
					if (acl_res != 0) {
						perror("acl_set_file");
					}
					errors.mac_error_count++;
					if (options.quit_read_errors == 1) {
						acl_free(acl);
						return -1;
					}
					else if (options.quit_read_errors == 0) {
						some_errors++;
						acl_free(acl);
					}
				}
				if (options.xattrs == 1) {
					setxattr_res = set_xattrs(NULL,new_source,new_destination,ARRAY);
					if (setxattr_res != 0) {
						errors.xattr_error_count++;
						if (options.quit_read_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0)
							some_errors++;
					}
				}
			}
			else if (S_ISREG(file_t->st_mode)) {
				strcpy(new_source,source_path);
				strcat(new_source,"/");
				strcat(new_source,direntry->d_name);
				strcpy(new_destination,destination_path);
				strcat(new_destination,"/");
				strcat(new_destination,direntry->d_name);

				errno = 0;
				read_descriptor = open(new_source, O_RDONLY | options.open_flags);
				if (read_descriptor == -1) {	
					perror("open");
					fprintf(stderr, "read_write_data() 3: %s\n", source_path);
					errors.file_open_error_count++;
					if (options.quit_read_errors == 1)
						return -1;
					else if (options.quit_read_errors == 0) {
						some_errors++;
						continue;
					}
				}
				file_perms = 0;
				file_perms |= file_t->st_mode;
				errno = 0;
				write_descriptor = open(new_destination, O_WRONLY | O_CREAT | O_EXCL, file_perms);
				if (write_descriptor == -1) {
					perror("open");
					fprintf(stderr, "read_write_data() 3: %s\n", destination_path);
					errors.file_create_error_count++;
					if (options.quit_write_errors == 1)
						return -1;
					else if (options.quit_write_errors == 0) {
						some_errors++;
						continue;
					}
				}
				else {
					errno = 0;
					while ((num_read = read(read_descriptor,buf,BUF_SIZE)) > 0)
						if (write(write_descriptor, buf, num_read) != num_read) {
							fprintf(stderr, "read_write_data: coudn't write the whole buffer.\n");
							errors.file_write_error_count++;
							if (options.quit_write_errors == 1) 
								return -1;
							else if (options.quit_write_errors == 0) {
								some_errors++;
								continue;
							}
						}
					if (num_read == -1) {
						perror("read");
						fprintf(stderr, "read_write_data() 3: %s\n", source_path);
						errors.file_read_error_count++;
						if (options.quit_read_errors == 1)
							return -1;
						else if (options.quit_read_errors == 0) {
							some_errors++;
							continue;
						}
					}
				}
				if (options.time_mods == 1) {
					if (options.preserve_a_time == 1) {
						options.times[0].tv_sec = file_t->st_atime;
						errno = 0;
						if (utimensat(0, new_destination, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
							perror("utimensat");
							errors.atimestamp_error_count++;
							if (options.quit_write_errors == 1)
								return -1;
							else if (options.quit_write_errors == 0)
								some_errors++;
						}
					}
					if (options.preserve_m_time == 1) {
						options.times[1].tv_sec = file_t->st_mtime;
						errno = 0;
						if (utimensat(0, new_destination, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
							perror("utimensat");
							errors.mtimestamp_error_count++;
							if (options.quit_write_errors == 1)
								return -1;
							else if (options.quit_write_errors == 0)
								some_errors++;
						}
					}
				}
				if (options.acls == 1 && options.xattrs == 0) {
					errno = 0;
					acl = acl_get_file(new_source,ACL_TYPE_ACCESS);
					if (acl == NULL) {
						perror("acl_get_file");
					}
					errors.mac_error_count++;
					if (options.quit_read_errors == 1) {
						acl_free(acl);
						return -1;
					}
					else if(options.quit_read_errors == 0)
						some_errors++;
					errno = 0;
					acl_res = acl_set_file(new_destination,ACL_TYPE_ACCESS,acl);
					if (acl_res != 0) {
						perror("acl_set_file");
					}
					errors.mac_error_count++;
					if (options.quit_read_errors == 1) {
						acl_free(acl);
						return -1;
					}
					else if (options.quit_read_errors == 0) {
						some_errors++;
						acl_free(acl);
					}
				}
				if (options.xattrs == 1) {
					setxattr_res = set_xattrs(NULL,new_source,new_destination,ARRAY);
					if (setxattr_res != 0) {
						errors.xattr_error_count++;
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0)
							some_errors++;
					}
				}
				
				if (options.show_write_proc != 0)
					printf("%s\n", new_destination);
				if (close(read_descriptor) == -1) {
					fprintf(stderr, "read_write_data: error closing the read descriptor.\n");
					if (options.quit_read_errors == 1)
						return -1;
					else if (options.quit_read_errors == 0)
						some_errors++;
				}
				if (close(write_descriptor) == -1) {
					printf("read_write_data: error closing the write descriptor.\n");
					if (options.quit_write_errors == 1)
						return -1;
					else if (options.quit_write_errors == 0)
						some_errors++;
				}
			}
			else if (S_ISLNK(file_t->st_mode)) {
				strcpy(new_source,source_path);
				strcat(new_source,"/");
				strcat(new_source,direntry->d_name);
				strcpy(new_destination,destination_path);
				strcat(new_destination,"/");
				strcat(new_destination,direntry->d_name);
				errno = 0;
				if (readlink(new_source,linkpath,PATH_MAX) != -1) {
					link_len = strlen(linkpath)+1;
					linkpath[link_len] = '\0';
				}
				else {
					perror("readlink");
					fprintf(stderr, "read_write_data() 3: %s\n", new_source);
					errors.symlink_read_error_count++;
					if (options.quit_read_errors == 1)
						return -1;
					else if (options.quit_read_errors == 0) {
						some_errors++;
						continue;
					}
				}
				errno = 0;
				if (symlink(linkpath,new_destination) != 0) {
					perror("symlink");
					fprintf(stderr, "read_write_data 3: %s\n", new_destination);
					errors.symlink_write_error_count++;
					if (options.quit_write_errors == 1)
						return -1;
					if (options.quit_write_errors == 0) {
						some_errors++;
						continue;
					}
				}
				if (options.time_mods == 1) {
					if (options.preserve_a_time == 1) {
						options.times[0].tv_sec = file_t->st_atime;
						errno = 0;
						if (utimensat(0, new_destination, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
							perror("utimensat");
							errors.atimestamp_error_count++;
							if (options.quit_write_errors == 1)
								return -1;
							else if (options.quit_write_errors == 0)
								some_errors++;
						}
					}
					if (options.preserve_m_time == 1) {
						options.times[1].tv_sec = file_t->st_mtime;
						errno = 0;
						if (utimensat(0, new_destination, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
							perror("utimensat");
							errors.mtimestamp_error_count++;
							if (options.quit_write_errors == 1)
								return -1;
							else if (options.quit_write_errors == 0)
								some_errors++;
						}
					}
				}
				if (options.xattrs == 1) {
					setxattr_res = set_xattrs(NULL,new_source,new_destination,ARRAY);
					if (setxattr_res != 0) {
						errors.xattr_error_count++;
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0)
							some_errors++;
					}
				}
				if (options.show_write_proc != 0)
					printf("%s\n", new_destination);
				for (link_del = 0; link_del < link_len; link_del++)
					linkpath[link_del] = '\0';
			}
		} // for (;;)
		errno = 0;
		if (closedir(dir) != 0) {
			perror("closedir");
			errors.dir_close_error_count++;
			if (options.quit_read_errors == 1)
				return -1;
			else if (options.quit_read_errors == 0)
				some_errors++;
		}
		if (file_t_init != 0) {
			free(file_t);
			file_t_init = 0;
		}
		if (direntry_init != 0) {
			free(direntry);
			direntry_init = 0;
		}
		if (some_errors > 0)
			return 1;
		return 0;
	}

	/* overwrite files */
	else if (choose == 4) {
		file_list = data;
		for (read_file_list = file_list->head; read_file_list != NULL; read_file_list = read_file_list->next) {
			errno = 0;
			read_descriptor = open(read_file_list->dir_location, O_RDONLY | options.open_flags);
			if (read_descriptor == -1) {
				if (errno == ELOOP) {
					errno = 0;
					// PATH_MAX umjesto 1024
					if (readlink(read_file_list->dir_location,linkpath,1024) != -1) {
						link_len = strlen(linkpath)+1;
						linkpath[link_len] = '\0';
					}
					else {
						perror("readlink");
						fprintf(stderr, "read_write_data() 4: %s\n", read_file_list->dir_location);
						errors.symlink_read_error_count++;
						if (options.quit_read_errors == 1)
							return -1;
						else if (options.quit_read_errors == 0) {
							some_errors++;
							continue;
						}
					}
					errno = 0;
					if (unlink(read_file_list->new_location) != 0) {
						perror("unlink");
						fprintf(stderr, "read_write_data() 4: %s\n", read_file_list->new_location);
						errors.symlink_overwrite_error_count++;
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0) {
							some_errors++;
							continue;
						}
					}
					errno = 0;
					if (symlink(linkpath,read_file_list->new_location) != 0) {
						perror("symlink");
						fprintf(stderr, "read_write_data() 4: %s\n", read_file_list->dir_location);
						errors.symlink_overwrite_error_count++;
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0) {
							some_errors++;
							continue;
						}
					}
					if (options.time_mods == 1) {
						if (options.preserve_a_time == 1) {
							options.times[0].tv_sec = read_file_list->atime;
							errno = 0;
							if (utimensat(0, read_file_list->new_location, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
								perror("utimensat");
								if (options.quit_write_errors == 1)
									exit(1);
							}
						}
						if (options.preserve_m_time == 1) {
							options.times[1].tv_sec = read_file_list->mtime;
							errno = 0;
							if (utimensat(0, read_file_list->new_location, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
								perror("utimensat");
								if (options.quit_write_errors == 1)
									exit(1);
							}
						}
					}
					if (options.xattrs == 1) {
						setxattr_res = set_xattrs(read_file_list,0,0,LIST);
						if (setxattr_res != 0)
							return -1;
					}
					if (options.show_write_proc != 0)
						printf("Overwriting: %s\n", read_file_list->new_location);
					for (link_del = 0; link_del < link_len; link_del++)
						linkpath[link_del] = '\0';
					//continue;
				} // if (ELOOP)
				else {	
					perror("open");
					fprintf(stderr, "read_write_data() 4: %s\n", read_file_list->dir_location);
					if (options.quit_read_errors == 1)
						return -1;
					else if (options.quit_read_errors == 0) {
						some_errors++;
						continue;
					}
				}
			}
			file_perms = 0;
			file_perms |= read_file_list->st_mode;
			errno = 0;
			write_descriptor = open(read_file_list->new_location, O_WRONLY | O_EXCL | O_TRUNC, file_perms);
			if (write_descriptor == -1) {
				perror("open");
				fprintf(stderr, "read_write_data() 4: %s\n", read_file_list->new_location);
				errors.file_open_error_count++;
				if (options.quit_write_errors == 1)
					return -1;
				else if (options.quit_write_errors == 0) {
					some_errors++;
					continue;
				}

			}
			else {
				errno = 0;
				while ((num_read = read(read_descriptor,buf,BUF_SIZE)) > 0) {
					if (write(write_descriptor, buf, num_read) != num_read) {
						fprintf(stderr, "read_write_data: coudn't write the whole buffer: %s\n", read_file_list->new_location);
						errors.file_write_error_count++;
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0) {
							some_errors++;
							break;
						}

					}
				}
				if (num_read == -1) {
					perror("read");
					fprintf(stderr, "read_write_data() 4: error reading the data: %s\n", read_file_list->dir_location);
					if (options.quit_read_errors == 1)
						return -1;
					else if (options.quit_read_errors == 0) {
						some_errors++;
						continue;
					}
				}
			}
			if (options.time_mods == 1) {
				if (options.preserve_a_time == 1) {
					options.times[0].tv_sec = read_file_list->atime;
					errno = 0;
					if (utimensat(0, read_file_list->new_location, options.times, 0) == -1) {
						perror("utimensat");
						if (options.quit_write_errors == 1)
							exit(1);
					}
				}
				if (options.preserve_m_time == 1) {
					options.times[1].tv_sec = read_file_list->mtime;
					errno = 0;
					if (utimensat(0, read_file_list->new_location, options.times, 0) == -1) {
						perror("utimensat");
						if (options.quit_write_errors == 1)
							exit(1);
					}
				}
			}
			if (options.acls == 1 && options.xattrs == 0) {
				errno = 0;
				acl = acl_get_file(read_file_list->dir_location,ACL_TYPE_ACCESS);
				if (acl == NULL) {
					perror("acl_get_file");
				}
				errno = 0;
				acl_res = acl_set_file(read_file_list->new_location,ACL_TYPE_ACCESS,acl);
				if (acl_res != 0) {
					perror("acl_set_file");
				}
				acl_free(acl);
			}
			if (options.xattrs == 1) {
				setxattr_res = set_xattrs(read_file_list,0,0,LIST);
				if (setxattr_res != 0)
					return -1;
			}
			if (options.show_write_proc != 0)
				printf("Overwriting: %s\n", read_file_list->new_location);
			errno = 0;
			if (close(read_descriptor) == -1) {
				perror("close");
				printf("read_write_data() 4: error closing the read descriptor.\n");
				if (options.quit_read_errors == 1)
					return -1;
				else if (options.quit_read_errors == 0) {
					some_errors++;
					continue;
				}
			}
			errno = 0;
			if (close(write_descriptor) == -1) {
				perror("close");
				printf("read_write_data() 4: error closing the write descriptor.\n");
				if (options.quit_write_errors == 1)
					return -1;
				else if (options.quit_write_errors == 0) {
					some_errors++;
					continue;
				}
			}
		} // for (read_file_list = file_list->head...
		if (some_errors > 0)
			return 1;
		return 0;
	} // choose 4

	// delete files
	else if (choose == 5) {
		file_list = data;
		for (read_file_list = file_list->head; read_file_list != NULL; read_file_list = read_file_list->next) {
			errno = 0;
			if (unlink(read_file_list->dir_location) != 0) {
				perror("unlink");
				printf("read_write_data() 5: %s\n", read_file_list->dir_location);
				errors.file_delete_error_count++;
				if (options.quit_delete_errors == 1)
					return -1;
				else if (options.quit_delete_errors == 0) {
					some_errors++;
					continue;
				}
			}
			if (options.show_write_proc != 0)
				printf("Deleting: %s\n", read_file_list->dir_location);
		}
		if (some_errors > 0)
			return 1;
		return 0;
	}

	// delete directories
	else if (choose == 6) {
		dir_list = data;
		for (read_dir_list = dir_list->head; read_dir_list != NULL; read_dir_list = read_dir_list->next) {
			read_write_data_res = read_write_data(NULL,7,read_dir_list->dir_location,NULL);
			if (read_write_data_res == -1)
				return -1;
			else if (read_write_data_res == 1) {
				if (options.quit_write_errors == 0) {
					some_errors++;
					continue;
				}
			}
			errno = 0;
			if (rmdir(read_dir_list->dir_location) != 0) {
				perror("rmdir");
				printf("read_write_data() 6: %s\n", read_dir_list->dir_location);
				errors.dir_delete_error_count++;
				if (options.quit_delete_errors == 1)
					return -1;
				else if (options.quit_delete_errors == 0) {
					some_errors++;
					continue;
				}
			}
			if (options.show_write_proc != 0)
				printf("Deleting: %s\n", read_dir_list->dir_location);
		}
		if (some_errors > 0)
			return 1;
		return 0;
	}

	/* open directories and delete all files from them, and when they are empty, delete these directories */
	else if (choose == 7) {
		errno = 0;
		dir = opendir(source);
		if (dir == NULL) {
			perror("opendir");
			printf("read_write_data(): error 7. %s\n", source);
			if (options.quit_read_errors == 1)
				return -1;
			else if (options.quit_read_errors == 0)
				return 1;
		}
		for (;;) {
			if (direntry_init != 1) {
				direntry = malloc(sizeof(struct dirent));
				if (direntry == NULL) {
					printf("malloc(), read_write_data() 7: direntry.\n");
					return -1;
				}
				direntry_init = 1;
			}
			errno = 0;
			direntry = readdir(dir);
			if (direntry == NULL) {
				if (errno != 0) {
					perror("readdir");
					if (options.quit_read_errors == 1)
						return -1;
					else if (options.quit_read_errors == 0) {
						some_errors++;
						continue;
					}
				}
			}
			if (strcmp(direntry->d_name, ".") == 0 || strcmp(direntry->d_name, "..") == 0)
				continue;
			if (file_t_init != 1) {
				file_t = malloc(sizeof(struct stat));
				if (file_t == NULL) {
					printf("malloc() read_write_data() 7: file_t_init\n");
					return -1;
				}
				file_t_init = 1;
			}

			len1 = strlen(source) + 1;
			len2 = strlen(direntry->d_name) + 1;
			if ((len1 + len2) > PATH_MAX) {
				printf("read_write_data() 7: pathname exeeded PATH_MAX limit. exiting.\n");
				return -1;
			}
			strcpy(test,source);
			strcat(test,"/");
			strcat(test,direntry->d_name);

			errno = 0;
			if (options.stat_f(test,file_t) != 0) {
				if (options.follow_sym_links == 1)
					perror("stat");
				else if (options.follow_sym_links == 0)
					perror("lstat");
			}
			if (S_ISDIR(file_t->st_mode)) {
				len1 = strlen(source) + 1;
				len2 = strlen(direntry->d_name) + 1;
				if ((len1 + len2) > PATH_MAX) {
					printf("read_write_data() 7: pathname exeeded PATH_MAX limit. exiting.\n");
					return -1;
				}
				strcpy(deeper,source);
				strcat(deeper,"/");
				strcat(deeper,direntry->d_name);
				read_write_data_res = read_write_data(NULL,7,deeper,NULL);
				errno = 0;
				if (rmdir(deeper) != 0) {
					perror("rmdir");
					printf("read_write_data() 8: %s\n", deeper);
					errors.dir_delete_error_count++;
					if (options.quit_delete_errors == 1)
						return -1;
					else if (options.quit_delete_errors == 0) {
						some_errors++;
						continue;
					}
				}
				if (options.show_write_proc != 0)
					printf("Deleting: %s\n", deeper);
			}
			if (S_ISREG(file_t->st_mode)) {
				errno = 0;
				if (unlink(test) != 0) {
					perror("unlink");
					printf("read_write_data() 7: %s\n", test);
					errors.file_delete_error_count++;
					if (options.quit_delete_errors == 1)
						return -1;
					if (options.quit_delete_errors == 0) {
						some_errors++;
						continue;
					}
				}
				if (options.show_write_proc != 0)
					printf("Deleting: %s\n", test);
			}
			else if (S_ISLNK(file_t->st_mode)) {
				errno = 0;
				if (unlink(test) != 0) {
					perror("unlink");
					printf("read_write_data() 7: %s\n", test);
					errors.symlink_delete_error_count++;
					if (options.quit_delete_errors == 1)
						return -1;
					else if (options.quit_delete_errors == 0) {
						some_errors++;
						continue;
					}
				}
				if (options.show_write_proc != 0)
					printf("Deleting: %s\n", test);
			}
		}
		if (some_errors > 0)
			return 1;
		return 0;
	} // choose 7
	// write symbolic links
	else if (choose == 8) {
		file_list = data;
		for (read_file_list = file_list->head; read_file_list != NULL; read_file_list = read_file_list->next) {
			errno = 0;
			if (readlink(read_file_list->dir_location,linkpath,PATH_MAX) != -1) {
				link_len = strlen(linkpath)+1;
				linkpath[link_len] = '\0';
			}
			else {
				perror("readlink");
				printf("read_write_data() 8: %s\n", read_file_list->dir_location);
				errors.symlink_read_error_count++;
				if (options.quit_read_errors == 1)
					return -1;
				else if (options.quit_read_errors == 0) {
					some_errors++;
					continue;
				}
			}
			errno = 0;
			if (symlink(linkpath,read_file_list->new_location) != 0) {
				perror("symlink");
				printf("read_write_data() 8: %s\n", read_file_list->new_location);
				errors.symlink_write_error_count++;
				if (options.quit_write_errors == 1)
					return -1;
				else if (options.quit_write_errors == 0) {
					some_errors++;
					continue;
				}
			}
			if (options.time_mods == 1) {
				if (options.preserve_a_time == 1) {
					options.times[0].tv_sec = read_file_list->atime;
					errno = 0;
					if (utimensat(0, read_file_list->new_location, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
						perror("utimensat");
						errors.atimestamp_error_count++;
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0)
							some_errors++;
					}
				}
				if (options.preserve_m_time == 1) {
					options.times[1].tv_sec = read_file_list->mtime;
					errno = 0;
					if (utimensat(0, read_file_list->new_location, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
						perror("utimensat");
						errors.mtimestamp_error_count++;
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0)
							some_errors++;
					}
				}
			}
			if (options.show_write_proc != 0)
				printf("%s\n", read_file_list->new_location);
			for (link_del = 0; link_del < link_len; link_del++)
				linkpath[link_del] = '\0';
		}
		if (some_errors > 0)
			return 1;
		return 0;
	}
	// overwrite symbolic links
	else if (choose == 9) {
		file_list = data;
		for (read_file_list = file_list->head; read_file_list != NULL; read_file_list = read_file_list->next) {
			errno = 0;
			if (readlink(read_file_list->dir_location,linkpath,1024) != -1) {
				link_len = strlen(linkpath)+1;
				linkpath[link_len] = '\0';
				printf("linkpath: %s\n", linkpath);
			}
			else {
				perror("readlink");
				printf("read_write_data() 9: %s\n", read_file_list->dir_location);
				errors.symlink_read_error_count++;
				if (options.quit_read_errors == 1)
					return -1;
				else if (options.quit_read_errors == 0) {
					some_errors++;
					continue;
				}
			}
			errno = 0;
			if (unlink(read_file_list->new_location) != 0) {
				perror("unlink");
				printf("read_write_data() 9: %s\n", read_file_list->new_location);
				errors.symlink_overwrite_error_count++;
				if (options.quit_write_errors == 1)
					return -1;
				else if (options.quit_write_errors == 0) {
					some_errors++;
					continue;
				}
			}
			errno = 0;
			if (symlink(linkpath,read_file_list->new_location) != 0) {
				perror("symlink");
				printf("read_write_data() 9: %s\n", read_file_list->new_location);
				errors.symlink_overwrite_error_count++;
				if (options.quit_write_errors == 1)
					return -1;
				else if (options.quit_write_errors == 0) {
					some_errors++;
					continue;
				}
			}
			if (options.time_mods == 1) {
				if (options.preserve_a_time == 1) {
					options.times[0].tv_sec = read_file_list->atime;
					errno = 0;
					if (utimensat(0, read_file_list->new_location, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
						perror("utimensat");
						errors.atimestamp_error_count++;
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0)
							some_errors++;
					}
				}
				if (options.preserve_m_time == 1) {
					options.times[1].tv_sec = read_file_list->mtime;
					errno = 0;
					if (utimensat(0, read_file_list->new_location, options.times, AT_SYMLINK_NOFOLLOW) == -1) {
						perror("utimensat");
						errors.mtimestamp_error_count++;
						if (options.quit_write_errors == 1)
							return -1;
						else if (options.quit_write_errors == 0)
							some_errors;
					}
				}
			}
			if (options.show_write_proc != 0)
				printf("Overwriting %s\n", read_file_list->new_location);
			for (link_del = 0; link_del < link_len; link_del++)
				linkpath[link_del] = '\0';
		}
		if (some_errors > 0)
			return 1;
		return 0;
	}
}

int set_xattrs(DListElmt *to_copy, char source[], char destination[], int use)
{
	extern struct options_menu options;
	extern struct errors_data errors;

	acl_t		acl;
	int		acl_res; // return value for acl function

	char		xattr_list[XATTR_SIZE], xattr_value[XATTR_SIZE];
	char		newf_xattr_list[XATTR_SIZE], newf_xattr_value[XATTR_SIZE];
	int		xattr_len, xattr_val_len, xattr_res, setxattr_res;
	int		j;

	char		*xbuf, *key, *val;
	ssize_t		buflen, keylen, vallen;
	int		setxattr_status;

	char 		*current_loc, *new_loc;

	if (use == LIST) {
		current_loc = to_copy->dir_location;
		new_loc = to_copy->new_location;
	}
	else if (use == ARRAY) {
		current_loc = &source[0];
		new_loc = &destination[0];
	}
		
	errno = 0;
	buflen = options.listxattr_func(current_loc, NULL, 0);
	if (buflen == -1) {
		perror("listxattr");
		errors.xattr_error_count++;
		if (options.quit_read_errors == 1)
			return -1;
		else if (options.quit_read_errors == 0) {
			//some_errors++;
			return 1;
		}
	}
	if (buflen != 0) {
		xbuf = malloc(buflen);
		if (xbuf != NULL)
			key = xbuf;
		else {
			fprintf(stderr, "set_xattrs(): buflen malloc() error.\n");
			return -1;
		}
		buflen = options.listxattr_func(current_loc, xbuf, buflen);
		if (buflen == -1) {
			errors.xattr_error_count++;
			perror("listxattr");
			if (options.quit_read_errors == 1) {
				free(xbuf);
				return -1;
			}
			else if (options.quit_read_errors == 0) {
				free(xbuf);
				return 1;
			}
		}
		key = xbuf;
		while (buflen > 0) {
			if (strcmp("system.posix_acl_access",key) == 0 && options.acls != 1) {
				keylen = strlen(key)+1;
				buflen -= keylen;
				key += keylen;
				continue;
			}
			errno = 0;
			vallen = options.getxattr_func(current_loc, key, NULL, 0);
			if (vallen == -1) {
				perror("getxattr");
				errors.xattr_error_count++;
				if (options.quit_read_errors == 1) {
					free(xbuf);
					return -1;
				}
				else if (options.quit_read_errors == 0) {
					free(xbuf);
					return 1;
				}
			}
			else if (vallen > 0) {
				val = malloc(vallen+1);
				if (val == NULL) {
					fprintf(stderr, "set_xattrs(4): vallen+1 malloc() error.\n");
					return -1;
				}
				errno = 0;
				vallen = options.getxattr_func(current_loc, key, val, vallen);
				if (vallen == -1) {
					errors.xattr_error_count++;
					perror("getxattr");
					if (options.quit_read_errors == 1) {
						free(xbuf);
						free(val);
						return -1;
					}
					else if (options.quit_read_errors == 0) {
						free(xbuf);
						free(val);
						return 1;
					}
				}
				else {
					val[vallen] = 0;
					errno = 0;
					setxattr_status = options.setxattr_func(new_loc, key, val, vallen, XATTR_CREATE);
					if (setxattr_status == -1) {
						errors.xattr_error_count++;
						perror("estxattr");
						if (options.quit_read_errors == 1) {
							free(xbuf);
							free(val);
							return -1;
						}
						else if (options.quit_read_errors == 0) {
							free(xbuf);
							free(val);
							return 1;
						}
					}
					free(val);
				}
			}
			else if (vallen == 0) {
				free(xbuf);
				break;
			}
			keylen = strlen(key)+1;
			buflen -= keylen;
			key += keylen;
		}
		free(xbuf);
	}
	return 0;
}
