#include "types.h"
#include "defs.h"

int
sys_mprotect(void)
{
  char *p;
  int i;

  if (argptr(0, &p, sizeof(void*)) < 0 || argint(1, &i) < 0) return -1;

  return mprotect((void*)p, i);
}

int
sys_munprotect(void)
{
  char *p;
  int i;

  if (argptr(0, &p, sizeof(void*)) < 0 || argint(1, &i) < 0) return -1;

  return munprotect((void*)p, i);
}
