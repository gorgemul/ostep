#include "types.h"
#include "user.h"

#define PGSIZE 4096

int
main()
{
  int *addr = (int*)0x1000;
  int child;

  printf(1, "read addr 0x1000: %d\n", *addr);
  mprotect((void*)addr, PGSIZE);

  child = fork();

  if (child == -1) {
    printf(1, "mprotectforktest: fork fail\n");
    exit();
  }
  
  if (child == 0) {
    printf(1, "unprotect the memory\n");
    munprotect((void*)addr, PGSIZE);

    printf(1, "wrting to addr 0x1000 from child\n");
    *addr = 10000;
    printf(1, "read addr from child: %d\n", *addr);
    exit();    
  } else {
    wait();
    printf(1, "wrting to addr 0x1000 from parent\n"); // Shoud fail
    *addr = 10000;
    printf(1, "read addr from parent: %d\n", *addr);
  }

  exit();
}
