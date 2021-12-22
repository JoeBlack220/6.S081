#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  // Ingnore any input arguments
  
  int parent[2];
  int child[2];

  // Create a pipe in p
  pipe(parent);
  pipe(child);
  
  char buf[1];
  // child process, write one byte to child[1]
  if(fork() == 0) {
		close(parent[1]);
		close(child[0]);
    read(parent[0], buf, 1);
		close(parent[0]);
    if(buf[0] == 'p') {
      printf("%d: received ping\n", getpid());
    }
    write(child[1], "c", 1);
		close(child[1]);
  } else { // parent process, write one byte to parent[1]
		close(parent[0]);
		close(child[1]);
    write(parent[1], "p", 1);
		close(parent[1]);
    read(child[0], buf, 1);
		close(child[0]);
    if(buf[0] == 'c') {
      printf("%d: received pong\n", getpid());
    } 
  }

  exit(0);

}
