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

int clean_tree(DList_of_lists *file_tree_element, short deeper) 
{
	DList_of_lists		*delete_element;
	static int		count;

	if (deeper == 0) {
		if (file_tree_element->last_dir_in_chain != NULL) {
			file_tree_element = file_tree_element->last_dir_in_chain;
		}
		else {
			if (file_tree_element == file_tree_element->first_dir_in_chain) {
				if (file_tree_element->files != NULL)
					dlist_destroy(file_tree_element->files);
				if (file_tree_element->sym_links != NULL)
					dlist_destroy(file_tree_element->sym_links);
				if (file_tree_element->directories != NULL)
					dlist_destroy(file_tree_element->directories);
				free(file_tree_element);
			}
			return 0;
		}
		if (file_tree_element->down != NULL)
			clean_tree(file_tree_element->down,1);
		while (file_tree_element != file_tree_element->file_tree_top_dir) {
			if (file_tree_element->files != NULL)
				dlist_destroy(file_tree_element->files);
			if (file_tree_element->sym_links != NULL)
				dlist_destroy(file_tree_element->sym_links);
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
		if (file_tree_element == file_tree_element->file_tree_top_dir) {
			if (file_tree_element->files != NULL)
				dlist_destroy(file_tree_element->files);
			if (file_tree_element->sym_links != NULL)
				dlist_destroy(file_tree_element->sym_links);
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
		while (file_tree_element != file_tree_element->first_dir_in_chain) {
			if (file_tree_element->files != NULL)
				dlist_destroy(file_tree_element->files);
			if (file_tree_element->sym_links != NULL)
				dlist_destroy(file_tree_element->sym_links);
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
		if (file_tree_element == file_tree_element->first_dir_in_chain) {
			if (file_tree_element->files != NULL)
				dlist_destroy(file_tree_element->files);
			if (file_tree_element->sym_links != NULL)
				dlist_destroy(file_tree_element->sym_links);
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
