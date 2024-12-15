	char *help_string1 = "--copy-extraneous-back or -b";
	char *help_string2 = "Copy the extraneous data from the secondary location into the main location while synchronizing the directories.";
	char *help_string3 = "--delete-extraneous or -x";
	char *help_string4 = "Delete the extraneous data from the secondary location while synchronizing the directories.";
	char *help_string5 = "--overwrite-with-smaller or -s";
	char *help_string6 = "If two files with the same name are found, overwrite the larger file in the secondary location with the smaller from the main location.";
	char *help_string7 = "--overwrite-with-larger or -l";
	char *help_string8 = "If two files with the same name are found, overwrite the smaller file in the secondary location with the larger file from the main location.";
	char *help_string9 = "--list-conflicting or -L";
	char *help_string10 = "List files with the same name, but different size or modification time.";
	char *help_string11 = "--list-extraneous or -e";
	char *help_string12 = "Just list extraneous files and directories, but dont copy them.";
	char *help_string13 = "--dont-list-data-to-copy or -g";
	char *help_string14 = "Don't list the files and directories to copy after scaning.";
	char *help_string15 = "--help or -h";
	char *help_string16 = "Show help and options.";
	char *help_string17 = "--version or -v";
	char *help_string18 = "Show version of the program.";
	//char *help_string19 = "--just-content-file=[FILE] or -C";
	//char *help_string20 = "Just write the content of both directories to a file and exit the program.";
	char *help_string21 = "--copy-content-file=[FILE] or -k";
	char *help_string22 = "Write the files and directories to copy into a file.";
	char *help_string23 = "--just-copy-content-file=[FILE] or -K";
	char *help_string24 = "Just write the files and directories to copy into a file and exit the program.";
	char *help_string25 = "--dont-list-stats or -G";
	char *help_string26 = "Don't list statistics about the file and directory size, number, etc.";
	char *help_string27 = "--dont-quit-read-errors or -y";
	char *help_string28 = "Don't quit on read errors. Useful for unexpected permissions on a file or directory.";
	char *help_string29 = "--dont-quit-write-errors or -z";
	char *help_string30 = "Don't quit on write errors. Useful for unexpected permissions on a file or directory.";
	char *help_string31 = "--dont-quit-delete-errors or -p";
	char *help_string32 = "Don't quit if deleting the file or directory fails. Useful for unexpected permissions on a file or directory.";
	char *help_string33 = "--no-questions or -q";
	char *help_string34 = "Don't ask for confirmation to write the data.";
	char *help_string35 = "--unit=OPTION";
	char *help_string36 = "Show sizes in unit of a choice (KB, MB, GB, TB, example: --unit=MB) insted of the default unit appropriate for the size.";
	char *help_string37 = "--si-units";
	char *help_string38 = "Use powers of 1000 instead of the default 1024";
	char *help_string39 = "--dont-show-read-process or -r";
	char *help_string40 = "Don't list files and directories currently reading.";
	char *help_string41 = "--dont-show-write-process or -w";
	char *help_string42 = "Don't list files and directories currently writing.";
	char *help_string43 = "--just-copy-extraneous-back or -B";
	char *help_string44 = "Just copy the extraneous data from the secondary location into the main location, but don't synchronize directories.";
	char *help_string45 = "--follow-sym-links or -F";
	char *help_string46 = "Follow symbolic links.";
	char *help_string47 = "--no-access-time or -a";
	char *help_string48 = "Do not update the last access time on files in the source directory during copying.";
	char *help_string49 = "--preserve-atime or -A";
	char *help_string50 = "Preserve access time on the data to be copied.";
	char *help_string51 = "--preserve-mtime or -M";
	char *help_string52 = "Preserve modification time on the data to be copied.";
	char *help_string53 = "--overwrite-with-newer or -N";
	char *help_string54 = "If two files with the same name are found, overwrite the older file in the secondary location with the newer file from the main location.";
	char *help_string55 = "--overwrite-with-older or -O";
	char *help_string56 = "If two files with the same name are found, overwrite the newer file in the secondary location with the older file from the main location.";
	char *help_string57 = "--ignore or -I";
	char *help_string58 = "Ignore named files or directories in the top directory during scanning/copying. Example: -I dir1 or -I dir1,dir2,dir3,file1,file2.";
	char *help_string59 = "--size-mode or -S";
	char *help_string60 = "Scan based on size difference instead of modification time.";
	char *help_string61 = "--just-delete-extraneous or -X";
	char *help_string62 = "Just delete the extraneous data from the secondary location.";
	char *help_string63 = "--preserve-perms or -P";
	char *help_string64 = "Preserve the permissions during copying.";
	char *help_string71 = "--less-detailed or -D";
	char *help_string72 = "Don't show detailed information about each file (size, permissions, etc) for copy list or copy content file.";
	char *help_string65 = "--acls";
	char *help_string66 = "Preserve ACLs during copying.";
	char *help_string67 = "--xattrs";
	char *help_string68 = "Preserve extended attributes during copying.";
	char *help_string69 = "--ignore-symlinks or -i";
	char *help_string70 = "Ignore (don't copy) symbolic links.";