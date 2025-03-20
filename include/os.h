#ifndef _OS_H
#define _OS_H

#include "types.h"

// OSInterrupt.h
typedef u32 OSInterruptMask;

// os.h
#define OS_CACHED_REGION_PREFIX 0x8000
#define OS_BASE_CACHED          (OS_CACHED_REGION_PREFIX << 16)

typedef struct OSCalendarTime {
    int sec; // seconds after the minute [0, 61]
    int min; // minutes after the hour [0, 59]
    int hour; // hours since midnight [0, 23]
    int mday; // day of the month [1, 31]
    int mon; // month since January [0, 11]
    int year; // years in AD [1, ...]
    int wday; // days since Sunday [0, 6]
    int yday; // days since January 1 [0, 365]

    int msec; // milliseconds after the second [0,999]
    int usec; // microseconds after the millisecond [0,999]
} OSCalendarTime;

// OSAddress.h
static inline void* OSPhysicalToCached(u32 ofs) {
    return (void*)(ofs + OS_BASE_CACHED);
}

// OSContext.h
typedef struct OSContext {
    /* 0x0 */ u32 gprs[32];
    /* 0x80 */ u32 cr;
    /* 0x84 */ u32 lr;
    /* 0x88 */ u32 ctr;
    /* 0x8C */ u32 xer;
    /* 0x90 */ f64 fprs[32];
    /* 0x190 */ u32 fpscr_pad;
    /* 0x194 */ u32 fpscr;
    /* 0x198 */ u32 srr0;
    /* 0x19C */ u32 srr1;
    /* 0x1A0 */ u16 mode;
    /* 0x1A2 */ u16 state;
    /* 0x1A4 */ u32 gqrs[8];
    /* 0x1C4 */ u32 psf_pad;
    /* 0x1C8 */ f64 psfs[32];
} OSContext;

// OSThread.h
typedef struct OSThreadQueue {
    /* 0x0 */ struct OSThread* head;
    /* 0x4 */ struct OSThread* tail;
} OSThreadQueue;

typedef struct OSMutexQueue {
    /* 0x0 */ struct OSMutex* head;
    /* 0x4 */ struct OSMutex* tail;
} OSMutexQueue;

typedef struct OSThread {
    /* 0x000 */ OSContext context;
    /* 0x2C8 */ u16 state;
    /* 0x2CA */ u16 flags;
    /* 0x2CC */ s32 suspend;
    /* 0x2D0 */ s32 priority;
    /* 0x2D4 */ s32 base;
    /* 0x2D8 */ u32 val;
    /* 0x2DC */ OSThreadQueue* queue;
    /* 0x2E0 */ struct OSThread* next;
    /* 0x2E4 */ struct OSThread* prev;
    /* 0x2E8 */ OSThreadQueue joinQueue;
    /* 0x2F0 */ struct OSMutex* mutex;
    /* 0x2F4 */ OSMutexQueue mutexQueue;
    /* 0x2FC */ struct OSThread* nextActive;
    /* 0x300 */ struct OSThread* prevActive;
    /* 0x304 */ u32* stackBegin;
    /* 0x308 */ u32* stackEnd;
    /* 0x30C */ s32 error;
    /* 0x310 */ void* specific[2];
} OSThread;

typedef void* (*OSThreadFunc)(void* arg);
typedef void (*OSExceptionHandler)(u8 type, OSContext* ctx);

typedef s64 OSTime;
typedef u32 OSTick;

typedef struct OSAlarm OSAlarm;

typedef void (*OSAlarmHandler)(OSAlarm* alarm, OSContext* context);

struct OSAlarm {
    OSAlarmHandler handler;
    u32 tag;
    OSTime fire;
    OSAlarm* prev;
    OSAlarm* next;
    OSTime period;
    OSTime start;
};

static inline u64 gettick() {
    register u32 tbu;
    register u32 tbl;

    // clang-format off
    __asm__ __volatile__(
        "mftbl %0\n"
        "mftbu %1\n"
        ::
        "r"(tbl), "r"(tbu));
    // clang-format on

    return (u64)((u64)tbu << 32 | tbl);
}

