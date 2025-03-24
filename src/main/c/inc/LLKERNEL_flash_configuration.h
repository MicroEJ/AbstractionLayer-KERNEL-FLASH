/*
 * C
 *
 * Copyright 2023-2025 MicroEJ Corp. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be found with this software.
 */

/**
 * @file
 * @brief LLKERNEL flash implementation configuration.
 * @author MicroEJ Developer Team
 * @version 1.0.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef  LLKERNEL_FLASH_CONFIGURATION_H
#define  LLKERNEL_FLASH_CONFIGURATION_H

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
// Includes
// ----------------------------------------------------------------------------

#if !defined(__CC_ARM)
// cppcheck-suppress [misra-c2012-20.9]: The macro '__has_include' is defined by the toolchain.
   #if __has_include("veeport_configuration.h")
	  #include "veeport_configuration.h"
   #else
	  #warning "'veeport_configuration.h' not found, default configuration used for all parameters."
   #endif // __has_include("veeport_configuration.h")
#else
   #warning "This C module needs a 'veeport_configuration.h' in your ARMCC project. "
   #include "veeport_configuration.h"
#endif // !defined ( __CC_ARM)

// ----------------------------------------------------------------------------
// Macros and defines
// ----------------------------------------------------------------------------

/**
 * @brief MicroEJ KF RAM buffer size for features installation. Default is 100 KB.
 */
#if !defined(LLKERNEL_RAM_BUFFER_SIZE)
#define LLKERNEL_RAM_BUFFER_SIZE (1024u * 100u) // 100 KB
#endif //LLKERNEL_RAM_BUFFER_SIZE

/**
 * @brief Typical RAM alignment requirements for an application. Default is 256.
 */
#if !defined(LLKERNEL_RAM_ALIGN_SIZE)
#define LLKERNEL_RAM_ALIGN_SIZE  256u
#endif //LLKERNEL_RAM_ALIGN_SIZE

/**
 * @brief LLKERNEL log level
 */
#if !defined(LLKERNEL_LOG_LEVEL)
#define LLKERNEL_LOG_LEVEL LLKERNEL_LOG_DEBUG
#endif // LLKERNEL_LOG_LEVEL

/**
 * @brief Sets function used to print LLKERNEL logs. (default is printf)
 */
#if !defined(LLKERNEL_TRACE)
#include <stdio.h>
#define LLKERNEL_TRACE  printf
#endif // LLKERNEL_TRACE

/**
 * @brief Start address of the Flash.
 */
#if !defined(LLKERNEL_FLASH_BASE)
#define LLKERNEL_FLASH_BASE         (0x90000000u)
#endif // LLKERNEL_FLASH_BASE

/**
 * @brief Total size of the Flash. Default is 64 MB.
 */
#if !defined(LLKERNEL_FLASH_SIZE)
#define LLKERNEL_FLASH_SIZE         (0x04000000u) // 64 MB
#endif // LLKERNEL_FLASH_SIZE

/**
 * @brief Size of the Flash page. Default is 256 bytes.
 */
#if !defined(LLKERNEL_FLASH_PAGE_SIZE)
#define LLKERNEL_FLASH_PAGE_SIZE    (0x100u) // 256 bytes
#endif // LLKERNEL_FLASH_PAGE_SIZE

/**
 * @brief Typical Flash subsector size. Default is 4 KB.
 */
#if !defined(LLKERNEL_FLASH_SUBSECTOR_SIZE)
#define LLKERNEL_FLASH_SUBSECTOR_SIZE  (4u * 1024u)
#endif //LLKERNEL_FLASH_SUBSECTOR_SIZE

/**
 * @brief The size of the reserved space in the Flash for the Kernel to store features. Default is 4 MB.
 */
#if !defined(LLKERNEL_KF_BLOCK_SIZE)
#define LLKERNEL_KF_BLOCK_SIZE      (0x400000u) // 4 MB
#endif // LLKERNEL_KF_BLOCK_SIZE

/**
 * @brief The start address of the Flash area reserved for feature allocation.
 */
#if !defined(LLKERNEL_KF_START)
#define LLKERNEL_KF_START           ((LLKERNEL_FLASH_BASE + 0u))
#endif // LLKERNEL_KF_START

/**
 * @brief The end address of the Flash area reserved for feature allocation.
 */
#if !defined(LLKERNEL_KF_END)
#define LLKERNEL_KF_END             (LLKERNEL_KF_START + LLKERNEL_KF_BLOCK_SIZE)
#endif // LLKERNEL_KF_END

/**
 * @brief Magic number used for making features as used.
 */
#if !defined(LLKERNEL_FEATURE_USED_MAGIC_NUMBER)
#define LLKERNEL_FEATURE_USED_MAGIC_NUMBER           0x181C77E8u
#endif // LLKERNEL_FEATURE_USED_MAGIC_NUMBER

/**
 * @brief Magic number used for making features as removed.
 */
#if !defined(LLKERNEL_FEATURE_REMOVED_MAGIC_NUMBER)
#define LLKERNEL_FEATURE_REMOVED_MAGIC_NUMBER        0x3ADCA7u
#endif // LLKERNEL_FEATURE_REMOVED_MAGIC_NUMBER

// ----------------------------------------------------------------------------
// End
// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // LLKERNEL_FLASH_CONFIGURATION_H
