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

struct errors_data {
	unsigned long	file_open_error_count;
	unsigned long	file_create_error_count;
	unsigned long 	file_close_error_count;
	unsigned long 	file_delete_error_count;
	unsigned long	dir_open_error_count;
	unsigned long	dir_close_error_count;
	unsigned long	dir_delete_error_count;
	unsigned long	file_read_error_count;
	unsigned long 	file_write_error_count;
	unsigned long 	file_overwrite_error_count;
	unsigned long	dir_read_error_count;
	unsigned long	dir_create_error_count;
	unsigned long	file_attr_error_count;
	unsigned long	dir_attr_error_count;
	unsigned long	symlink_read_error_count;
	unsigned long	symlink_write_error_count;
	unsigned long	symlink_overwrite_error_count;
	unsigned long
	DList *file_read_error, *file_write_error, *dir_read_error, *dir_write_error;
};
