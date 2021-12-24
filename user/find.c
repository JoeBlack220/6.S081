#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find_helper(char buf[512], char* cur_name, char* name) 
{
	char *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(buf, 0)) < 0) {
		fprintf(2, "find: cannot open %s\n", buf);
		return;
	}

	if(fstat(fd, &st) < 0) {
		fprintf(2, "find: cannot stat %s\n", buf);
		close(fd);
		return;
	}

	switch(st.type) {
		case T_FILE:
			if(strcmp(cur_name, name) == 0) {
				printf("%s\n", buf);
			}
			break;
		case T_DIR:
			if(strlen(buf) + 1 + DIRSIZ + 1 > 512) {
				printf("find: path too long\n");
				break;
			}
			p = buf+strlen(buf);
			*p++ = '/';
			while(read(fd, &de, sizeof(de)) == sizeof(de)) {
				if(de.inum == 0)
					continue;
				if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
					continue;
				}
				memmove(p, de.name, DIRSIZ);
				p[DIRSIZ] = 0;
				find_helper(buf, de.name, name);
			}
			break;
	}
	close(fd);
	return;

}


void
find(char* path, char *name) 
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(path, 0)) < 0) {
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}

	if(fstat(fd, &st) < 0) {
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}

	switch(st.type) {
		case T_FILE:
			fprintf(2,"find: given path %s is not a directory\n", path);
			break;
		case T_DIR:
			if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
				printf("find: path too long\n");
				break;
			}
			strcpy(buf, path);
			p = buf+strlen(buf); // move p to the end of buf
			*p++ = '/';
			while(read(fd, &de, sizeof(de)) == sizeof(de)) {
				if(de.inum == 0) { 
					continue;
				}
				if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
					continue;
				}
				memmove(p, de.name, DIRSIZ);
				p[DIRSIZ] = 0;
				find_helper(buf, de.name, name);
			}
			break;
	}
	close(fd);
	return;
}

int
main(int argc, char *argv[])
{
	if(argc < 3) {
		fprintf(2, "usage: find [path] [filename]\n");
		exit(1);
	}

	find(argv[1], argv[2]);

	exit(0);
}
