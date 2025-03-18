#include "types.h"
#include "user.h"
#include "param.h"
#include "pstat.h"

int
main()
{
  struct pstat ps;
  if (getpinfo(&ps) == -1) {
    printf(1, "ps: getpinfo fail\n");
    exit();
  }

  for (int i = 0; i < NPROC; ++i) {
    if (!ps.inuse[i]) break; // getpinfo return inuse proc at the beginning, if encounter a not inuse one, meaning that is the end

    if (i == 0) printf(1, "| pid | tickets | ticks |\n");
    printf(1, "   %d   ", ps.pid[i]); 
    printf(1, "    %d    ", ps.tickets[i]); 
    printf(1, "    %d    \n", ps.ticks[i]); 
  }

  exit();
}
