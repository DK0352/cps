![screenshot1](screenshot/scr1.jpg)

cps compares and syncronizes two directories by copying only the missing files and directories, but also enables you to overwrite the same files of different size or different last modification time, as well as to copy or delete any extraneous data. It provides very useful information in it's statistical output after scanning and enables you to see the list and size of the files and directories to be copied before actually commencing the copying operation. It also allows you to create a text file with the list of all the files and directories that are about to be copied without necesarrily copying anything. 

By default, the program compares files based on modification time, but you can suppy the option to compare them based on their size. 

For help, run the program without any arguments or --help, -h.

Build and install:
```
./configure
make
sudo make install
```

Some examples of use:

```
cps OPTIONS directory1 directory2
```

1. To copy missing files and directories from **directory1** (the main directory or source) to **directory2** (the secondary directory or destination), you don't have to supply any options. For example:

```
cps /home/user/Documents/photos/2023 /mnt/backup/photos/2023
```
After you are asked if you want to copy missing files and directories, answer yes to begin copying operation or no if you want to quit. Use the -q option if you don't want to be asked for the confirmation. 

The program doesn't differentiate whether the pathnames end with a '/' or not. Whether you type **/mnt/backup/photos/2023** or **/mnt/backup/photos/2023/** - it is the same.

2. To copy missing files and directories from **directory1** (the main directory or source) to **directory2** (the secondary directory or destination) and to also copy any extraneous files and directories from **directory2** to **directory1**
```
cps --copy-extraneous-back /home/user/Documents/dir1 /home/user/Documents/dir2
```
or using the short option:
```
cps -b /home/user/Documents/dir1 /home/user/Documents/dir2
```
After you are asked if you want to copy missing files and directories, answer yes to begin copying operation or no if you want to quit. Then you will be asked: Do you want to write the extraneous data from the destination directory back to the source? 
Again, type yes or no if you changed your mind. Typing 'no' to a second question won't abort the result of the previous question/operation. 

Extraneous files and directories are identified in the statistical output of the program even during default operation of the program, for example:
```
Number of individual extraneous files: 0   0 bytes  0 bytes
Number of extraneous symbolic links: 0   0 bytes  0 bytes
Number of extraneous directories: 217   51.32 MB  53818020 bytes
```
This way you know that the **secondary directory or destination** contains extraneous data, and you can ignore it, copy it back to a **main directory** or delete it.

If you want list out such files without neccesarily copying them use --list-extraneous or -e option.

3. To copy missing files and directories from **/home/user/Documents/dir1** to **/home/user/Documents/dir2** and to delete files and directories from **/home/user/Documents/dir2** that don't exist in 
**/home/user/Documents/dir1**:
```
cps --delete-extraneous /home/user/Documents/dir1 /home/user/Documents/dir2
```
or using the short option:
```
cps -x /home/user/Documents/dir1 /home/user/Documents/dir2
```
After you are asked if you want to copy missing files and directories, answer yes or no if you don't want to. Then you will be asked: Do you want to delete the extraneous data from the destionation directory? 
Answer yes, or no if you don't want to. Typing 'no' to a second question won't abort the result of the previous question/operation. 

4. If you want to add new data from a directory2 (destination) to a directory1 (source) without copying (syncing) data to a directory2 from a directory1, specify --just-copy-extraneous-back or -B. 
```
cps --just-copy-extraneous-back /home/user/Documents/dir1 /home/user/Documents/dir2
```
or using the short option:
```
cps -B /home/user/Documents/dir1 /home/user/Documents/dir2
```

5. cps copies only the missing files and directories from directory1 to a directory2 and ignores the files with the same name, but different modification time or different size (in case you use -S or --size-mode option). 
To list out any such files and directories, use:
```
cps --list-conflicting /home/user/Documents/dir1 /home/user/Documents/dir2
```
or using the short option:
```
cps -L /home/user/Documents/dir1 /home/user/Documents/dir2
```
6. If you want to overwrite the files based on modification time:

Such files are identified in the statistical output of the program:

```
Same files with different modification time (main location newer): 22
Same files with different modification time (main location older): 1599
```

