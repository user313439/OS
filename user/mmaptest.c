#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "user.h"
#include "../kernel/fcntl.h"
#include "../kernel/memlayout.h"
#include "../kernel/param.h"
#include "../kernel/spinlock.h"
#include "../kernel/sleeplock.h"
#include "../kernel/fs.h"
#include "../kernel/syscall.h"

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_ANONYMOUS 0x1
#define MAP_POPULATE 0x2
#define MMAPBASE 0x40000000

void
test_anonymous_populate(void)
{
  printf("Test 1: Anonymous mapping with MAP_POPULATE\n");
  int before = freemem();

  char *addr = mmap(0, 8192, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_POPULATE, -1, 0);
  if(addr == 0) {
    printf("FAIL: mmap returned 0\n");
    return;
  }

  int after_mmap = freemem();
  printf("Freemem before: %d, after mmap: %d, diff: %d\n", before, after_mmap, before - after_mmap);

  addr[0] = 'A';
  addr[4095] = 'B';
  addr[4096] = 'C';
  addr[8191] = 'D';

  if(addr[0] != 'A' || addr[4095] != 'B' || addr[4096] != 'C' || addr[8191] != 'D') {
    printf("FAIL: Data mismatch\n");
    return;
  }

  if(munmap(addr) < 0) {
    printf("FAIL: munmap failed\n");
    return;
  }

  int after_munmap = freemem();
  printf("Freemem after munmap: %d\n", after_munmap);
  printf("PASS\n\n");
}

void
test_anonymous_no_populate(void)
{
  printf("Test 2: Anonymous mapping without MAP_POPULATE\n");
  int before = freemem();

  char *addr = mmap(0, 8192, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, -1, 0);
  if(addr == 0) {
    printf("FAIL: mmap returned 0\n");
    return;
  }

  int after_mmap = freemem();
  printf("Freemem before: %d, after mmap: %d, diff: %d\n", before, after_mmap, before - after_mmap);

  addr[0] = 'A';

  int after_first_access = freemem();
  printf("Freemem after first page access: %d\n", after_first_access);

  addr[4096] = 'B';

  int after_second_access = freemem();
  printf("Freemem after second page access: %d\n", after_second_access);

  if(munmap(addr) < 0) {
    printf("FAIL: munmap failed\n");
    return;
  }

  int after_munmap = freemem();
  printf("Freemem after munmap: %d\n", after_munmap);
  printf("PASS\n\n");
}

void
test_file_populate(void)
{
  printf("Test 3: File mapping with MAP_POPULATE\n");

  int fd = open("README", O_RDONLY);
  if(fd < 0) {
    printf("FAIL: Cannot open README\n");
    return;
  }

  int before = freemem();

  char *addr = mmap(0, 8192, PROT_READ, MAP_POPULATE, fd, 0);
  if(addr == 0) {
    printf("FAIL: mmap returned 0\n");
    close(fd);
    return;
  }

  int after_mmap = freemem();
  printf("Freemem before: %d, after mmap: %d, diff: %d\n", before, after_mmap, before - after_mmap);

  printf("First 3 chars: %c%c%c\n", addr[0], addr[1], addr[2]);

  if(munmap(addr) < 0) {
    printf("FAIL: munmap failed\n");
    close(fd);
    return;
  }

  close(fd);

  int after_munmap = freemem();
  printf("Freemem after munmap: %d\n", after_munmap);
  printf("PASS\n\n");
}

void
test_file_no_populate(void)
{
  printf("Test 4: File mapping without MAP_POPULATE\n");

  int fd = open("README", O_RDONLY);
  if(fd < 0) {
    printf("FAIL: Cannot open README\n");
    return;
  }

  int before = freemem();

  char *addr = mmap(0, 8192, PROT_READ, 0, fd, 0);
  if(addr == 0) {
    printf("FAIL: mmap returned 0\n");
    close(fd);
    return;
  }

  int after_mmap = freemem();
  printf("Freemem before: %d, after mmap: %d, diff: %d\n", before, after_mmap, before - after_mmap);

  char ch = addr[0];

  int after_access = freemem();
  printf("Freemem after page fault: %d\n", after_access);
  printf("First char: %c\n", ch);

  if(munmap(addr) < 0) {
    printf("FAIL: munmap failed\n");
    close(fd);
    return;
  }

  close(fd);

  int after_munmap = freemem();
  printf("Freemem after munmap: %d\n", after_munmap);
  printf("PASS\n\n");
}

void
test_fork(void)
{
  printf("Test 5: Fork with mmap\n");

  int fd = open("README", O_RDONLY);
  if(fd < 0) {
    printf("FAIL: Cannot open README\n");
    return;
  }

  char *addr = mmap(0, 4096, PROT_READ, MAP_POPULATE, fd, 0);
  if(addr == 0) {
    printf("FAIL: mmap returned 0\n");
    close(fd);
    return;
  }

  char parent_ch = addr[0];

  int pid = fork();
  if(pid < 0) {
    printf("FAIL: fork failed\n");
    munmap(addr);
    close(fd);
    return;
  }

  if(pid == 0) {
    char child_ch = addr[0];
    if(child_ch != parent_ch) {
      printf("FAIL: Child sees different data\n");
      exit(1);
    }
    printf("Child sees: %c\n", child_ch);
    munmap(addr);
    close(fd);
    exit(0);
  } else {
    wait(0);
    printf("Parent sees: %c\n", parent_ch);
    munmap(addr);
    close(fd);
  }

  printf("PASS\n\n");
}

int
main(void)
{
  printf("=== mmap Test Suite ===\n\n");

  test_anonymous_populate();
  test_anonymous_no_populate();
  test_file_populate();
  test_file_no_populate();
  test_fork();

  printf("=== All Tests Complete ===\n");
  exit(0);
}
