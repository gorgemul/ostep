#include "type.h"
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

struct superblock *sb;

struct superblock *init_superblock(char *fs)
{
    struct superblock *sb = malloc(sizeof(struct superblock));
    assert(sb && "malloc");
    memcpy(sb, &fs[B2B(SUPERBLOCK_START, 0)], sizeof(struct superblock));
    return sb;
}

int fd2sz(int fd)
{
    struct stat s;
    int rc = fstat(fd, &s);
    assert(rc != -1 && "fstat");
    return (int)s.st_size;
}

uint16_t xshort(uint16_t x)
{
  uint16_t y;
  uint8_t *a = (uint8_t*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint32_t xint(uint32_t x)
{
  uint32_t y;
  uint8_t *a = (uint8_t*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

void write_inode(char *fs, int ino, struct dinode *inode)
{
    memcpy(&fs[B2B(IBLOCK(ino, sb), (ino % IPB) * sizeof(struct dinode))], inode, sizeof(struct dinode));
}

int main()
{
    int write_sz = 0;
    struct dinode din = {0};
    int fd = open("fs.img", O_RDWR);
    assert(fd != -1 && "open");
    int file_sz = fd2sz(fd);
    char *fs = mmap(NULL, file_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    sb = init_superblock(fs);
    assert(fs != MAP_FAILED && "mmap");
    int test1_1_img = open("test1_1.img", O_RDWR | O_CREAT | O_TRUNC, 0644);
    int test1_2_img = open("test1_2.img", O_RDWR | O_CREAT | O_TRUNC, 0644);
    int test1_3_img = open("test1_3.img", O_RDWR | O_CREAT | O_TRUNC, 0644);
    assert(test1_1_img != -1 && "open");
    assert(test1_2_img != -1 && "open");
    assert(test1_3_img != -1 && "open");
    din.type = xshort(5);
    write_inode(fs, 1, &din);
    write_sz = write(test1_1_img, fs, file_sz);
    assert(write_sz == file_sz && "write");
    write_sz = write(test1_2_img, fs, file_sz);
    write_inode(fs, 100, &din);
    assert(write_sz == file_sz && "write");
    write_inode(fs, 199, &din);
    write_sz = write(test1_3_img, fs, file_sz);
    assert(write_sz == file_sz && "write");
    return 0;
}
