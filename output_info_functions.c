#include "main.h"
#include "dlist.h"
#include "data_copy_info.h"
#include "options.h"

#define SCREEN 101		// output from list_stats() and calc_size() to stdout
#define IN_FILE 106

char *detailed_output(DList *to_copy_list, int output, char *what_is_copied, int fd);

	char *string1 = "Files to copy:\n";
	char *string1_1 = "Symbolic links to copy:\n";
	char *string2 = "Directories to copy:\n";
	char *string3 = "Files to overwrite (source location files larger than destination)\n";
	char *string4 = "Files to overwrite (source location files smaller than destination)\n";
	char *string5 = "Extraneous files to copy:\n";
	char *string5_1 = "Extraneous symbolic links to copy:\n";
	char *string6 = "Extraneous directories to copy:\n";
	char *string7 = "Extraneous files to delete:\n";
	char *string7_1 = "Extraneous symbolic links to delete:\n";
	char *string8 = "Extraneous directories to delete:\n";
	char *string9 = "Files to overwrite (source location files newer than destination)\n";
	char *string10 = "Files to overwrite (source location files older than destination)\n";
	char *string11 = "Symbolic links to overwrite (source location files smaller than destination)\n";
	char *string12 = "Symbolic links to overwrite. (source location files larger than destination)\n";
	char *string13 = "Symbolic links to overwrite (source location files newer than destination)\n";
	char *string14 = "Symbolic links to overwrite (source location files older than destination)\n";
	char *string15 = "Extraneous files:\n";
	char *string16 = "Extraneous symbolic links:\n";
	char *string17 = "Extraneous directories:\n";
	
