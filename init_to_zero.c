#include "main.h"
#include "dlist.h"

void init_to_zero(DList_of_lists *file_tree_element)
{
	file_tree_element->file_num = 0;
	file_tree_element->subdir_file_num = 0;
	file_tree_element->complete_file_num = 0;
	file_tree_element->files_size = 0;
	file_tree_element->complete_files_size = 0;
	file_tree_element->dir_num = 0;
	file_tree_element->subdir_num = 0;
	file_tree_element->complete_dir_num = 0;
	file_tree_element->subdirs_size = 0;
	file_tree_element->complete_dir_size = 0;
	file_tree_element->last_dir = 0;
	file_tree_element->found_dir_match = 0;
	file_tree_element->one_of_the_top_dirs_num = 0;
	file_tree_element->this_is_top_dir = 0;
	file_tree_element->st_mode = 0;
	file_tree_element->dirname = NULL;
	file_tree_element->dir_location = NULL;
	file_tree_element->files = NULL;
	file_tree_element->directories = NULL;
	file_tree_element->sym_links = NULL;
	file_tree_element->sockets = NULL;
	file_tree_element->fifos = NULL;
	file_tree_element->ch_devs = NULL;
	file_tree_element->bl_devs = NULL;
	file_tree_element->files_to_copy = NULL;
	file_tree_element->dirs_to_copy = NULL;
	file_tree_element->files_surplus = NULL;
	file_tree_element->dirs_surplus = NULL;
	file_tree_element->diff_type = NULL;
	file_tree_element->diff_size = NULL;
	file_tree_element->diff_size_ml = NULL;
	file_tree_element->diff_size_ms = NULL;
	file_tree_element->one_of_the_top_dirs = NULL;
	file_tree_element->first_dir_in_chain = NULL;
	file_tree_element->last_dir_in_chain = NULL;
	file_tree_element->this_directory = NULL;
	file_tree_element->up = NULL;
	file_tree_element->down = NULL;
	file_tree_element->next = NULL;
	file_tree_element->prev = NULL;
}
