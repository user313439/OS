#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid = getpid();
  
  printf(">>> Testing getnice and setnice:\n");
  printf("initial nice value: %d\n", getnice(pid));
  setnice(pid, 10);
  printf("nice value after setting: %d\n", getnice(pid));
  
  printf("\n>>> Testing ps:\n");
  ps(0);
  
  printf("\n>>> Testing meminfo:\n");
  meminfo();
  
  printf("\n>>> Testing waitpid:\n");
  int child_pid = fork();
  if(child_pid == 0) {
    sleep(10);
    exit(0);
  }
  printf("waiting for child %d...\n", child_pid);
  waitpid(child_pid);
  printf("child %d terminated\n", child_pid);
  
  exit(0);
}
