#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
cpu_bound_process(int nice_val)
{
  int pid = getpid();
  setnice(pid, nice_val);
  
  for(int i = 0; i < 1000000000; i++) {
    if(i % 100000000 == 0) {
      printf("PID %d (nice %d) running...\n", pid, nice_val);
    }
  }
  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("=== EEVDF Scheduler Test ===\n");
  
  int pid1 = fork();
  if(pid1 == 0) {
    cpu_bound_process(0);
  }
  
  int pid2 = fork();
  if(pid2 == 0) {
    cpu_bound_process(10);
  }
  
  int pid3 = fork();
  if(pid3 == 0) {
    cpu_bound_process(20);
  }
  
  sleep(50);
  
  printf("\n=== Process Status ===\n");
  ps(0);
  
  wait(0);
  wait(0);
  wait(0);
  
  exit(0);
}