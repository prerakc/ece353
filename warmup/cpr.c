#include "common.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/dir.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

/* make sure to use syserror() when a system call fails. see common.h */

void recurseDir (char *srcpath, char *dstpath);
void copyFile (char *src, char *dst);

void
usage()
{
	fprintf(stderr, "Usage: cpr srcdir dstdir\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct stat sb;
	int stat_ret = stat(argv[1], &sb);

	if (stat_ret == -1) {
		syserror(stat, argv[1]);
	}

	int mkdir_ret = mkdir(argv[2], (sb.st_mode & S_IRUSR)|(sb.st_mode & S_IWUSR)|(sb.st_mode & S_IXUSR)|
			        (sb.st_mode & S_IRGRP)|(sb.st_mode & S_IWGRP)|(sb.st_mode & S_IXGRP)|
				        (sb.st_mode & S_IROTH)|(sb.st_mode & S_IWOTH)|(sb.st_mode & S_IXOTH));

	if (mkdir_ret == -1) {
		syserror(mkdir, argv[2]);
	}

	/*
	if (S_ISDIR(sb.st_mode)) {
		printf("this directory");
	}
	*/

	/*
	printf("%u\n", sb.st_mode);
	struct stat sb_2;
	stat(argv[2], &sb_2);
	printf("%u\n", sb_2.st_mode);
	*/	
	

	
	//printf("%s\n%s\n\n", argv[1], argv[2]);

	recurseDir(argv[1], argv[2]);	
	return 0;
}

void recurseDir (char *srcpath, char *dstpath) {
	DIR *srcdir = opendir(srcpath);

	if (srcdir == NULL) {
		syserror(opendir, srcpath);
	}

	//printf("%s\n%s\n\n", srcpath, dstpath);


	struct dirent *entity = readdir(srcdir);

	//int i = 0;
	while (entity != NULL) {	
		if (strcmp(entity->d_name, ".") == 0 || strcmp(entity->d_name, "..") == 0) {
			//printf("%d\tskip %s\n", i, entity->d_name);
			//i += 1;
			entity = readdir(srcdir);
			continue;
		}
	
		//i += 2;

		//printf("%s\n%s\n\n", srcpath, dstpath);

		char s_path[256] = {0};
		strcat(s_path, srcpath);
		strcat(s_path, "/");
		strcat(s_path, entity->d_name);

		char d_path[256] = {0};
		strcat(d_path, dstpath);
		strcat(d_path, "/");
		strcat(d_path, entity->d_name);
		
		//printf("%s\n", s_path);

		struct stat sb;
		int stat_ret = stat(s_path, &sb);

		if (stat_ret == -1) {
			syserror(stat, s_path);
		}

		mode_t mode = (sb.st_mode & S_IRUSR)|(sb.st_mode & S_IWUSR)|(sb.st_mode & S_IXUSR)|
				(sb.st_mode & S_IRGRP)|(sb.st_mode & S_IWGRP)|(sb.st_mode & S_IXGRP)|
				        (sb.st_mode & S_IROTH)|(sb.st_mode & S_IWOTH)|(sb.st_mode & S_IXOTH);
		
		if (entity->d_type == DT_DIR) {
			//printf("DIR:\n%s\n%s\n%s\n%s\n\n", srcpath, dstpath, s_path, d_path);
			int mkdir_ret = mkdir(d_path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
			if (mkdir_ret == -1) {
				syserror(mkdir, d_path);
			}
			recurseDir(s_path, d_path);
		} else if (entity->d_type == DT_REG) {
			//printf("FILE:\n%s\n%s\n%s\n%s\n\n", srcpath, dstpath, s_path, d_path);
			copyFile(s_path, d_path);
		}
		chmod(d_path, mode);	
		entity = readdir(srcdir);
	}

	closedir(srcdir);
}

void copyFile (char *src, char *dst) {
	int fd1, fd2, flags;

	flags = O_RDONLY;
	fd1 = open(src, flags);

	if (fd1 == -1) {
		syserror(open, src);
	}

	fd2 = creat(dst, S_IRUSR | S_IWUSR);
	
	if (fd2 == -1) {
		syserror(creat, dst);
	}

	char buffer[4096];
	int read_ret, write_ret;
	read_ret = read(fd1, buffer, 4096);
	
	if (read_ret == -1) {
		syserror(read, src);
	}
	
	//printf("%d\n", read_ret);

	while (read_ret != 0) {
		//printf("%d\n", read_ret);
		write_ret = write(fd2, buffer, read_ret);
		if (write_ret == -1) {
			syserror(write, src);
		}
		read_ret = read(fd1, buffer, 4096);
		if (read_ret == -1) {
			syserror(read, src);
		}
	}

	close(fd1);
	close(fd2);
}
