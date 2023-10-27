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

		hold_pos = file_tree_element->file_tree_top_dir;
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
