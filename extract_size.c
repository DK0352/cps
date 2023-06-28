#include "main.h"
#include "dlist.h"

void extract_size(DList_of_lists *file_tree_element) 
{
	DList_of_lists		*hold_pos, *hold_for_up;

	if (file_tree_element->this_is_top_dir != 1) {

		hold_for_up = file_tree_element->up;
		file_tree_element = file_tree_element->first_dir_in_chain;

		while (file_tree_element->last_dir != 1) {
			hold_for_up->subdir_file_num += file_tree_element->file_num;
			hold_for_up->subdir_file_num += file_tree_element->subdir_file_num;
			hold_for_up->complete_file_num += file_tree_element->file_num;
			hold_for_up->complete_file_num += file_tree_element->subdir_file_num;
			hold_for_up->subdirs_size += file_tree_element->complete_dir_size;
			hold_for_up->complete_dir_size += file_tree_element->complete_dir_size;
			hold_for_up->subdir_num += file_tree_element->dir_num;
			hold_for_up->subdir_num += file_tree_element->subdir_num;
			hold_for_up->complete_dir_num += file_tree_element->dir_num;
			hold_for_up->complete_dir_num += file_tree_element->subdir_num;
			file_tree_element = file_tree_element->next;
		}
		if (file_tree_element->last_dir == 1) {
			hold_for_up->subdir_file_num += file_tree_element->file_num;
			hold_for_up->subdir_file_num += file_tree_element->subdir_file_num;
			hold_for_up->complete_file_num += file_tree_element->file_num;
			hold_for_up->complete_file_num += file_tree_element->subdir_file_num;
			hold_for_up->subdirs_size += file_tree_element->complete_dir_size;
			hold_for_up->complete_dir_size += file_tree_element->complete_dir_size;
			hold_for_up->subdir_num += file_tree_element->dir_num;
			hold_for_up->subdir_num += file_tree_element->subdir_num;
			hold_for_up->complete_dir_num += file_tree_element->dir_num;
			hold_for_up->complete_dir_num += file_tree_element->subdir_num;
		}
	}
	else if (file_tree_element->this_is_top_dir == 1) {

		hold_pos = file_tree_element->this_directory;
		file_tree_element = file_tree_element->first_dir_in_chain;

		while (file_tree_element->last_dir != 1) {
			hold_pos->subdir_file_num += file_tree_element->file_num;
			hold_pos->subdir_file_num += file_tree_element->subdir_file_num;
			hold_pos->complete_file_num += file_tree_element->file_num;
			hold_pos->complete_file_num += file_tree_element->subdir_file_num;
			hold_pos->subdirs_size += file_tree_element->complete_dir_size;
			hold_pos->complete_dir_size += file_tree_element->complete_dir_size;
			hold_pos->subdir_num += file_tree_element->dir_num;
			hold_pos->subdir_num += file_tree_element->subdir_num;
			hold_pos->complete_dir_num += file_tree_element->dir_num;
			hold_pos->complete_dir_num += file_tree_element->subdir_num;
			file_tree_element = file_tree_element->next;
		}
		if (file_tree_element->last_dir == 1) {
			hold_pos->subdir_file_num += file_tree_element->file_num;
			hold_pos->subdir_file_num += file_tree_element->subdir_file_num;
			hold_pos->complete_file_num += file_tree_element->file_num;
			hold_pos->complete_file_num += file_tree_element->subdir_file_num;
			hold_pos->subdirs_size += file_tree_element->complete_dir_size;
			hold_pos->complete_dir_size += file_tree_element->complete_dir_size;
			hold_pos->subdir_num += file_tree_element->dir_num;
			hold_pos->subdir_num += file_tree_element->subdir_num;
			hold_pos->complete_dir_num += file_tree_element->dir_num;
			hold_pos->complete_dir_num += file_tree_element->subdir_num;
		}
		hold_pos->complete_file_num += hold_pos->subdir_file_num;
	}
}
