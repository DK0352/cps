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
	long	file_open_errors;
	long	file_read_errors;
	long	symlink_read_errors;
	long 	file_write_errors;
	long 	symlink_write_errors;
	long	dir_read_errors;
	long	dir_create_errors;
	long	file_perms_errors;
	long	dir_perms_errors;

	long	set_atime_errors;
	long	set_mtime_errors;
	//long	read_attr_errors;
	//long	set_attr_errors;
	long	read_xattr_errors;
	long	set_xattr_errors;
	long	get_acl_errors;
	long	set_acl_errors;

	int	acl_malloc_fatal;	// to be able to inspect -1 value

	// new option: quit on attribute set error??
	DList *file_read_error, *file_write_error, *dir_read_error, *dir_create_error;
	DList *symlink_read_error, *symlink_write_error;
};
