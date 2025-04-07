/*
 * C
 *
 * Copyright 2023-2025 MicroEJ Corp. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be found with this software.
 */

/**
 * @file
 * @brief LLKERNEL Flash common defines/functions/structs.
 * @author MicroEJ Developer Team
 * @version 1.0.1
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef LLKERNEL_FLASH_H
#define LLKERNEL_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------

#include "LLKERNEL_flash_configuration.h"

// -----------------------------------------------------------------------------
// Macros and defines
// -----------------------------------------------------------------------------

/**@brief Log priority levels */
#define LLKERNEL_LOG_DEBUG      0
#define LLKERNEL_LOG_INFO       1
#define LLKERNEL_LOG_WARNING    2
#define LLKERNEL_LOG_ERROR      3
#define LLKERNEL_LOG_ASSERT     4
#define LLKERNEL_LOG_NONE       5

#ifndef LLKERNEL_LOG_LEVEL
	#error "LLKERNEL_LOG_LEVEL must be defined"
#endif

/**@brief Debug logger */
#if (LLKERNEL_LOG_DEBUG >= LLKERNEL_LOG_LEVEL)
	#define LLKERNEL_DEBUG_LOG        LLKERNEL_TRACE("[LLKERNEL][D] "); LLKERNEL_TRACE
#else
	#define LLKERNEL_DEBUG_LOG(...)   ((void)0)
#endif

/**@brief Info logger */
#if (LLKERNEL_LOG_INFO >= LLKERNEL_LOG_LEVEL)
	#define LLKERNEL_INFO_LOG         LLKERNEL_TRACE("[LLKERNEL][I] "); LLKERNEL_TRACE
#else
	#define LLKERNEL_INFO_LOG(...)    ((void)0)
#endif

/**@brief Warning logger */
#if (LLKERNEL_LOG_WARNING >= LLKERNEL_LOG_LEVEL)
// cppcheck-suppress [misra-c2012-2.5]: macro defined for optional log usage.
	#define LLKERNEL_WARNING_LOG      LLKERNEL_TRACE("[LLKERNEL][W] "); LLKERNEL_TRACE
#else
	#define LLKERNEL_WARNING_LOG(...) ((void)0)
#endif

/**@brief Error logger */
#if (LLKERNEL_LOG_ERROR >= LLKERNEL_LOG_LEVEL)
	#define LLKERNEL_ERROR_LOG        LLKERNEL_TRACE("[LLKERNEL][E] %s:%d ", __FILE__, __LINE__); LLKERNEL_TRACE
#else
	#define LLKERNEL_ERROR_LOG(...)   ((void)0)
#endif

/**@brief Assert logger */
#if (LLKERNEL_LOG_ASSERT >= LLKERNEL_LOG_LEVEL)
	#define LLKERNEL_ASSERT_LOG(Format, ...)  LLKERNEL_TRACE("[LLKERNEL][A] " Format, __VA_ARGS__); while (1)
#else
	#define LLKERNEL_ASSERT_LOG(...)  ((void)0)
#endif

// -----------------------------------------------------------------------------
// EOF
// -----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // LLKERNEL_FLASH_H
