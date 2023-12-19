cps compares and syncronizes two directories by copying only the missing files and directories, but it will notify you in the statistical info about the surplus files and directories
(those that exist in the directory2 (destination), but not in the directory1 (source), files with the same name but different size, and with the proper options, these files and directories can be copied, overwritten or deleted as well.

Before any writing operation begins, cps lists all the files and directories that are about to be copied, and asks you for the confirmation. It also allows you to create the text file with all the files and 
directories that are about to be copied without actually copying anything, as well as creating a text file with the complete file and directory content of both directory1 and directory2. You can also copy
surplus files and directories without synchronizing the source and destination.

For help, run the program without any arguments or --help, -h.

Build and install:
```
./configure
make
sudo make install
```

Some examples of use:

1. To copy missing files and directories from (directory1) or /home/user/Documents/photos/2023 to (directory2) or /mnt/backup/photos/2023. The program doesn't differentiate whether pathname ends with / or not.

```
cps /home/user/Documents/photos/2023 /mnt/backup/photos/2023
```
When you are asked if you want to copy missing files and directories, answer yes to begin copying operation or no if you want to quit.

2. To copy missing files and directories from source (directory1) to destination (directory2) and to also copy missing files and directories from destination (directory2) to source (directory1)
```
cps --copy-surplus-back /home/user/Documents/dir1 /home/user/Documents/dir2
```
When you are asked if you want to copy missing files and directories, answer yes to begin copying operation or no if you want to quit. Then you will be asked: Do you want to write the surplus data from the destination directory back to the source? 
Again, type yes or no. 

Surplus files and directories are identified in the statistical output of the program even during default operation of the program, for example:
```
Number of surplus files: 464
Size of surplus files in bytes: 627106671
Size in megabytes: 598.06 MB
Number of surplus directories: 0
Size of surplus directories in bytes: 0
```
This way you know that directory2 (or destination) contains surplus data, and you can ignore it, copy it back to a main directory or delete it.

If you want list out such files without neccesarily copying them use --list-surplus option.

3. To copy missing files and directories from /home/user/Documents/dir1  to /home/user/Documents/dir2 and to delete files and directories from /home/user/Documents/dir2 that don't exist in /home/user/Documents/dir1
```
cps --delete-surplus /home/user/Documents/dir1 /home/user/Documents/dir2
```
When you are asked if you want to copy missing files and directories, answer yes or no if you don't want to. Then you will be asked: Do you want to delete the surplus data from the destionation directory back to the source? 
Answer yes, or no if you don't want to.

4. If you want to add new data from a directory2 (destination) to a directory1 (source) without copying (syncing) data to a directory2 from a directory1, specify --just-copy-surplus-back. 
```
cps --just-copy-surplus-back /home/user/Documents/dir1 /home/user/Documents/dir2
```
You can also take longer route:
```
cps --copy-surplus-back /home/user/Documents/dir1 /home/user/Documents/dir2
```
When you are asked if you want to copy missing files and directories, answer no. Then, when you are asked: Do you want to write the surplus data from the destionation directory back to the source? Answer yes. This is useful if you
change your mind, and decide not to fully syncronize directories.

5. cps copies only the missing files and directories from directory1 to a directory2 and ignores the files with the same name, but different size. If you want to overwrite such files, you must specify whether you want to overwrite the smaller 
files from the directory2 (destination) with the larger files from directory1 (source). 
```
cps --overwrite-with-larger /home/user/Documents/dir1 /home/user/Documents/dir2
```
or if you want to overwrite the larger files from the directory2 (destination) with the smaller files from the directory1 (source):
```
cps --overwrite-with-smaller /home/user/Documents/dir1 /home/user/Documents/dir2
```
Such files and directories are identified in the statistical output of the program even during default operation of the program, for example:
```
Same files with different size (main location smaller): 6
Same files with different size (main location larger): 7
```
6. If you don't want to be asked whether or not you want to copy data, you can specify --no-questions or -q option. cps then proceeds with copying immediately after scaning the directories.
```
cps -q /home/user/Documents/dir1 /home/user/Documents/dir2
```
7. If you are not entirely sure what needs or what will be copied, you can just print out the list of files that will be copied, without actually coping anything. It is also useful in case there are many files and directories listed out
and screen scrolls too fast. Specify the full pathname for the location of the copy-content file + name of the file.

option --just-copy-content-file=/path/to/file. /path/to/file is a desired pathname of your choice.
```
cps --just-copy-content-file=/home/user/Documents/copy_content /home/user/Documents/dir1 /home/user/Documents/dir2
```
You can also copy data and create file with the content of all directories copied. In that case use:
```
cps --copy-content-file=/home/user/copy_content /home/user/Documents/dir1 /home/user/Documents/dir2
```
8. You can list complete content of files and directories from both directory1 (source) and directory2 (destination) without copying anything. use option --just-conent-file=/path/to/file where /path/to/file is a desired pathname of your choice.
```
cps --just-content-file=/home/user/dir_content /home/user/Documents/dir1 /home/user/Documents/dir2
```
You can also copy/syncronize the data and create a file with a complete directory content of both source and destination directories.
```
cps --content-file=/home/user/dir_content /home/user/Documents/dir1 /home/user/Documents/dir2
```
9. By default, the program outputs all files and directories to copy along with their current pathname, new pathname and size in bytes. If you don't want any of that, use:
```
cps --dont-list-data-to-copy /home/user/Documents/dir1 /home/user/Documents/dir2
```
10. By default, if program encounters a file or directory for which it doesn't have read or write permissions, it will quit. To skip over such files for read and write errors, using short options, use:
```
cps -no /home/user/Documents/dir1 /home/user/Documents/dir2
```
11. By default, program outputs sizes in bytes and another appropriate unit determined by the program. If you want to choose different second unit, use --unit option. For example, to output the size in megabytes:
```
cps --unit=MB /home/user/Documents/dir1 /home/user/Documents/dir2
```



General view of the program after directory scan:

```
SOURCE DIRECTORY

Number of files: 4124
Number of directories (excluding the top directory): 393
Size of directory in bytes: 1409287736
Size in gigabytes: 1.31 GB


DESTINATION DIRECTORY

Number of files: 2430
Number of directories (excluding the top directory): 264
Size of directory in bytes: 1190539251
Size in gigabytes: 1.11 GB


Number of individual files to copy: 1694
Size of individual files to copy in bytes: 132287675
Size in megabytes: 126.16 MB
Number of directories to copy: 129
Size of directories to copy in bytes: 86460810
Size in megabytes: 82.46 MB
Files and directories to copy, Size in megabytes: 208.61 MB
Number of surplus files: 0
Size of surplus files in bytes: 0
Number of surplus directories: 0
Size of surplus directories in bytes: 0
Same files with different size (main location smaller): 0
Same files with different size (main location larger): 0
Files with the same name, different type (main): 0
Files with the same name, different type (secondary): 0

Do you want to write the missing files and directories? Type yes or no ...
```
