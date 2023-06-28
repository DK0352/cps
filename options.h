struct options_menu {
	int	pause_read_errors;		// if read error occurs, pause any operations; the default is ignore read/write error, but mark all such files and directories
	int	pause_write_errors;		// if write error occurs, pause any operations; the default is to quit on read error.
	int	quit_read_errors;		// if read error occurs, if 1 (default) quit the program. if 0, don't quit the program.
	int	quit_write_errors;		// if write error occurs, if 1 (default) quit the program. if 0, don't quit the program.
	int	quit_delete_errors;		// if delete error occurs, if 1 (default) quit the program. if 0, don't quit the program.
	int	ignore_read_errors;		// ignore read errors. files or dirs that failed to be read will be added to list.
	int	ignore_write_errors;		// ignore write errors. files or dirs that failed to be written will be added to list.
	int	copy_surplus_back;		// copy the surplus from the secondary location into the main
	int	delete_surplus; 		// delete surplus files from main location after equating data with secondary location
	int	ow_main_smaller;		// if two files with the same name are found, overwrite the larger file in the secondary location with the smaller from the main location
	int	ow_main_larger;			// if two files with the same name are found, overwrite the smaller file in the secondary location with the larger file from the main location
	int	ow_type_main;			// overwrite the secondary location file type with the main location file type.
	int	list_surplus;			// just list surplus files/dirs, but dont copy them.
	int	dont_list_data_to_copy;		// don't list files and directories to copy
	int	no_questions;			// don't ask for confirmation whether to copy/write data
	int	help;				// show help
	int	write_content_file;		// write file trees of both directories into file after copying
	int	just_write_content_file;	// write file trees of both directories, but don't copy anything
	int	write_copy_content_file;	// write copyied files and directories into the file
	int	just_write_copy_content_file;	// just write files and directories to copy into the file, bud don't copy anything
	int	dont_list_stats;		// dont list statistics about files and dirs, their size, number, etc...
	int	other_unit;			// if 1, use nondefault, user specified size unit (KB, MB, GB)
	int	si_units;			// if 1, use multiples of 1000 insted of 1024 for units (KB, MB, GB)
	char	unit[2];			// unit to display for size: KB, MB, GB
	//short	newer_mod_time;			// compare last modification time and copy the files that have been modified more recently
	//short	preserve_time;			// don't modify read/write time
	//short	preserve_attributes;		// don't modify file attributes
	//short	preserve_permissions;		// preserve permissions into new locatioin
	//short	retry_interval;			// if file cannot be read or written, retry operation every x seconds. default is =
	//short	retry_attempts;			// number of attempts to retry operation if unsuccessful. default is =
	//short suppress_all_output_msgs;	// supress all informative output messages, like the files, dires copied, etc...
	//short main_larger;			// by default, larger files in the main location will overwrite smaller files in the secodary location
	//short main_smaller;			// by default, smaller files in the main location will overwrite larger files in the secodary location
	//short ignore_diff_type;		// ignore same name - different type files
};
