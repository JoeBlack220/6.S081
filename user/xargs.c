#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#define MAXLEN 100
int
main(int argc, char *argv[]) 
{
	if(argc <= 1) {
		fprintf(2, "usage: xargs command (arg...)\n");
		exit(1);
	}
	char *command = argv[1];
	char buf;
	char new_argv[MAXARG][MAXLEN]; // assuming the maximun single parameter length is 512
	char *p_new_argv[MAXARG];

	while(1) {
		memset(new_argv, 0, MAXARG * MAXLEN); // reset the parameter

		for(int i = 1; i < argc; ++i) {
			strcpy(new_argv[i-1], argv[i]);
		}

		int cur_argc = argc - 1;
		int offset = 0;
		int is_read = 0;

		while((is_read = read(0, &buf, 1)) > 0) {
			if(buf == ' ') {
				cur_argc++;
				offset = 0;	
				continue;
			}
			if(buf == '\n') {
				break;
			}
			if(offset==MAXLEN) {
				fprintf(2, "xargs: parameter too long\n");
				exit(1);
			}
			if(cur_argc == MAXARG) {
				fprintf(2, "xargs: too many arguments\n");
				exit(1);
			}
			new_argv[cur_argc][offset++] = buf;
		}
		
		if(is_read <= 0) {
			break;
		} 
		for(int i = 0; i <= cur_argc; ++i) {
			p_new_argv[i] = new_argv[i];
		}
		if(fork() == 0) {
			exec(command, p_new_argv);
			exit(1);
		} 
		wait((int*) 0);
	}
	exit(0);
}
