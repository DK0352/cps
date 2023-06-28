struct errors_data {
	short	file_read_error_count;
	short 	file_write_error_count;
	short	dir_read_error_count;
	short	dir_write_error_count;
	DList *file_read_error, *file_write_error, *dir_read_error, *dir_write_error;
};
