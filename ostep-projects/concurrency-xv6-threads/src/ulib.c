#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

struct lock_t {
    int ticket;
    int turn;
};

struct lock_t lock;
int lock_have_init = 0;

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

int
thread_create(void (*fcn)(void *arg1, void *arg2), void *arg1, void *arg2)
{
    if (!lock_have_init) {
        lock_init(&lock);
        lock_have_init = 1;
    }
    lock_acquire(&lock);
    void *stack = malloc(4096);
    lock_release(&lock);
    if (!stack) {
        printf(1, "thread_create: malloc error\n");
        exit();
    }
    return clone(fcn, arg1, arg2, stack);
}

int
thread_join(void)
{
    void *stack;
    if (!lock_have_init) {
        lock_init(&lock);
        lock_have_init = 1;
    }
    lock_acquire(&lock);
    int pid = join(&stack);
    lock_release(&lock);
    return pid;
}

void
lock_init(struct lock_t *lock)
{
    lock->ticket = 0;
    lock->turn = 0;
}

void
lock_acquire(struct lock_t *lock)
{
    int my_turn = fetch_and_add1(&lock->ticket);
    while (lock->turn != my_turn);
}

void
lock_release(struct lock_t *lock)
{
    lock->turn++;
}
