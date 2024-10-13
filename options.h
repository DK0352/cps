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

// 
struct options_menu {
	int	pause_read_errors;		// if read error occurs, pause any operations; the default is ignore read/write error, but mark all such files and directories
	int	pause_write_errors;		// if write error occurs, pause any operations; the default is to quit on read error.
	int	quit_read_errors;		// if read error occurs, if 1 (default) quit the program. if 0, don't quit the program.
	int	quit_write_errors;		// if write error occurs, if 1 (default) quit the program. if 0, don't quit the program.
	int	quit_delete_errors;		// if delete error occurs, if 1 (default) quit the program. if 0, don't quit the program.
	int	ignore_read_errors;		// ignore read errors. files or dirs that failed to be read will be added to list.
	int	ignore_write_errors;		// ignore write errors. files or dirs that failed to be written will be added to list.
	int	copy_extraneous_back;		// copy extraneous data from secondary location into main along with syncing directories
	int	just_copy_extraneous_back;		// copy extraneous data from secondary location into main, without syncing directories
	int	delete_extraneous; 		// delete extraneous files and dirs from the destination directory
	int	just_delete_extraneous;		// just delete extraneous files and dirs from the destination directory and quit
	int	ow_main_smaller;		// if two files with the same name are found, overwrite the larger file in the secondary location with the smaller from the main location
	int	ow_main_larger;			// if two files with the same name are found, overwrite the smaller file in the secondary location with the larger file from the main location
	int	list_extraneous;			// just list extraneous files/dirs, but dont copy them.
	int	dont_list_data_to_copy;		// don't list files and directories to copy
	int	no_questions;			// don't ask for confirmation whether to copy/write data
	int	show_help;
	int	show_version;
	int	write_content_file;		// write file trees of both directories into file after copying
	int	just_write_content_file;	// write file trees of both directories, but don't copy anything
	int	write_copy_content_file;	// write copyied files and directories into the file
	int	just_write_copy_content_file;	// just write files and directories to copy into the file, bud don't copy anything
	int	dont_list_stats;		// dont list statistics about files and dirs, their size, number, etc...
	int	other_unit;			// if 1, use nondefault, user specified size unit (KB, MB, GB)
	int	si_units;			// if 1, use multiples of 1000 insted of 1024 for units (KB, MB, GB)
	int	show_read_proc;			// show read process (listing file or directory currently reading) if 1
	int	show_write_proc;		// show write process (listing file or directory currently writing) if 1
	char	unit[2];			// unit to display for size: KB, MB, GB
	int	follow_sym_links;		// follow symbolic links if 1
	int	(*stat_f)(const char *pathname, struct stat *statbuf);		// pointer to stat() or lstat() function depending on follow_sym_links option
	int	open_flags;			// flags for open() syscall
	int	noatime;			// do not update the file last access time when the file is read
	int	time_mods;			// some timestamp options are enabled
	struct	timespec times[2];		// for use with utimensat()
	int	preserve_a_time;		// preserve access times
	int	preserve_m_time;		// preserve modification times
	int	time_based;			// decide what to copy based on last modification time
	int	size_based;			// decide what to copy based on size
	int	ow_main_newer;			// overwrite older files with newer
	int	ow_main_older;			// overwrite newer files with older
	int	naive_mode;			// go deeper into file tree only if there is difference in size
	int	list_conflicting;		// files with the same name, but different size or modification time
	int	less_detailed;			// 
	int	preserve_perms;			// preserve file permissions during the copying
	int	acls;				// preserve ACLs (Access Control Lists)
	int	xattrs;				// preserve extended attributes
	int	ignore_symlinks;		// don't copy symbolic links
	int	ignore;				// ignore named files and directories in the top dir during scanning
	DList	*ignore_list;			// file and directories to ignore in the top directory
	int	(*setxattr_func)(const char *path, const char *name, const void *value, size_t size, int flags);
	ssize_t	(*getxattr_func)(const char *path, const char *name, void *value, size_t size);
	ssize_t	(*listxattr_func)(const char *path, char *list, size_t size);
};