#define __OS_INTERRUPT_MEM_0         0
#define __OS_INTERRUPT_MEM_1         1
#define __OS_INTERRUPT_MEM_2         2
#define __OS_INTERRUPT_MEM_3         3
#define __OS_INTERRUPT_MEM_ADDRESS   4
#define __OS_INTERRUPT_DSP_AI        5
#define __OS_INTERRUPT_DSP_ARAM      6
#define __OS_INTERRUPT_DSP_DSP       7
#define __OS_INTERRUPT_AI_AI         8
#define __OS_INTERRUPT_EXI_0_EXI     9
#define __OS_INTERRUPT_EXI_0_TC      10
#define __OS_INTERRUPT_EXI_0_EXT     11
#define __OS_INTERRUPT_EXI_1_EXI     12
#define __OS_INTERRUPT_EXI_1_TC      13
#define __OS_INTERRUPT_EXI_1_EXT     14
#define __OS_INTERRUPT_EXI_2_EXI     15
#define __OS_INTERRUPT_EXI_2_TC      16
#define __OS_INTERRUPT_PI_CP         17
#define __OS_INTERRUPT_PI_PE_TOKEN   18
#define __OS_INTERRUPT_PI_PE_FINISH  19
#define __OS_INTERRUPT_PI_SI         20
#define __OS_INTERRUPT_PI_DI         21
#define __OS_INTERRUPT_PI_RSW        22
#define __OS_INTERRUPT_PI_ERROR      23
#define __OS_INTERRUPT_PI_VI         24
#define __OS_INTERRUPT_PI_DEBUG      25
#define __OS_INTERRUPT_PI_HSP        26
#define __OS_INTERRUPT_MAX           32

#define OS_INTERRUPTMASK(interrupt)  (0x80000000u >> (interrupt))

#define OS_INTERRUPTMASK_MEM_0       OS_INTERRUPTMASK(__OS_INTERRUPT_MEM_0)
#define OS_INTERRUPTMASK_MEM_1       OS_INTERRUPTMASK(__OS_INTERRUPT_MEM_1)
#define OS_INTERRUPTMASK_MEM_2       OS_INTERRUPTMASK(__OS_INTERRUPT_MEM_2)
#define OS_INTERRUPTMASK_MEM_3       OS_INTERRUPTMASK(__OS_INTERRUPT_MEM_3)
#define OS_INTERRUPTMASK_MEM_ADDRESS OS_INTERRUPTMASK(__OS_INTERRUPT_MEM_ADDRESS)
#define OS_INTERRUPTMASK_MEM                                                                             \
    (OS_INTERRUPTMASK_MEM_0 | OS_INTERRUPTMASK_MEM_1 | OS_INTERRUPTMASK_MEM_2 | OS_INTERRUPTMASK_MEM_3 | \
     OS_INTERRUPTMASK_MEM_ADDRESS)
