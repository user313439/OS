#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int i;

  fprintf(1, "=== Starting All System Call Tests ===\n\n");

  // 1. Test ps() and nice values for existing processes
  fprintf(1, "--- Initial Process Status (ps(0)) ---\n");
  ps(0);
  fprintf(1, "-------------------------------------\n\n");

  // 2. Test getnice() and setnice() with loops and boundary checks
  fprintf(1, "--- Testing getnice() and setnice() ---\n");
  // Test a few initial processes
  for (i = 1; i < 5; i++) {
    int initial_nice = getnice(i);
    if(initial_nice != -1) { // Only test if PID is valid
      fprintf(1, "PID %d: Initial nice = %d\n", i, initial_nice);
      fprintf(1, "PID %d: Setting nice to 10... (ret: %d)\n", i, setnice(i, 10));
      fprintf(1, "PID %d: New nice = %d\n", i, getnice(i));
      setnice(i, initial_nice); // Revert to original nice value
    }
  }
  // Test invalid PID
  fprintf(1, "PID 999 (invalid): getnice() ret = %d\n", getnice(999));
  fprintf(1, "PID 999 (invalid): setnice() ret = %d\n", setnice(999, 10));
  
  // Boundary checks on PID 2 (e.g., 'sh')
  fprintf(1, "\n--- Boundary value tests for setnice() on PID 2 ---\n");
  int original_nice_sh = getnice(2);
  fprintf(1, "PID 2 original nice: %d\n", original_nice_sh);
  fprintf(1, "Setting nice to -1 (invalid): ret %d\n", setnice(2, -1));
  fprintf(1, "Setting nice to 40 (invalid): ret %d\n", setnice(2, 40));
  fprintf(1, "PID 2 nice should be unchanged: %d\n", getnice(2));
  fprintf(1, "Setting nice to 5 (valid): ret %d\n", setnice(2, 5));
  fprintf(1, "PID 2 new nice: %d\n", getnice(2));
  setnice(2, original_nice_sh); // Revert
  fprintf(1, "PID 2 reverted to original nice: %d\n", getnice(2));
  fprintf(1, "-------------------------------------------\n\n");

  // 3. Test ps() with a loop
  fprintf(1, "--- Testing ps() for individual PIDs ---\n");
  for (i = 1; i < 5; i++) {
    fprintf(1, "\n-- ps(%d) --\n", i);
    ps(i);
  }
  fprintf(1, "---------------------------------------\n\n");

  // 4. Test meminfo()
  fprintf(1, "--- Testing meminfo() ---\n");
  meminfo();
  fprintf(1, "--------------------------\n\n");

  // 5. Test waitpid() using fork()
  fprintf(1, "--- Testing waitpid() ---\n");
  int pid = fork();
  if (pid == 0) {
    // Child process
    fprintf(1, "Child (PID %d) created. Wasting time...\n", getpid());
    for (volatile int j = 0; j < 100000000; j++); // Busy wait
    fprintf(1, "Child exiting.\n");
    exit(0);
  } else if (pid > 0) {
    // Parent process
    fprintf(1, "Parent waiting for child PID %d...\n", pid);
    int ret = waitpid(pid);
    fprintf(1, "waitpid(%d) returned: %d. Child terminated.\n", pid, ret);
    
    // Also test waiting for a non-existent/non-child PID
    fprintf(1, "Parent waiting for non-child PID 999...\n");
    ret = waitpid(999);
    fprintf(1, "waitpid(999) returned: %d.\n", ret);
  } else {
    fprintf(1, "fork() failed!\n");
  }
  fprintf(1, "------------------------\n\n");

  fprintf(1, "=== System Call Tests Complete ===\n");

  exit(0);
}
