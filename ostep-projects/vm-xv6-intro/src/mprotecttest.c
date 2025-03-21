#include "types.h"
#include "user.h"

#define PGSIZE 4096

int
main()
{
  int *addr =  (int *)0x1000;

  printf(1, "reading the text: %d\n", *addr);

  if (mprotect((void*)addr, PGSIZE) == -1) {
    printf(1, "mprotecttest: mprotect fail\n");
    exit();
  }

  printf(1, "wrting 10000 to the text\n");
  *addr = 10000;

  printf(1, "reading the text: %d\n", *addr);

  exit();
}
