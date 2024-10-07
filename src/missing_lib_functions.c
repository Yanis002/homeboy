#define HW_DOL

#include "exi.h"
#include "gc/lwp.h"
#include "types.h"
#include "vc.h"

typedef struct _lwpnode {
    struct _lwpnode* next;
    struct _lwpnode* prev;
} lwp_node;

typedef struct _lwpqueue {
    lwp_node* first;
    lwp_node* perm_null;
    lwp_node* last;
} lwp_queue;

typedef u32 lwpq_t;

typedef struct _exibus_priv {
    EXICallback CallbackEXI;
    EXICallback CallbackTC;
    EXICallback CallbackEXT;

    u32 flags;
    u32 imm_len;
    void* imm_buff;
    u32 lockeddev;
    u32 exi_id;
    u64 exi_idtime;
    u32 lck_cnt;
    u32 lckd_dev_bits;
    lwp_queue lckd_dev;
    OSThreadQueue syncqueue;
} exibus_priv;

#define _CPU_ISR_Disable(_isr_cookie)                                   \
    do {                                                                \
        register u32 _disable_mask = 0;                                 \
        __asm__ __volatile__("mfmsr %1\n"                               \
                             "rlwinm %0,%1,0,17,15\n"                   \
                             "mtmsr %0\n"                               \
                             "extrwi %1,%1,1,16"                        \
                             : "=&r"(_disable_mask), "=&r"(_isr_cookie) \
                             :                                          \
                             : "memory");                               \
    } while (0)

#define _CPU_ISR_Restore(_isr_cookie)              \
    do {                                           \
        register u32 _enable_mask = 0;             \
        __asm__ __volatile__("mfmsr %0\n"          \
                             "insrwi %0,%1,1,16\n" \
                             "mtmsr %0\n"          \
                             : "=&r"(_enable_mask) \
                             : "r"(_isr_cookie)    \
                             : "memory");          \
    } while (0)

#define __stringify(rn) #rn
#define SPRG0	272
#define mfspr(_rn) \
({	register u32 _rval = 0; \
	__asm__ __volatile__("mfspr %0," __stringify(_rn) \
	: "=r" (_rval));\
	_rval; \
})

static exibus_priv eximap[EXI_DEVICE_MAX];
static vu32 (*const _exiReg)[5] = (u32(*)[])0xCC006800;

u32 __lwp_isr_in_progress(void) {
    register u32 isr_nest_level;
    isr_nest_level = mfspr(SPRG0);
    return isr_nest_level;
}

static s32 __exi_synccallback(s32 nChn, s32 nDev) {
    exibus_priv* exi = &eximap[nChn];
    OSWakeupThread(&exi->syncqueue);
    return 1;
}

static s32 __exi_syncex(s32 nChn) {
    u32 level;
    exibus_priv* exi = &eximap[nChn];

    _CPU_ISR_Disable(level);
    // if (exi->syncqueue.head == LWP_TQUEUE_NULL) {
    //     if (LWP_InitQueue(&exi->syncqueue) == -1) {
    //         _CPU_ISR_Restore(level);
    //         return 0;
    //     }
    // }
    while (exi->flags & EXI_STATE_BUSY) {
        OSSleepThread(&exi->syncqueue);
    }
    _CPU_ISR_Restore(level);
    return 1;
}

s32 EXI_DmaEx(s32 nChn, void* pData, u32 nLen, u32 nMode) {
    u32 roundlen;
    s32 missalign;
    s32 len = nLen;
    u8* ptr = pData;

    if (!ptr || len <= 0)
        return 0;

    missalign = -((u32)ptr) & 0x1f;
    if ((len - missalign) < 32)
        return EXIImmEx(nChn, ptr, len, nMode);

    if (missalign > 0) {
        if (!EXIImmEx(nChn, ptr, missalign, nMode))
            return 0;
        len -= missalign;
        ptr += missalign;
    }

    roundlen = (len & ~0x1f);
    if (nMode == EXI_READ)
        DCInvalidateRange(ptr, roundlen);
    else
        DCStoreRange(ptr, roundlen);
    if (!__lwp_isr_in_progress()) {
        if (!EXIDma(nChn, ptr, roundlen, nMode, __exi_synccallback))
            return 0;
        if (!__exi_syncex(nChn))
            return 0;
    } else {
        if (!EXIDma(nChn, ptr, roundlen, nMode, NULL))
            return 0;
        if (!EXISync(nChn))
            return 0;
    }

    len -= roundlen;
    ptr += roundlen;
    if (len > 0)
        return EXIImmEx(nChn, ptr, len, nMode);

    return 1;
}

s32 EXI_SelectSD(s32 nChn, s32 nDev, s32 nFrq) {
    u32 val;
    u32 level;
    exibus_priv* exi = &eximap[nChn];

#ifdef _EXI_DEBUG
    OSReport("EXI_SelectSD(%d,%d,%d)\n", nChn, nDev, nFrq);
#endif
    _CPU_ISR_Disable(level);

    if (exi->flags & EXI_STATE_SELECTED) {
#ifdef _EXI_DEBUG
        OSReport("EXI_SelectSD(): already selected.\n");
#endif
        _CPU_ISR_Restore(level);
        return 0;
    }

    if (nChn != EXI_DEVICE_2) {
        if (nDev == EXI_DEVICE_0 && !(exi->flags & EXI_STATE_ATTACHED)) {
            if (EXIProbe(nChn) == 0) {
                _CPU_ISR_Restore(level);
                return 0;
            }
        }
        if (!(exi->flags & EXI_STATE_LOCKED) || exi->lockeddev != nDev) {
#ifdef _EXI_DEBUG
            OSReport("EXI_SelectSD(): not locked or wrong dev(%d).\n", exi->lockeddev);
#endif
            _CPU_ISR_Restore(level);
            return 0;
        }
    }

    exi->flags |= EXI_STATE_SELECTED;
    val = _exiReg[nChn][0];
    val = (val & 0x405) | (nFrq << 4);
    _exiReg[nChn][0] = val;

    if (exi->flags & EXI_STATE_ATTACHED) {
        if (nChn == EXI_DEVICE_0)
            __OSMaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXT);
        else if (nChn == EXI_DEVICE_1)
            __OSMaskInterrupts(OS_INTERRUPTMASK_EXI_1_EXT);
    }

    _CPU_ISR_Restore(level);
    return 1;
}