void output_info_function(int copyfile) // Dodat još delete opcije
{
	extern struct Data_Copy_Info data_copy_info;
	extern struct options_menu options;
	extern struct data_found results;
	extern int just_opt_active;
	
	DList *file_list, *dir_list, *file_surp_list, *dir_surp_list;	// file and directory lists, file and directory extraneous lists
	DList *symlinks_list, *symlinks_surp_list;
	DList *file_ms_list, *file_ml_list;				// file main smaller and main larger lists
	DList *file_mn_list, *file_mo_list;				// file main newer and main smaller lists
	DList *symlinks_ms_list, *symlinks_ml_list;
	DList *symlinks_mn_list, *symlinks_mo_list;
	DListElmt *file_list_element, *dir_list_element;		// used to loop through file and directory lists to display files and diretories to copy, etc...
	
	char file_location[PATH_MAX];					// copy/content file location + the newline char to avoid using write() sys call just for '\n'
	
	file_list = data_copy_info.files_to_copy_list;
	if (file_list != NULL) {
		if (file_list->num != 0 && just_opt_active != 1) { // dodat kasnije razdvojeno za just write copy file i copy file nakon samog kopiranja gdje će prepoznat ak je negdje odgovoreno s NO?
			results.copy_files = 1;
			if (options.dont_list_data_to_copy != 1) {
				if (options.less_detailed != 1)
					detailed_output(file_list,SCREEN,string1,0);
				else if (options.less_detailed == 1) {
					printf("\nFiles to copy:\n\n");
					for (file_list_element = file_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
						printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
				}
			}
			if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
				if (options.less_detailed != 1)
					detailed_output(file_list,IN_FILE,string1,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string1, strlen(string1));
					for (file_list_element = file_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						strcpy(file_location,file_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
		}
	}
	if (options.ignore_symlinks != 1) {
		symlinks_list = data_copy_info.symlinks_to_copy_list;
		if (symlinks_list != NULL && just_opt_active != 1) {
			if (symlinks_list->num != 0) {
				results.copy_symlinks = 1;
				if (options.dont_list_data_to_copy != 1) {
					if (options.less_detailed != 1) 
						detailed_output(symlinks_list,SCREEN,string1_1,0);
					else if (options.less_detailed == 1) {
						printf("\nSymbolic links to copy:\n\n");
						for (file_list_element = file_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
				}
				if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_list,IN_FILE,string1_1,copyfile);
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
		}
	}
	dir_list = data_copy_info.dirs_to_copy_list;
	if (dir_list != NULL) {
		if (dir_list->num != 0 && just_opt_active != 1) {
			results.copy_dirs = 1;
			if (options.dont_list_data_to_copy != 1) {
				if (options.less_detailed != 1)
					detailed_output(dir_list,SCREEN,string2,0);
				else if (options.less_detailed == 1) {
					printf("\nDirectories to copy:\n\n");
					for (dir_list_element = dir_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next)
						printf("directory: %s\n location: %s\n", dir_list_element->name, dir_list_element->dir_location);
				}
			}
			if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
				if (options.less_detailed != 1)
					detailed_output(dir_list,IN_FILE,string2,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string2, strlen(string2));
					for (dir_list_element = dir_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next) {
						strcpy(file_location,dir_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
		}
	}
	file_surp_list = data_copy_info.files_extraneous_list;
	if (file_surp_list != NULL) {
		if (file_surp_list->num != 0 && options.list_extraneous == 1) {
			results.files_extraneous = 1;
			if (options.dont_list_data_to_copy != 1) {
				if (options.list_extraneous == 1) {
					if (options.less_detailed != 1)
						detailed_output(file_surp_list,SCREEN,string15,0);
					else if (options.less_detailed == 1) {
						printf("\nExtraneous files:\n\n");
						for (file_list_element = file_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
					if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
						if (options.less_detailed != 1)
							detailed_output(file_surp_list,IN_FILE,string5,copyfile);
						else if (options.less_detailed == 1) {
							write(copyfile, string5, strlen(string5));
							for (file_list_element = file_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
								strcpy(file_location,file_list_element->dir_location);
								strcat(file_location,"\n");
								write(copyfile, file_location, strlen(file_location));
							}
						}
					}
				}
			}
		}
	}
	if (options.ignore_symlinks != 1) {
		symlinks_surp_list = data_copy_info.symlinks_extraneous_list;
		if (symlinks_surp_list != NULL) {
			if (symlinks_surp_list->num != 0 && options.list_extraneous == 1) {
				results.symlinks_extraneous = 1;
				if (options.dont_list_data_to_copy != 1) {
					if (options.list_extraneous == 1) {
						if (options.less_detailed != 1)
							detailed_output(symlinks_surp_list,SCREEN,string16,0);
						else if (options.less_detailed == 1) {
							printf("\nExtraneous symbolic:\n\n");
								for (file_list_element = symlinks_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next)
									printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
					}
					if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
						if (options.less_detailed != 1)
							detailed_output(symlinks_surp_list,IN_FILE,string5_1,copyfile);
						else if (options.less_detailed == 1) {
							write(copyfile, string5_1, strlen(string5_1));
							for (file_list_element = symlinks_surp_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
								strcpy(file_location,file_list_element->dir_location);
								strcat(file_location,"\n");
								write(copyfile, file_location, strlen(file_location));
							}
						}
					}
				}
			}
		}
	}
	dir_surp_list = data_copy_info.dirs_extraneous_list;
	if (dir_surp_list != NULL) {
		if (dir_surp_list->num != 0 && options.list_extraneous == 1) {
			results.dirs_extraneous = 1;
			if (options.list_extraneous == 1) {
				if (options.dont_list_data_to_copy != 1) {
					if (options.less_detailed != 1)
						detailed_output(dir_surp_list,SCREEN,string17,0);
					else if (options.less_detailed == 1) {
						printf("\nExtraneous directories:\n\n");
						for (dir_list_element = dir_surp_list->head; dir_list_element != NULL; dir_list_element = dir_list_element->next)
							printf("directory: %s\n location: %s\n", dir_list_element->name, dir_list_element->dir_location);
					}
				}
				if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
					if (options.less_detailed != 1)
						detailed_output(dir_surp_list,IN_FILE,string6,copyfile);
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
		}
	}
	file_ml_list = data_copy_info.diff_size_ml_list;
	if (file_ml_list != NULL) {
		if (file_ml_list->num != 0) {
			if (options.ow_main_larger == 1)
				results.ow_main_larger = 1; // overwrite main larger
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_larger == 1) {
				if (options.less_detailed != 1)
					detailed_output(file_ml_list,SCREEN,string3,0);
				else if (options.less_detailed == 1) {
					printf("\nFiles to overwrite. (source location files larger than destination)\n\n");
					for (file_list_element = file_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
				}
			}
			if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
				if (options.less_detailed != 1)
					detailed_output(file_ml_list,IN_FILE,string3,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string3, strlen(string3));
					for (file_list_element = file_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						strcpy(file_location,file_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
		}
	}
	
	file_ms_list = data_copy_info.diff_size_ms_list;
	if (file_ms_list != NULL) {
		if (file_ms_list->num != 0) {
			if (options.ow_main_smaller == 1)
				results.ow_main_smaller = 1; // overwrite main smaller
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_smaller == 1) {
				if (options.less_detailed != 1) 
					detailed_output(file_ms_list,SCREEN,string4,0);
				else if (options.less_detailed == 1) {
					printf("\nFiles to overwrite. (source location files smaller than destination)\n\n");
					for (file_list_element = file_ms_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
				}
			}
			if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
				if (options.less_detailed != 1)
					detailed_output(file_ms_list,IN_FILE,string4,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string4, strlen(string4));
					for (file_list_element = file_ms_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						strcpy(file_location,file_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
		}
	}

	file_mn_list = data_copy_info.diff_time_mn_list;
	if (file_mn_list != NULL) {
		if (file_mn_list->num != 0) {
			if (options.ow_main_newer == 1)
				results.ow_main_newer = 1; // overwrite main newer
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_newer == 1) {
				if (options.less_detailed != 1)
					detailed_output(file_mn_list,SCREEN,string9,0);
				else if (options.less_detailed == 1) {
					printf("\nFiles to overwrite. (source location files newer than destination)\n\n");
					for (file_list_element = file_mn_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
				}
			}
			if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
				if (options.less_detailed != 1)
					detailed_output(file_mn_list,IN_FILE,string9,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string9, strlen(string9));
					for (file_list_element = file_mn_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						strcpy(file_location,file_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
		}
	}
	file_mo_list = data_copy_info.diff_time_mo_list;
	if (file_mo_list != NULL) {
		if (file_mo_list->num != 0) {
			if (options.ow_main_older == 1)
				results.ow_main_older = 1; // overwrite main older
			if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_older == 1) {
				if (options.less_detailed != 1)
					detailed_output(file_mo_list,SCREEN,string10,0);
				else if (options.less_detailed == 1) {
					printf("\nFiles to overwrite. (source location files older than destination)\n\n");
					for (file_list_element = file_mo_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
					}
				}
			}
			if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
				if (options.less_detailed != 1)
					detailed_output(file_mo_list,IN_FILE,string10,copyfile);
				else if (options.less_detailed == 1) {
					write(copyfile, string10, strlen(string10));
					for (file_list_element = file_mo_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
						strcpy(file_location,file_list_element->dir_location);
						strcat(file_location,"\n");
						write(copyfile, file_location, strlen(file_location));
					}
				}
			}
		}
	}

	if (options.ignore_symlinks != 1) {
		symlinks_ml_list = data_copy_info.symlinks_diff_size_ml_list;
		if (symlinks_ml_list != NULL) {
			if (symlinks_ml_list->num != 0) {
				if (options.ow_main_larger == 1)
					results.ow_symlinks_main_larger = 1; // overwrite main larger
				if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_larger == 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_ml_list,SCREEN,string12,0);
					else if (options.less_detailed == 1) {
						printf("\nSymbolic links to overwrite. (source location files larger than destination)\n\n");
						for (file_list_element = file_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
					}
				}
				if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_ml_list,IN_FILE,string12,copyfile);
					else if (options.less_detailed == 1) {
						write(copyfile, string12, strlen(string12));
						for (file_list_element = symlinks_ml_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							strcpy(file_location,file_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
			}
		}
	}
	
	if (options.ignore_symlinks != 1) {
		symlinks_ms_list = data_copy_info.symlinks_diff_size_ms_list;		
		if (symlinks_ms_list != NULL) {
			if (symlinks_ms_list->num != 0) {
				if (options.ow_main_smaller == 1)
					results.ow_symlinks_main_smaller = 1; // overwrite main smaller
				if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_smaller == 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_ms_list,SCREEN,string11,0);
					else if (options.less_detailed == 1) {
						printf("\nSymbolic links to overwrite. (source location files smaller than destination)\n\n");
						for (file_list_element = file_ms_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
					}
				}
				if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_ms_list,IN_FILE,string11,copyfile);
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
		}
	}
	
	if (options.ignore_symlinks != 1) {
		symlinks_mn_list = data_copy_info.symlinks_diff_time_mn_list;
		if (symlinks_mn_list != NULL) {
			if (symlinks_mn_list->num != 0) {
				if (options.ow_main_newer == 1)
					results.ow_symlinks_main_newer = 1; // overwrite main smaller
				if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_newer == 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_mn_list,SCREEN,string13,0);
					else if (options.less_detailed == 1) {
						printf("\nSymbolic links to overwrite. (source location files newer than destination)\n\n");
						for (file_list_element = file_mn_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
					}
				}
				if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_mn_list,IN_FILE,string13,copyfile);
					else if (options.less_detailed == 1) {
						write(copyfile, string13, strlen(string13));
						for (file_list_element = symlinks_mn_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							strcpy(file_location,file_list_element->dir_location);
							strcat(file_location,"\n");
							write(copyfile, file_location, strlen(file_location));
						}
					}
				}
			}
		}
	}
	
	if (options.ignore_symlinks != 1) {
		symlinks_mo_list = data_copy_info.symlinks_diff_time_mo_list;
		if (symlinks_mo_list != NULL) {
			if (symlinks_mo_list->num != 0) {
				if (options.ow_main_older == 1)
					results.ow_symlinks_main_older = 1; // overwrite main larger
				if (options.dont_list_data_to_copy != 1 && options.list_conflicting == 1 || options.dont_list_data_to_copy != 1 && options.ow_main_older == 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_mo_list,SCREEN,string14,0);
					else if (options.less_detailed == 1) {
						printf("\nSymbolic links to overwrite. (source location files older than destination)\n\n");
						for (file_list_element = file_mo_list->head; file_list_element != NULL; file_list_element = file_list_element->next) {
							printf("file: %s\n location: %s\n", file_list_element->name, file_list_element->dir_location);
						}
					}
				}
				if (options.copy_content_file == 1 || options.just_copy_content_file == 1) {
					if (options.less_detailed != 1)
						detailed_output(symlinks_mo_list,IN_FILE,string14,copyfile);
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
		}
	}
}
