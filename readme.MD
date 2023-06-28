cps compares and syncronizes two directories by copying only the missing files and directories from directory1 to a directory2, but it will notify you in the statistical info about surplus files
(files that exist in the directory2, but not in the directory1), files with the same name but different size, and with the proper options, those files can be copied or overwritten as well.

Before any writing operation begins, cps lists all the files and directories that are about to be copied, and asks you for the confirmation. It also allows you to create the text file with all the files and 
directories that are about to be copied without actually copying anything, as well as creating the text file with the complete file and directory content of both directory1 and directory2.

For help, run the program without any arguments or --help, -h.

Some examples of use:

1. To copy missing files and directories from (directory1) or /home/user/Documents/photos to (directory2) or /mnt/backup/photos 

or
```
./cps /home/user/Documents/photos /mnt/backup/photos
```
After you are asked if you want to copy missing files and directories, answer yes or no if you want to quit.

2. To copy missing files and directories from (directory1) to (directory2) and to also copy missing files and directories from (directory2) to (directory1)
```
./cps --copy-surplus-back /home/user/Documents/dir1 /home/user/Documents/dir2
```
After you are asked if you want to copy missing files and directories, answer yes or no if you don't want to. Then you will be asked: Do you want to write the surplus data from the destionation directory back to the source? 
Answer yes, or no if you don't want to. 

Surplus files and directories are identified in the statistical output of the program even during default operation of the program, for example:
```
Number of surplus files: 464
Size of surplus files in bytes: 627106671
Size in megabytes: 598.06 MB
Number of surplus directories: 0
Size of surplus directories in bytes: 0
```
This way you know that directory2 contains surplus data, and you can ignore it, copy it back to a main directory or delete it.

If you want list out such files without copying them use --list-surplus option.

3. To copy missing files and directories from /home/user/Documents/dir1  to /home/user/Documents/dir2 and to delete files and directories from /home/user/Documents/dir2 that don't exist in /home/user/Documents/dir1
```
./cps --delete-surplus /home/user/Documents/dir1 /home/user/Documents/dir2
```
After you are asked if you want to copy missing files and directories, answer yes or no if you don't want to. Then you will be asked: Do you want to delete the surplus data from the destionation directory back to the source? 
Answer yes, or no if you don't want to.

4. If you want to add new data from a directory2 to a directory1 without copying (syncing) data to a directory2 from a directory1, specify --copy-surplus-back. 
```
./cps --copy-surplus-back /home/user/Documents/dir1 /home/user/Documents/dir2
```
After you are asked if you want to copy missing files and directories, answer no. Then when you are asked: Do you want to write the surplus data from the destionation directory back to the source? Answer yes.

5. cps copies only the missing files and directories from directory1 to a directory2 and ignores the files with the same name, but different size. If you want to overwrite such files: in case you want to overwrite the smaller files from
the directory2 with the larger files from directory1, specify --overwrite-larger. 
```
./cps --overwrite-larger /home/user/Documents/dir1 /home/user/Documents/dir2
```
If you want to overwrite the larger files from the directory2 with the smaller files from the directory1:
```
./cps --overwrite-smaller /home/user/Documents/dir1 /home/user/Documents/dir2
```
In case you want to overwrite the larger files from the directory2 with the smaller files from the directory1, specify --overwrite-smaller

Such files and directories are identified in the statistical output of the program even during default operation of the program, for example:
```
Same files with different size (main location smaller): 6
Same files with different size (main location larger): 7
```
6. If you don't want to be asked whether or not you want to copy data, you can specify --no-questions or -q option. cps then proceeds with copying immediately after scaning the directories.
```
./cps -q /home/user/Documents/dir1 /home/user/Documents/dir2
```
7. If you are not entirely sure what needs or what will be copied, you can just print out the list of files that will be copied. It is also usefull if there are many files and directories to copy that it doesn't fit the screen. Use 
option --just-copy-content-file=/path/to/file. /path/to/file is a desired pathname of your choice.
```
./cps --just-copy-content-file=/home/user/copy_content /home/user/Documents/dir1 /home/user/Documents/dir2
```
You can also copy data and create file with the content of all directories copied. In that case use:
```
./cps --copy-content-file=/home/user/copy_content /home/user/Documents/dir1 /home/user/Documents/dir2
```
8. You can list complete content of files and directories from both directory1 and directory2 without copying anything. use option --just-conent-file=/path/to/file. /path/to/file is desired pathname of your choice.
```
./cps --just-content-file=/home/user/dir_content /home/user/Documents/dir1 /home/user/Documents/dir2
```
You can also copy data and create a file with a complete directory content.
```
./cps --content-file=/home/user/dir_content /home/user/Documents/dir1 /home/user/Documents/dir2
```
9. By default, program outputs all files and directories to copy along with their current pathname, new pathname and size in bytes. If you don't want that:
```
./cps --dont-list-data-to-copy /home/user/Documents/dir1 /home/user/Documents/dir2
```
10. By default, if program encounters a file or directory for which it doesn't have read or write permissions, it will quit. To skip over such files for read and write errors, using short options, use:
```
./cps -no /home/user/Documents/dir1 /home/user/Documents/dir2
```
11. By default, program outputs sizes in bytes and another appropriate unit determined by the program. If you want to choose different second unit, use --unit option. For example, to output the size in megabytes:
```
./cps --unit=MB /home/user/Documents/dir1 /home/user/Documents/dir2
```
