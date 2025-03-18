#include "types.h"
#include "user.h"

#define CHILD_NUM 3

int
main()
{
  int tickets[] = {1, 2, 3};

  for (int i = 0; i < CHILD_NUM; ++i) {
    int child = fork();

    if (child == -1) {
      printf(1, "pstest: fork fail\n");
      exit();
    }

    if (child == 0) {
      settickets(tickets[i]);
      int j = 0;
      int k = 0;
      while (1) {
        j += k;
      }
      exit(); // Should never return;
    }
  }

  for (int i = 0; i < CHILD_NUM; ++i) {
    wait();
  }

  exit();
}