To overwrite the older modification time files in the secondary/destination location with the newer files from the main/source location, use --overwrite-with-newer or -N option:
```
cps -N /home/user/Documents/dir1 /home/user/Documents/dir2
```

To overwrite the older modification time files in the secondary/destination location with the newer files from the main/source location, use --overwrite-with-older or -O option:
```
cps -O /home/user/Documents/dir1 /home/user/Documents/dir2
```

7. By default, the program compares files based on their last modification time. To compare then based on the size, use --size-mode or -S option:

```
cps -S /home/user/Documents/dir1 /home/user/Documents/dir2
```
If you want to overwrite the smaller files from the directory2 (destination) with the larger files from directory1 (source):
```
cps --overwrite-with-larger /home/user/Documents/dir1 /home/user/Documents/dir2
```
using the short option:
```
cps -l /home/user/Documents/dir1 /home/user/Documents/dir2
```
or if you want to overwrite the larger files from the directory2 (destination) with the smaller files from the directory1 (source):
```
cps --overwrite-with-smaller /home/user/Documents/dir1 /home/user/Documents/dir2
```
using the short option:
```
cps -s /home/user/Documents/dir1 /home/user/Documents/dir2
```
Such files are identified in the statistical output of the program:
```
Same files with different size (main location smaller): 6
Same files with different size (main location larger): 7
```
8. Time options:

To preserve the last access time on the data to be copied use --preserve-atime or -A
```
cps -A /home/user/Documents/dir1 /home/user/Documents/dir2
```
To preserve the last modification time on the data to be copied use --preserve-mtime or -M
```
cps -M /home/user/Documents/dir1 /home/user/Documents/dir2
```
To avoid updating the last access time on files during reading operation use --no-access-time or -a
```
cps -a /home/user/Documents/dir1 /home/user/Documents/dir2
```

9. If you don't want to be asked whether or not you want to copy/overwrite/delete data, you can specify --no-questions or -q option. cps then proceeds with the desired operations immediately after scaning the directories.
```
cps -q /home/user/Documents/dir1 /home/user/Documents/dir2
```
10. If you only want to know which files and directories will be copied, you can just print out the list of files that will be copied without actually coping anything. It is also useful in case there are many files and directories listed out 
and the screen scrolls too fast. Specify the full pathname for the location of the copy-content file and name of the file.

option --just-copy-content-file=/path/to/file. /path/to/file is a desired pathname of your choice, or using short option -K
```
cps --just-copy-content-file=/home/user/Documents/copy_list /home/user/Documents/dir1 /home/user/Documents/dir2
```
or using the short option:
```
cps -K /home/user/Documents/copy_list /home/user/Documents/dir1 /home/user/Documents/dir2
```
You can also copy data and create file with the content of all files and directories copied. In that case use:
```
cps --copy-content-file=/home/user/copy_content /home/user/Documents/dir1 /home/user/Documents/dir2
```
or using short option:
```
cps -k /home/user/Documents/copy_list /home/user/Documents/dir1 /home/user/Documents/dir2
```

11. By default, the program outputs all files and directories to copy along with their current pathname, new pathname and size in bytes. If you don't want any of that, use --dont-list-data-to-copy or -d:
```
cps -d /home/user/Documents/dir1 /home/user/Documents/dir2
```
12. By default, if program encounters a file or directory for which it doesn't have read or write permissions, it will quit. To skip over such files for read and write errors using the short option, use:
```
cps -yz /home/user/Documents/dir1 /home/user/Documents/dir2
```
 
13. By default, program outputs sizes in bytes and another appropriate unit determined by the program. If you want to choose different second unit, use --unit option. For example, to output the size in megabytes:
```
cps --unit=MB /home/user/Documents/dir1 /home/user/Documents/dir2
```

14. To follow symbolic links and to copy the actual data these symbolic links point to, use --follow-sym-links or -S:
```
cps -S /home/user/Documents/dir1 /home/user/Documents/dir2
```
To ignore symbolic links during copying, use the -i or --ignore-symlinks option.

General view of the program after a directory scan:

![screenshot1](screenshot/scr2.jpg)

