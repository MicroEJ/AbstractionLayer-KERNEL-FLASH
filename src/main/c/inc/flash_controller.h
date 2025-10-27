/*
 * C
 *
 * Copyright 2025 MicroEJ Corp. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be found with this software.
 */

/**
 * @file
 * @brief Flash controller layer used in the LLKERNEL flash implementation.
 * @author MicroEJ Developer Team
 * @version 1.0.2
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef FLASH_CONTROLLER_H
#define FLASH_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------

#include <stdint.h>

// -----------------------------------------------------------------------------
// Macros and defines
// -----------------------------------------------------------------------------

// Error codes
#define FLASH_CTRL_OK          ((uint32_t)0x00) /** < Successful execution. */
// cppcheck-suppress [misra-c2012-2.5]: false positive, used in the implementation file.
#define FLASH_CTRL_ERROR       ((uint32_t)0x01) /** < Error during execution. */

// --------------------------------------------------------------------------------
//          Additional information about flash memory areas nomenclature
// --------------------------------------------------------------------------------

/*
 * Subsector: It must correspond to the Erase Unit Size, the smallest erasable unit in the selected memory.
 */

/*
 * Page: When writing, flash memory devices temporarily store the data to be written in an internal page buffer.
 * The page size must correspond to the biggest writeable unit in the selected memory.
 */

// --------------------------------------------------------------------------------
//                        Functions that must be implemented
// --------------------------------------------------------------------------------

/**
 * @brief  Initializes and configure the flash memory device interface. This function must be called at the beginning
 * of the main entry point if the device is not already initialized by the BSP. The memory mapped mode must be enabled.
 *
 * @retval FLASH_CTRL_OK on success, FLASH_CTRL_ERROR when the flash memory device returned an error.
 */
uint32_t flash_ctrl_startup(void);

/**
 * @brief  Writes in the flash at the beginning of a page the given content in parameters. The write size should not
 * exceed the page's size.
 * @param  pData Pointer to the data to be written
 * @param  addr Write start address, offset in MCU memory
 * @param  size Size of the data to be written
 *
 * @retval FLASH_CTRL_OK on success, FLASH_CTRL_ERROR if an error occurs.
 *
 * @note If a cache is enabled, invalidate the cache of the memory area updated before the return statement.
 */
uint32_t flash_ctrl_page_write(uint8_t *pData, uint32_t addr, uint32_t size);

/**
 * @brief  Erases a subsector of the flash memory that contains the address given in parameters.
 * @param  addr Subsector address to erase, offset in MCU memory
 *
 * @retval FLASH_CTRL_OK on success, FLASH_CTRL_ERROR if an error occurs.
 *
 * @attention If a cache is enabled, invalidate the cache of the memory area updated before the return statement.
 */
uint32_t flash_ctrl_erase_subsector(uint32_t addr);

/**
 * @brief  Configures the flash controller in memory mapped for read operations.
 *
 * @retval FLASH_CTRL_OK on success, FLASH_CTRL_ERROR if an error occurs.
 */
uint32_t flash_ctrl_enable_memory_mapped_mode(void);

/**
 * @brief  Exits the flash controller from the memory mapped mode.
 *
 * @retval FLASH_CTRL_OK on success, FLASH_CTRL_ERROR if an error occurs.
 */
uint32_t flash_ctrl_disable_memory_mapped_mode(void);

/**
 * @brief  Obtains the start address of the subsector which contains the parameter address.
 * @param  addr address within a subsector
 *
 * @retval The subsector start address.
 */
uint32_t flash_ctrl_get_subsector_address(uint32_t address);

/**
 * @brief  Obtains the start address of the page which contains the parameter address.
 * @param  addr address within a page
 *
 * @retval The page start address.
 */
uint32_t flash_ctrl_get_page_address(uint32_t address);

/**
 * @brief  Obtains the size of a subsector.
 *
 * @retval The subsector size in bytes.
 */
uint32_t flash_ctrl_get_subsector_size(void);

/**
 * @brief  Obtains the size of a page.
 *
 * @retval The page size in bytes.
 */
uint32_t flash_ctrl_get_page_size(void);

/**
 * @brief  Obtains the start address of the kernel feature reserved area.
 *
 * @retval kf area start address
 */
uint32_t flash_ctrl_get_kf_start_address(void);

/**
 * @brief  Obtains the end address of the kernel feature reserved area.
 *
 * @retval kf area end address
 */
uint32_t flash_ctrl_get_kf_end_address(void);

// -----------------------------------------------------------------------------
// EOF
// -----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // FLASH_CONTROLLER_H