#define OS_INTERRUPTMASK_DSP_AI    OS_INTERRUPTMASK(__OS_INTERRUPT_DSP_AI)
#define OS_INTERRUPTMASK_DSP_ARAM  OS_INTERRUPTMASK(__OS_INTERRUPT_DSP_ARAM)
#define OS_INTERRUPTMASK_DSP_DSP   OS_INTERRUPTMASK(__OS_INTERRUPT_DSP_DSP)
#define OS_INTERRUPTMASK_DSP       (OS_INTERRUPTMASK_DSP_AI | OS_INTERRUPTMASK_DSP_ARAM | OS_INTERRUPTMASK_DSP_DSP)
#define OS_INTERRUPTMASK_AI_AI     OS_INTERRUPTMASK(__OS_INTERRUPT_AI_AI)
#define OS_INTERRUPTMASK_AI        (OS_INTERRUPTMASK_AI_AI)
#define OS_INTERRUPTMASK_EXI_0_EXI OS_INTERRUPTMASK(__OS_INTERRUPT_EXI_0_EXI)
#define OS_INTERRUPTMASK_EXI_0_TC  OS_INTERRUPTMASK(__OS_INTERRUPT_EXI_0_TC)
#define OS_INTERRUPTMASK_EXI_0_EXT OS_INTERRUPTMASK(__OS_INTERRUPT_EXI_0_EXT)
#define OS_INTERRUPTMASK_EXI_0     (OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_0_TC | OS_INTERRUPTMASK_EXI_0_EXT)
#define OS_INTERRUPTMASK_EXI_1_EXI OS_INTERRUPTMASK(__OS_INTERRUPT_EXI_1_EXI)
#define OS_INTERRUPTMASK_EXI_1_TC  OS_INTERRUPTMASK(__OS_INTERRUPT_EXI_1_TC)
#define OS_INTERRUPTMASK_EXI_1_EXT OS_INTERRUPTMASK(__OS_INTERRUPT_EXI_1_EXT)
#define OS_INTERRUPTMASK_EXI_1     (OS_INTERRUPTMASK_EXI_1_EXI | OS_INTERRUPTMASK_EXI_1_TC | OS_INTERRUPTMASK_EXI_1_EXT)
#define OS_INTERRUPTMASK_EXI_2_EXI OS_INTERRUPTMASK(__OS_INTERRUPT_EXI_2_EXI)
#define OS_INTERRUPTMASK_EXI_2_TC  OS_INTERRUPTMASK(__OS_INTERRUPT_EXI_2_TC)
#define OS_INTERRUPTMASK_EXI_2     (OS_INTERRUPTMASK_EXI_2_EXI | OS_INTERRUPTMASK_EXI_2_TC)
#define OS_INTERRUPTMASK_EXI                                                               \
    (OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_0_TC | OS_INTERRUPTMASK_EXI_0_EXT | \
     OS_INTERRUPTMASK_EXI_1_EXI | OS_INTERRUPTMASK_EXI_1_TC | OS_INTERRUPTMASK_EXI_1_EXT | \
     OS_INTERRUPTMASK_EXI_2_EXI | OS_INTERRUPTMASK_EXI_2_TC)
#define OS_INTERRUPTMASK_PI_PE_TOKEN  OS_INTERRUPTMASK(__OS_INTERRUPT_PI_PE_TOKEN)
#define OS_INTERRUPTMASK_PI_PE_FINISH OS_INTERRUPTMASK(__OS_INTERRUPT_PI_PE_FINISH)
#define OS_INTERRUPTMASK_PI_PE        (OS_INTERRUPTMASK_PI_PE_TOKEN | OS_INTERRUPTMASK_PI_PE_FINISH)
#define OS_INTERRUPTMASK_PI_CP        OS_INTERRUPTMASK(__OS_INTERRUPT_PI_CP)
#define OS_INTERRUPTMASK_PI_SI        OS_INTERRUPTMASK(__OS_INTERRUPT_PI_SI)
#define OS_INTERRUPTMASK_PI_DI        OS_INTERRUPTMASK(__OS_INTERRUPT_PI_DI)
#define OS_INTERRUPTMASK_PI_RSW       OS_INTERRUPTMASK(__OS_INTERRUPT_PI_RSW)
#define OS_INTERRUPTMASK_PI_ERROR     OS_INTERRUPTMASK(__OS_INTERRUPT_PI_ERROR)
#define OS_INTERRUPTMASK_PI_VI        OS_INTERRUPTMASK(__OS_INTERRUPT_PI_VI)
#define OS_INTERRUPTMASK_PI_DEBUG     OS_INTERRUPTMASK(__OS_INTERRUPT_PI_DEBUG)
#define OS_INTERRUPTMASK_PI_HSP       OS_INTERRUPTMASK(__OS_INTERRUPT_PI_HSP)
#define OS_INTERRUPTMASK_PI                                                                               \
    (OS_INTERRUPTMASK_PI_CP | OS_INTERRUPTMASK_PI_SI | OS_INTERRUPTMASK_PI_DI | OS_INTERRUPTMASK_PI_RSW | \
     OS_INTERRUPTMASK_PI_ERROR | OS_INTERRUPTMASK_PI_VI | OS_INTERRUPTMASK_PI_PE_TOKEN |                  \
     OS_INTERRUPTMASK_PI_PE_FINISH | OS_INTERRUPTMASK_PI_DEBUG | OS_INTERRUPTMASK_PI_HSP)

#endif
