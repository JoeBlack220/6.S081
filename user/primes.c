#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{

  int p[2];
  pipe(p);
  int buf[1];

  for(int i = 2; i <= 35; ++i) {
    write(p[1], &i, 1);
  }
  close(p[1]);

  int read_fd = p[0];
  while(read(read_fd, buf, 1) > 0) {
    int cur_prime = buf[0];
    printf("prime %d\n", cur_prime);
    pipe(p);          // create new file descriptor for the next child
    int fork_pid = fork();
    if(fork_pid != 0) {
      close(p[0]);
      while(read(read_fd, buf, 1) > 0) {
        if(buf[0] % cur_prime != 0) {
          write(p[1], &buf[0], 1);
        } 
      }
      close(read_fd);
      close(p[1]);
      wait((int *)0);
    } else {
        close(p[1]);
        read_fd = p[0];
    }
  }
  exit(0);
} 
