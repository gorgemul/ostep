#include "../type.h"
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

void build_test1_img(char *fs, int fs_sz, short type, int ino, int index)
{
    static char test_img_name[100];
    sprintf(test_img_name, "test1_%d.img", index);
    int test_img = open(test_img_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
    int write_sz = write(test_img, fs, fs_sz);
    assert(write_sz == fs_sz && "write");
    char *test_img_map = mmap(NULL, fs_sz, PROT_READ | PROT_WRITE, MAP_SHARED, test_img, 0);
    close(test_img);
    struct dinode din;
    din.type = xshort(type);
    write_inode(test_img_map, ino, &din);
    munmap(test_img_map, fs_sz);
}

int main(void)
{
    int fd = open("fs.img", O_RDWR);
    assert(fd != -1 && "open");
    int file_sz = fd2sz(fd);
    char *fs = mmap(NULL, file_sz, PROT_READ, MAP_PRIVATE, fd, 0);
    sb = init_superblock(fs);
    build_test1_img(fs, file_sz, 5, 1, 1);
    build_test1_img(fs, file_sz, 8, 100, 2);
    build_test1_img(fs, file_sz, 10, 199, 3);
    munmap(fs, file_sz);
    return 0;
}
