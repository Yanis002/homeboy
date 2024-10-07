#include "types.h"
#include "sd.h"
#include "version.h"
#include "gc/card_io.h"
#include "exi.h"

#if IS_GC

s32 drv_no = EXI_CHANNEL_1;
bool is_sdgecko = false;

bool sdio_is_initialized(void) {
    if (is_sdgecko) {
        return sdgecko_isInitialized(drv_no);
    }

    return false;
}

bool sdio_is_inserted(void) {
    if (is_sdgecko) {
        return sdgecko_isInserted(drv_no);
    }

    return false;
}

bool sdio_is_sdhc(void) {
    if (is_sdgecko) {
        return sdgecko_isSDHC(drv_no);
    }

    return false;
}

bool sdio_write_sectors(uint32_t sector, uint32_t sec_cnt, void* buffer) {
    if (is_sdgecko) {
        return sdgecko_writeSectors(drv_no, sector, sec_cnt, buffer);
    }

    return false;
}

bool sdio_read_sectors(uint32_t sector, uint32_t sec_cnt, void* buffer) {
    if (is_sdgecko) {
        return sdgecko_readSectors(drv_no, sector, sec_cnt, buffer);
    }

    return false;
}

bool sdio_stop(void) {
    return false;
}

bool sdio_start(void) {
    return false;
}

#endif
