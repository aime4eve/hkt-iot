/**
 * @file    upgrade_flash.h
 * @brief   Self-contained external SPI NOR flash driver (Puya P25Q40, 512KB)
 *          for the upgrade path. Owns SPI3 + CS(PA15) + /WP(PB4) so it builds
 *          identically in the APP and BootLoader projects.
 *
 *          Concurrency: the upgrade is a single-threaded bare-metal flow; do
 *          not call these from ISR context.
 *
 *          Millisecond tick: each project must provide
 *              uint32_t upg_tick_ms(void);
 *          (wrap get_syspant_ms()).
 */
#ifndef __UPGRADE_FLASH_H__
#define __UPGRADE_FLASH_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* provided by the integrator (APP and BootLoader each) */
extern uint32_t upg_tick_ms(void);

/* Init SPI3 + GPIO, release power-down, sanity-check the JEDEC ID.
 * @return 0 ok, -1 no usable flash */
int upg_flash_init(void);

/* Release power-down (e.g. after STOP2 wake) and re-check presence. */
int upg_flash_wake(void);

/* Enter power-down (before STOP2). */
void upg_flash_powerdown(void);

/* Read @addr into buf. @return 0 ok, -1 out-of-range/param */
int upg_flash_read(uint32_t addr, uint8_t *buf, uint32_t len);

/* Program bytes starting at @addr (page program loop; target must be erased).
 * @return 0 ok, -1 out-of-range/param/timeout */
int upg_flash_write(uint32_t addr, const uint8_t *buf, uint32_t len);

/* Erase the 4KB sector containing @addr (addr is truncated to sector base).
 * @return 0 ok, -1 out-of-range/timeout */
int upg_flash_erase_4k(uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif /* __UPGRADE_FLASH_H__ */
