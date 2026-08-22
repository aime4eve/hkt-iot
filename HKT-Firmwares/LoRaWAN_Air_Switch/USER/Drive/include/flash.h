
#ifndef __FLASH_H__
#define __FLASH_H__


#include <config.h>

#define FMC_PAGE_SIZE           ((uint16_t)0x400U)
#define FMC_WRITE_START_ADDR    ((uint32_t)0x0800C000U)
#define FMC_WRITE_END_ADDR      ((uint32_t)0x08010000U)

#define FMC_REPORT_INTERVAL_ADDR    ((uint32_t)0x0800C000U)


#ifdef __cplusplus
extern "C"
{
#endif

void fmc_erase_pages(u32 addr);
void fmc_program_report_interval(void);
void fmc_read_report_interval(void);
    

#ifdef __cplusplus
}
#endif

#endif
