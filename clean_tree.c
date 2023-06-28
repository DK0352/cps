#include "main.h"
#include "dlist.h"

int clean_tree(DList_of_lists *file_tree_element, short deeper) 
{
	DList_of_lists		*delete_element;
	static int		count;

	if (deeper == 0) {
		if (file_tree_element->last_dir_in_chain != NULL) {
			file_tree_element = file_tree_element->last_dir_in_chain;
		}
		else {
			if (file_tree_element == file_tree_element->this_directory) {
				if (file_tree_element->files != NULL)
					dlist_destroy(file_tree_element->files);
				if (file_tree_element->directories != NULL)
					dlist_destroy(file_tree_element->directories);
				free(file_tree_element);
			}
			return 0;
		}
		if (file_tree_element->down != NULL)
			clean_tree(file_tree_element->down,1);
		while (file_tree_element != file_tree_element->this_directory) {
			if (file_tree_element->files != NULL)
				dlist_destroy(file_tree_element->files);
			if (file_tree_element->directories != NULL)
				dlist_destroy(file_tree_element->directories);
			if (file_tree_element->next != NULL) {
				free(file_tree_element->next);
				file_tree_element->next = NULL;
			}
			if (file_tree_element->down != NULL) {
				clean_tree(file_tree_element->down,1);
			}
			file_tree_element = file_tree_element->prev;
		}
		if (file_tree_element == file_tree_element->this_directory) {
			if (file_tree_element->files != NULL)
				dlist_destroy(file_tree_element->files);
			if (file_tree_element->directories != NULL)
				dlist_destroy(file_tree_element->directories);
			if (file_tree_element->next != NULL) {
				free(file_tree_element->next);
				file_tree_element->next = NULL;
			}
			free(file_tree_element);
		}
		return 0;
	}
	else if (deeper == 1) {
		if (file_tree_element->last_dir_in_chain != NULL) {
			file_tree_element = file_tree_element->last_dir_in_chain;
		}
		else {
			delete_element = file_tree_element;
			file_tree_element = file_tree_element->up;
			file_tree_element->down = NULL;
			free(delete_element);
			return 0;
		}	
		if (file_tree_element->down != NULL) {
			clean_tree(file_tree_element->down,1);
		}
		while (file_tree_element != file_tree_element->this_directory) {
			if (file_tree_element->files != NULL)
				dlist_destroy(file_tree_element->files);
			if (file_tree_element->directories != NULL)
				dlist_destroy(file_tree_element->directories);
			if (file_tree_element->next != NULL) {
				free(file_tree_element->next);
				file_tree_element->next = NULL;
			}
			if (file_tree_element->down != NULL) {
				clean_tree(file_tree_element->down,1);
			}
			file_tree_element = file_tree_element->prev;
		}
		if (file_tree_element == file_tree_element->this_directory) {
			if (file_tree_element->files != NULL)
				dlist_destroy(file_tree_element->files);
			if (file_tree_element->directories != NULL)
				dlist_destroy(file_tree_element->directories);
			if (file_tree_element->next != NULL) {
				free(file_tree_element->next);
				file_tree_element->next = NULL;
			}
			delete_element = file_tree_element;
			file_tree_element = file_tree_element->up;
			file_tree_element->down = NULL;
			free(delete_element);
			return 0;
		}
	}
}
