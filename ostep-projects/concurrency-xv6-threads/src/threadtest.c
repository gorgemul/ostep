#include "types.h"
#include "stat.h"
#include "user.h"

#define LOOP 50

// since its multi-thread, arg1 and arg2 order can't be determined
void
f(void *arg1, void *arg2)
{
    printf(1, "arg: %d-%d\n", *((int*)arg1), *((int*)arg2));
    exit();
}

int
main(void)
{
    int a = 1;
    int b = 2;

    for (int i = 0; i < LOOP; i++)
        i % 2 == 0 ? thread_create(f, (void*)&a, (void*)&b) : thread_create(f, (void*)&b, (void*)&a);

    for (int i = 0; i < LOOP; i++)
        thread_join();

    printf(1, "all thread join\n");

    exit();
}
