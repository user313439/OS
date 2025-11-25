#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096
#define NPAGE 1000

int
main(int argc, char *argv[])
{
    printf("=== PA4 Swap Test ===\n");

    printf("\nTest 1: Allocate %d pages\n", NPAGE);
    char *pages[NPAGE];

    for(int i = 0; i < NPAGE; i++) {
        pages[i] = sbrk(PGSIZE);
        if(pages[i] == (char*)-1) {
            printf("FAIL: sbrk failed at page %d\n", i);
            exit(1);
        }
        pages[i][0] = 'A' + (i % 26);
        pages[i][PGSIZE-1] = 'a' + (i % 26);

        if((i+1) % 10 == 0) {
            printf("  Allocated %d pages\n", i+1);
        }
    }
    printf("PASS: Allocated %d pages\n", NPAGE);

    printf("\nTest 2: Access all pages\n");
    for(int i = 0; i < NPAGE; i++) {
        if(pages[i][0] != 'A' + (i % 26)) {
            printf("FAIL: Data corruption at page %d start\n", i);
            exit(1);
        }
        if(pages[i][PGSIZE-1] != 'a' + (i % 26)) {
            printf("FAIL: Data corruption at page %d end\n", i);
            exit(1);
        }
    }
    printf("PASS: All data verified\n");

    printf("\nTest 3: Access in reverse order\n");
    for(int i = NPAGE-1; i >= 0; i--) {
        if(pages[i][0] != 'A' + (i % 26)) {
            printf("FAIL: Data corruption at page %d\n", i);
            exit(1);
        }
    }
    printf("PASS: Reverse access successful\n");

    printf("\nTest 4: Rewrite and verify\n");
    for(int i = 0; i < NPAGE; i++) {
        pages[i][100] = 'X';
    }
    for(int i = 0; i < NPAGE; i++) {
        if(pages[i][100] != 'X') {
            printf("FAIL: New data lost at page %d\n", i);
            exit(1);
        }
    }
    printf("PASS: Data rewrite successful\n");

    printf("\n=== All Tests Passed ===\n");
    exit(0);
}
