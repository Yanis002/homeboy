#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "fs.h"
#include "io.h"
#include "types.h"
#include "vc.h"

#if IS_WII

typedef struct {
    /* 0x0000 */ char filepath[64];
    /* 0x0040 */ union {
        /* 0x0080 */ struct {
            /* 0x0000 */ char filepath_old[64];
            /* 0x0040 */ char filepath_new[64];
        } fsrename;
        /* 0x004C */ struct {
            /* 0x0000 */ uint32_t owner_id;
            /* 0x0004 */ uint16_t group_id;
            /* 0x0006 */ char filepath[64];
            /* 0x0046 */ uint8_t ownerperm;
            /* 0x0047 */ uint8_t groupperm;
            /* 0x0048 */ uint8_t otherperm;
            /* 0x0049 */ uint8_t attributes;
            /* 0x004A */ uint8_t pad0[2];
        } fsattr;
        /* 0x0024 */ struct {
            /* 0x0000 */ IPCIOVector vector[4];
            /* 0x0020 */ uint32_t entry_cnt;
        } fsreaddir;
        /* 0x0044 */ struct {
            /* 0x0000 */ IPCIOVector vector[4];
            /* 0x0020 */ uint32_t usage1;
            /* 0x0024 */ uint8_t pad0[28];
            /* 0x0040 */ uint32_t usage2;
        } fsusage;
        /* 0x001C */ struct {
            /* 0x0000 */ uint32_t a;
            /* 0x0004 */ uint32_t b;
            /* 0x0008 */ uint32_t c;
            /* 0x000C */ uint32_t d;
            /* 0x0010 */ uint32_t e;
            /* 0x0014 */ uint32_t f;
            /* 0x0018 */ uint32_t g;
        } fsstats;
    };
    /* 0x00C0 */ void* callback;
    /* 0x00C4 */ void* callback_data;
    /* 0x00C8 */ uint32_t functype;
    /* 0x00CC */ void* funcargv[8];
} isfs_t; // size = 0xEC

const char* fspath = "/dev/fs";
static int fs_fd = -1;
static bool fs_initialized = false;
static isfs_t* fs_buf = NULL;

bool fs_init() {
    if (fs_initialized) {
        return true;
    }

    fs_buf = iosAllocAligned(hb_hid, sizeof(*fs_buf), 32);
    if (fs_buf == NULL) {
        return false;
    }

    if (fs_fd < 0) {
        fs_fd = IOS_Open(fspath, 0);
    }

    if (fs_fd < 0) {
        return false;
    }

    fs_initialized = true;
    return true;
}

int fs_read(int fd, void* buf, size_t len) {
    char* rbuf = iosAllocAligned(hb_hid, 0x2000, 32);
    int ret = 0;
    char* p = buf;

    while (len > 0) {
        int to_read = len > 0x1000 ? 0x2000 : len;
        int read_cnt = IOS_Read(fd, rbuf, to_read);
        memcpy(p, rbuf, read_cnt);
        len -= read_cnt;
        ret += read_cnt;
        p += read_cnt;
    }

    iosFree(hb_hid, rbuf);
    return ret;
}

int fs_write(int fd, void* buf, size_t len) {
    char* wbuf = iosAllocAligned(hb_hid, 0x2000, 32);
    int ret = 0;
    const char* p = buf;

    while (len > 0) {
        int to_write = len > 0x1000 ? 0x2000 : len;
        memcpy(wbuf, p, to_write);
        int written = IOS_Write(fd, wbuf, to_write);
        len -= written;
        ret += written;
        p += written;
    }

    iosFree(hb_hid, wbuf);
    return ret;
}

int fs_close(int fd) {
    return IOS_Close(fd);
}

int fs_create(char* path, int mode) {
    int fd = -1;
    fs_buf->fsattr.attributes = 1;
    fs_buf->fsattr.ownerperm = 3;
    fs_buf->fsattr.groupperm = 3;
    fs_buf->fsattr.otherperm = 3;
    memcpy(fs_buf->fsattr.filepath, path, strlen(path) + 1);
    fd = IOS_Ioctl(fs_fd, 9, &fs_buf->fsattr, sizeof(fs_buf->fsattr), NULL, 0);
    if (fd == 0) {
        fd = IOS_Open(path, mode);
    }

    return fd;
}

int fs_open(char* path, int mode) {
    memcpy(fs_buf->filepath, path, strlen(path) + 1);
    return IOS_Open(fs_buf->filepath, mode);
}

int fs_delete(char* path) {
    memcpy(fs_buf->filepath, path, strlen(path) + 1);
    return IOS_Ioctl(fs_fd, 7, fs_buf->filepath, 64, NULL, 0);
}

#endif
