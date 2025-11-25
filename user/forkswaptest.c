#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096
#define NPAGE 30

int
main(int argc, char *argv[])
{
    printf("=== Fork Swap Test ===\n");

    printf("\nParent: Allocating %d pages\n", NPAGE);
    char *pages[NPAGE];

    for(int i = 0; i < NPAGE; i++) {
        pages[i] = sbrk(PGSIZE);
        if(pages[i] == (char*)-1) {
            printf("FAIL: Parent sbrk failed\n");
            exit(1);
        }
        pages[i][0] = i;
        pages[i][PGSIZE-1] = i + 100;
    }
    printf("Parent: Allocated and initialized\n");

    printf("\nForking...\n");
    int pid = fork();

    if(pid < 0) {
        printf("FAIL: fork failed\n");
        exit(1);
    }

    if(pid == 0) {
        printf("Child: Verifying inherited pages\n");
        for(int i = 0; i < NPAGE; i++) {
            if(pages[i][0] != i) {
                printf("FAIL: Child data mismatch at page %d start\n", i);
                exit(1);
            }
            if(pages[i][PGSIZE-1] != i + 100) {
                printf("FAIL: Child data mismatch at page %d end\n", i);
                exit(1);
            }
        }
        printf("Child: PASS - All data verified\n");

        for(int i = 0; i < NPAGE; i++) {
            pages[i][50] = 200 + i;
        }
        printf("Child: Modified data\n");
        exit(0);
    } else {
        wait(0);
        printf("\nParent: Child finished, verifying parent data\n");
        for(int i = 0; i < NPAGE; i++) {
            if(pages[i][0] != i) {
                printf("FAIL: Parent data corrupted at page %d\n", i);
                exit(1);
            }
            if(pages[i][50] == 200 + i) {
                printf("FAIL: Child modification visible in parent\n");
                exit(1);
            }
        }
        printf("Parent: PASS - Data unchanged after fork\n");
    }

    printf("\n=== Fork Test Passed ===\n");
    exit(0);
}
