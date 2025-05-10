#ifndef _TYPE_H_
#define _TYPE_H_

#include <stdint.h>

#define BSIZE                                512
#define NDIRECT                              12
#define NINDIRECT (BSIZE / sizeof(uint32_t))
#define SUPERBLOCK_START                     1
#define NINODES                              200
#define ROOT_DIR_INO                         1
#define DIRSIZ                               14

struct superblock {
    uint32_t size;         // Size of file system image (blocks)
    uint32_t nblocks;      // Number of data blocks
    uint32_t ninodes;      // Number of inodes.
    uint32_t nlog;         // Number of log blocks
    uint32_t logstart;     // Block number of first log block
    uint32_t inodestart;   // Block number of first inode block
    uint32_t bmapstart;    // Block number of first free map block
};

// On-disk inode structure
struct dinode {
    short type;           // File type
    short major;          // Major device number (T_DEV only)
    short minor;          // Minor device number (T_DEV only)
    short nlink;          // Number of links to inode in file system
    uint32_t size;            // Size of file (bytes)
    uint32_t addrs[NDIRECT+1];   // Data block addresses
};

struct dirent {
    uint16_t inum;
    char name[DIRSIZ];
};

enum FileType { T_FREE = 0, T_DIR = 1, T_FILE = 2, T_DEV = 3 };

#define B2B(block_num, off) (((block_num) * (BSIZE))+(off%BSIZE)) // block to byte
#define IPB (BSIZE / sizeof(struct dinode))
#define IBLOCK(i, sb) ((i) / IPB + sb->inodestart)
#define DPB (BSIZE / sizeof(struct dirent))
// Bitmap bits per block
#define BPB           (BSIZE*8)
// Block of free map containing bit for block b
#define BBLOCK(b, sb) (b/BPB + sb->bmapstart)

#endif // _TYPE_H_
