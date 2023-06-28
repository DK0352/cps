
/* doubly-linked list elements */
typedef struct DListElmt_ {
	char 				*name;		// name of a file or directory
	mode_t				st_mode;	// type and permissions
	unsigned long 			size;		// size of a file
	char				*dir_location;	// location of a file
	int				match;		// used in loop_files and loop_dirs function to signal that files has been compared
	char				*new_location;	// new location for a file in case it needs to be copied to a new location
	struct DListElmt_		*prev;
	struct DListElmt_		*next;
} DListElmt;

/* doubly-linked lists */
typedef struct DList_ {
	int		num;				// number of files in a directory
	long		files_size;			// size of all files in a directory
	DListElmt	*head;
	DListElmt	*tail;
} DList;

/* linked list of linked lists */
typedef struct DList_of_lists_ {
	unsigned long			file_num;		// number of files
	unsigned long			subdir_file_num;	// number of files in all subdirectories
	unsigned long			complete_file_num;	// complete file number for a directory including subdirectories
	unsigned long 			files_size;		// size of all files in the current directory 
	unsigned long 			complete_files_size;	// size of all files in all subdirectories
	unsigned long			dir_num;		// number of directories in the current directory 
	unsigned long			subdir_num;		// number of all subdirectories in the current directory
	unsigned long			complete_dir_num;	// dir_num + subdir_num
	unsigned long 			subdirs_size;		// size of all subdirectories in the current directory
	unsigned long			complete_dir_size;	// size of all files and subdirectories in the current directory
	int				last_dir;		// last directory in the linked list chain
	int				found_dir_match;	// found two directories with the same name. used in loop_dirs and compare_trees functions.
	int				one_of_the_top_dirs_num; // number of the top subdirectory
	int				this_is_top_dir;	// mark that this is a top subdirectory
	mode_t				st_mode;		// type/permission
	char				*source_pathname;	// source directory - is this needed?
	char				*destination_pathname;	// destination directory - is this needed?
	char				*dirname;		// name of directory
	char				*dir_location;		// directory location
	DList				*files;			// linked list of files
	DList				*directories;		// linked list of directories
	DList				*sym_links;		
	DList				*sockets;		
	DList				*fifos;
	DList				*ch_devs;
	DList				*bl_devs;
	DList				*files_to_copy;		// files to copy
	DList				*dirs_to_copy;		// directories to copy
	DList				*files_surplus;		// file surplus
	DList				*dirs_surplus;		// directories surplus
	DList				*diff_type;		// same name, different type
	DList				*diff_size;		// same name, different size
	DList				*diff_size_ml;		// same name, different size, main larger
	DList				*diff_size_ms;		// same name, different size, main smaller
	struct DList_of_lists_		*one_of_the_top_dirs;	// points to the top subdirectory of the file tree
	struct DList_of_lists_		*first_dir_in_chain;	// first directory in a list of directories in a file tree
	struct DList_of_lists_		*last_dir_in_chain;	// last directory in a list of directories in a file tree
	struct DList_of_lists_		*this_directory;	// first element in the list of directories in the current subdirectory. if you specify directory->down on some directory, this is the element you will get to.
	struct DList_of_lists_		*up;			// to go up the directory hierarchy
	struct DList_of_lists_		*down;			// to go down the directory hierarchy
	struct DList_of_lists_		*next;			// next directory in a list
	struct DList_of_lists_		*prev;			// previous directory in a list
} DList_of_lists;

struct thread_struct {
	char			*id;		/* source or destination pathname */
	char			*directory;	/* location */
	DList			*files;		/* list of files */
	DList			*directories;	/* list of directories */
	DList			*sym_links;
	DList			*sockets;
	DList			*fifos;
	DList			*ch_devs;
	DList			*bl_devs;
	int			(*file_function)(void *data1, void *data2); /* stat or lstat function depending on option*/
	DList_of_lists		*file_tree, *file_tree_top_dir;	/* file tree elements used in build_tree() and compare_trees() functions */
	struct thread_struct 	*other_thread_data;
};

void dlist_init(DList *list);
void dlist_destroy(DList *list);
int dlist_ins_next(DList *list, DListElmt *element, char *name, mode_t perm, long size, char *dir_location, int match, char *new_location);
int dlist_remove(DList *list, DListElmt *element, char **name, char **dir_location, char **new_location);
