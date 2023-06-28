#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <libgen.h>
#include <limits.h>

#include <sys/times.h>
#include <time.h>

#define _BSD_SOURCE
#include <sys/sysmacros.h>
