/*
 * C
 *
 * Copyright 2021-2025 MicroEJ Corp. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be found with this software.
 *
 */

/**
 * @file
 * @brief LLKERNEL Flash implementation over an external Flash memory.
 * @author MicroEJ Developer Team
 * @version 1.0.2
 */

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------

#include <string.h>
#include <stdbool.h>

#include "LLKERNEL_impl.h"
#include "LLKERNEL_flash.h"
#include "flash_controller.h"

// -----------------------------------------------------------------------------
// Macros and defines
// -----------------------------------------------------------------------------

#define UNUSED_RETURN(x) (void)(x)

// -----------------------------------------------------------------------------
// Typedef and Structure
// -----------------------------------------------------------------------------

typedef struct {
	uint32_t status;
	uint32_t nb_subsectors;
	uint32_t rom_address;
	uint32_t rom_size;
	uint32_t ram_address;
	uint32_t ram_size;
	uint32_t feature_index;
	uint32_t reserved; // Reserved word to align the rom area over 16 bytes.
} feature_header_t;

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

// Variables for flash page write buffer and feature_globals
static uint8_t mem_writeBuffer[LLKERNEL_FLASH_PAGE_SIZE] = { 0 };
static uint8_t *target_page_address = NULL; // destination ROM page address to write content of mem_writeBuffer to
static uint32_t mem_writeBuffer_offset = 0; // number of bytes stored in mem_writeBuffer

// features variables
static feature_header_t *last_feature_ptr = NULL;
static uint32_t nb_features = 0;

// Get LLKERNEL max number of dynamic features, a link-time option.
extern void _java_max_nb_dynamic_features;
static uint32_t kernel_max_nb_dynamic_features = (uint32_t)(&_java_max_nb_dynamic_features);

// -----------------------------------------------------------------------------
// Private functions
// -----------------------------------------------------------------------------

static inline bool is_feature_used(uint32_t feature_status);
static inline bool is_feature_removed(uint32_t feature_status);
static uint32_t llkernel_get_kf_area_size(void);
static uint32_t llkernel_get_feature_slot_size_rom_bytes(void);
static uint32_t llkernel_get_next_aligned_ram_address(uint32_t address);
static int llkernel_feature_is_in_last_kf_slot(feature_header_t *feature_ptr);
static feature_header_t * llkernel_get_next_feature(feature_header_t *feature_ptr);
static feature_header_t * llkernel_get_free_feature_slot(void);
static const char *llkernel_error_code_to_str(uint32_t error_code);
static uint32_t llkernel_flash_write(uint8_t *input_buffer, uint32_t flash_start_address, uint32_t size);

/**
 * @brief  Checks if the feature in the slot is used.
 * @retval true if used, false otherwise.
 */
static inline bool is_feature_used(uint32_t feature_status) {
	return (LLKERNEL_FEATURE_USED_MAGIC_NUMBER == feature_status);
}

/**
 * @brief  Checks if the feature in the slot has been removed.
 * @retval true if removed, false otherwise.
 */
static inline bool is_feature_removed(uint32_t feature_status) {
	return (LLKERNEL_FEATURE_REMOVED_MAGIC_NUMBER == feature_status);
}

/**
 * @brief  Obtains the size of the kernel feature reserved area.
 * @retval kf area size
 */
uint32_t llkernel_get_kf_area_size(void) {
	return flash_ctrl_get_kf_end_address() - flash_ctrl_get_kf_start_address();
}

/**
 * @brief Computes and returns the maximum ROM size in bytes that can be allocated per feature.
 *
 * @retval Maximum feature ROM size in bytes.
 */
static uint32_t llkernel_get_feature_slot_size_rom_bytes(void) {
	uint32_t nb_subsector_kf_area;
	uint32_t nb_subsector_per_slot;
	uint32_t result = 0;

	if (0u != kernel_max_nb_dynamic_features) {
		// Divisions done first to avoid an overflow.
		nb_subsector_kf_area = llkernel_get_kf_area_size() / LLKERNEL_FLASH_SUBSECTOR_SIZE;
		nb_subsector_per_slot = nb_subsector_kf_area / kernel_max_nb_dynamic_features;
		result = nb_subsector_per_slot * LLKERNEL_FLASH_SUBSECTOR_SIZE;
	}
	return result;
}

/**
 * @brief Computes and returns the next ram address aligned on LLKERNEL_RAM_ALIGN_SIZE bits.
 *
 * @param[in] address The input address where the next aligned address must be found.
 *
 * @retval The next aligned ram address.
 */
static uint32_t llkernel_get_next_aligned_ram_address(uint32_t address) {
	uint32_t ram_address = (address & ~(LLKERNEL_RAM_ALIGN_SIZE - 1u)) + LLKERNEL_RAM_ALIGN_SIZE;
	return ram_address;
}

/**
 * @brief Checks if the feature targeted by feature_ptr is in the last feature slot.
 *
 * @param[in] feature_ptr The feature header structure pointer of the target feature.
 *
 * @retval Returns true if the feature targeted by feature_ptr is in the last feature slot, false otherwise.
 */
static int llkernel_feature_is_in_last_kf_slot(feature_header_t *feature_ptr) {
	return (((uint32_t)feature_ptr + llkernel_get_feature_slot_size_rom_bytes()) >= flash_ctrl_get_kf_end_address());
}

/**
 * @brief Retrieves the structure pointer of the next feature.
 *
 * @param[in] feature_ptr Pointer to the current feature.
 *
 * @retval Returns the address of the next feature header structure, NULL if the current feature is in the last
 * feature slot.
 */
static feature_header_t * llkernel_get_next_feature(feature_header_t *feature_ptr) {
	feature_header_t *retval = NULL;
	uint32_t feature_slot_size_bytes = llkernel_get_feature_slot_size_rom_bytes();

	if (0u != feature_slot_size_bytes) {
		if (!llkernel_feature_is_in_last_kf_slot(feature_ptr)) {
			retval = (feature_header_t *)(((uint32_t)feature_ptr) + feature_slot_size_bytes);
		}
	}
	return retval;
}

/**
 * @brief Gives the pointer to the next free feature slot.
 *
 * @retval A feature header structure pointer that points to the first unused feature slot.
 */
static feature_header_t * llkernel_get_free_feature_slot(void) {
	feature_header_t *feature_ptr = (feature_header_t *)flash_ctrl_get_kf_start_address();

	do {
		// return first unused feature slot
		if (!is_feature_used(feature_ptr->status)) {
			break; // Leaves the loop to return the current feature_ptr.
		}

		feature_ptr = llkernel_get_next_feature(feature_ptr);
	} while (NULL != feature_ptr);
	return feature_ptr;
}

/**
 * @brief Converts into a string the error code received when an error occurs in a feature initialization.
 *
 * @param[in] error_code Error code of a feature initialization failure.
 *
 * @retval Returns the string equivalent (const char*) of the error code in parameter.
 */
static const char *llkernel_error_code_to_str(uint32_t error_code) {
	const char *str = "";
	switch (error_code) {
	case LLKERNEL_FEATURE_INIT_ERROR_CORRUPTED_CONTENT:
		str = "CORRUPTED CONTENT";
		break;

	case LLKERNEL_FEATURE_INIT_ERROR_INCOMPATIBLE_KERNEL_WRONG_UID:
		str = "INCOMPATIBLE_KERNEL_WRONG_UID";
		break;

	case LLKERNEL_FEATURE_INIT_ERROR_TOO_MANY_INSTALLED:
		str = "TOO_MANY_INSTALLED";
		break;

	case LLKERNEL_FEATURE_INIT_ERROR_ALREADY_INSTALLED:
		str = "ALREADY_INSTALLED";
		break;

	case LLKERNEL_FEATURE_INIT_ERROR_INCOMPATIBLE_KERNEL_WRONG_ADDRESSES:
		str = "INCOMPATIBLE_KERNEL_WRONG_ADDRESSES";
		break;

	case LLKERNEL_FEATURE_INIT_ERROR_ROM_OVERLAP:
		str = "ROM_OVERLAP";
		break;

	case LLKERNEL_FEATURE_INIT_ERROR_RAM_OVERLAP:
		str = "RAM_OVERLAP";
		break;

	case LLKERNEL_FEATURE_INIT_ERROR_RAM_ADDRESS_CHANGED:
		str = "RAM_ADDRESS_CHANGED";
		break;

	default:
		LLKERNEL_ASSERT_LOG("No LLKERNEL error code found for %d\n", error_code);
		break;
	}
	return str;
}

/**
 * @brief Writes into the flash an amount of data. This function is to use when the amount of data to write is higher
 * than the size of a page.
 *
 * @param[in] input_buffer Pointer to the data to write into the flash.
 * @param[in] flash_start_address The start address of the target flash area.
 * @param[in] size The amount of bytes to write into the flash.
 *
 * @retval FLASH_CTRL_OK on success, FLASH_CTRL_ERROR when the flash memory device returned an error.
 */
static uint32_t llkernel_flash_write(uint8_t *input_buffer, uint32_t flash_start_address, uint32_t size) {
	uint32_t remaining = size;
	uint8_t *input_buffer_ptr = input_buffer;
	uint32_t current_flash_address = flash_start_address;
	uint32_t result = FLASH_CTRL_OK;
	while (0u < remaining) {
		uint32_t copy_size = flash_ctrl_get_page_size();
		if (copy_size > remaining) {
			copy_size = remaining;
		}
		if (FLASH_CTRL_OK != flash_ctrl_page_write((uint8_t *)input_buffer_ptr, current_flash_address,
		                                           copy_size)) {
			LLKERNEL_ERROR_LOG("%s: Flash error during attempt to write at the address 0x%x in the flash.\n",
			                   __func__, current_flash_address);
			result = FLASH_CTRL_ERROR;
			break; // Leaves the loop after an error.
		}
		remaining -= copy_size;
		// cppcheck-suppress [misra-c2012-18.4]: points after the + operation.
		// cppcheck-suppress [misra-c2012-10.3]: this is a 8 bits pointer that stores a 32 bits address.
		input_buffer_ptr += copy_size;
		current_flash_address += copy_size;
	}
	return result;
}

// -----------------------------------------------------------------------------
// LLKERNEL_IMPL function implementations
// -----------------------------------------------------------------------------

// See the header file for the function documentation
// cppcheck-suppress [misra-c2012-5.5]: macro with same name generated in intern/LLKERNEL_impl.h.
// cppcheck-suppress [misra-c2012-8.7]: API function that can be used in another file.
int32_t LLKERNEL_IMPL_getAllocatedFeaturesCount(void) {
	LLKERNEL_DEBUG_LOG("%s\n", __func__);
	// Not allocated in the stack
	static uint8_t alloc_feature_buffer[LLKERNEL_FLASH_SUBSECTOR_SIZE] __attribute__((section(
																						  ".bss.microej.llkernel")));

	feature_header_t *feature_ptr = (feature_header_t *)flash_ctrl_get_kf_start_address();
	nb_features = 0;
	// If the number of features remains 0, ensure the last feature pointer is set to NULL.
	// This avoids an inconsistent state where there are no features, but the pointer is still
	// set to an address, which may result in an erroneous dereferencing.
	last_feature_ptr = NULL;
	uint8_t flash_error_occurred = false;
	do {
		if (is_feature_used(feature_ptr->status)) {
			if (feature_ptr->feature_index != nb_features) {
				uint8_t *mem_buffer = alloc_feature_buffer;
				uint32_t subsector_size = flash_ctrl_get_subsector_size();
				UNUSED_RETURN(memcpy((void *)mem_buffer, (const void *)feature_ptr, subsector_size));

				// cppcheck-suppress [misra-c2012-11.3] : cast used by many C framework to factorize code.
				feature_header_t *mem_buffer_feature_ptr = (feature_header_t *)mem_buffer;
				mem_buffer_feature_ptr->feature_index = nb_features;

				UNUSED_RETURN(flash_ctrl_disable_memory_mapped_mode());
				if (FLASH_CTRL_OK != flash_ctrl_erase_subsector((uint32_t)feature_ptr)) {
					LLKERNEL_ERROR_LOG(
						"%s: Flash error during attempt to erase the subsector at the address 0x%x in the flash.\n",
						__func__, (uint32_t)feature_ptr);
					flash_error_occurred = true; // Leaves the loop after this iteration.
				}
				if (FLASH_CTRL_OK != llkernel_flash_write(mem_buffer, (uint32_t)feature_ptr, subsector_size)) {
					flash_error_occurred = true; // Leaves the loop after this iteration.
				}
				if (FLASH_CTRL_OK != flash_ctrl_enable_memory_mapped_mode()) {
					LLKERNEL_ERROR_LOG("%s: Could not enable the memory mapped mode \n", __func__);
				}
			}
			last_feature_ptr = feature_ptr;
			nb_features += 1u;
		}

		if (!is_feature_used(feature_ptr->status) && !is_feature_removed(feature_ptr->status)) {
			// free slot for features
			break; // Leaves the loop to return the current nb_features.
		}
		feature_ptr = llkernel_get_next_feature(feature_ptr);
	} while ((NULL != feature_ptr) && (!flash_error_occurred));

	return nb_features;
}

// See the header file for the function documentation
int32_t LLKERNEL_IMPL_getFeatureHandle(int32_t allocation_index) {
	LLKERNEL_DEBUG_LOG("%s (%d)\n", __func__, allocation_index);

	int32_t result = 0;
	feature_header_t *feature_ptr = (feature_header_t *)flash_ctrl_get_kf_start_address();

	// Retrieves the feature handle when allocation_index is in nb_features range.
	if ((uint32_t)allocation_index < nb_features) {
		uint8_t is_leaving_loop = false;
		do {
			if (!is_feature_used(feature_ptr->status) && !is_feature_removed(feature_ptr->status)) {
				// Reach end of installed features
				is_leaving_loop = true;
			}

			if (is_feature_used(feature_ptr->status) && (feature_ptr->feature_index == (uint32_t)allocation_index)) {
				result = (int32_t)feature_ptr;
				is_leaving_loop = true;
			}

			if (true == is_leaving_loop) {
				break; // Leaves the loop to return the current feature_ptr.
			}
			feature_ptr = llkernel_get_next_feature(feature_ptr);
		}while (NULL != feature_ptr);
	}
	return result;
}

// See the header file for the function documentation
void * LLKERNEL_IMPL_getFeatureAddressRAM(int32_t handle) {
	LLKERNEL_DEBUG_LOG("%s : 0x%.8x\n", __func__, (uint32_t)handle);

	void *result = NULL;
	feature_header_t *feature_ptr = (feature_header_t *)handle;
	if (is_feature_used(feature_ptr->status)) {
		LLKERNEL_DEBUG_LOG("%s (0x%.8x): 0x%.8x\n", __func__, (uint32_t)handle, feature_ptr->ram_address);
		result = (void *)feature_ptr->ram_address;
	}
	return result;
}

// See the header file for the function documentation
void * LLKERNEL_IMPL_getFeatureAddressROM(int32_t handle) {
	LLKERNEL_DEBUG_LOG("%s 0x%.8x \n", __func__, (uint32_t)handle);

	void *result = NULL;
	feature_header_t *feature_ptr = (feature_header_t *)handle;
	if (is_feature_used(feature_ptr->status)) {
		LLKERNEL_DEBUG_LOG("%s (0x%.8x): 0x%.8x\n", __func__, (uint32_t)handle, feature_ptr->rom_address);
		result = (void *)feature_ptr->rom_address;
	}
	return result;
}

// See the header file for the function documentation
void LLKERNEL_IMPL_freeFeature(int32_t handle) {
	LLKERNEL_DEBUG_LOG("%s : 0x%.8x \n", __func__, (uint32_t)handle);

	feature_header_t *feature_ptr = (feature_header_t *)handle;

	if (is_feature_used(feature_ptr->status)) {
		// cppcheck-suppress [misra-c2012-11.3] : mem_writeBuffer is a byte buffer, cast necessary to use the data.
		feature_header_t *mem_buffer_feature_ptr = (feature_header_t *)mem_writeBuffer;

		UNUSED_RETURN(memcpy((void *)mem_buffer_feature_ptr, (const void *)feature_ptr, sizeof(feature_header_t)));
		mem_buffer_feature_ptr->status = LLKERNEL_FEATURE_REMOVED_MAGIC_NUMBER;
		mem_buffer_feature_ptr->nb_subsectors = 1;
		uint32_t subsector_address = (uint32_t)handle;
		UNUSED_RETURN(flash_ctrl_disable_memory_mapped_mode());

		// Erases only the subsector where the feature_header structure is located.
		if (FLASH_CTRL_OK != flash_ctrl_erase_subsector(subsector_address)) {
			LLKERNEL_ERROR_LOG(
				"%s: Flash error during attempt to erase the subsector at the address 0x%x in the flash.\n",
				__func__, subsector_address);
		}

		if (FLASH_CTRL_OK != flash_ctrl_page_write((uint8_t *)mem_buffer_feature_ptr, (uint32_t)feature_ptr,
		                                           sizeof(feature_ptr))) {
			LLKERNEL_ERROR_LOG("%s: Flash error during attempt to write at the address 0x%x in the flash.\n", __func__,
			                   (uint32_t)feature_ptr);
		}
		if (FLASH_CTRL_OK != flash_ctrl_enable_memory_mapped_mode()) {
			LLKERNEL_ERROR_LOG("%s: Could not enable the memory mapped mode \n", __func__);
		}

		nb_features -= 1u;
	}
}

// See the header file for the function documentation
int32_t LLKERNEL_IMPL_allocateFeature(int32_t size_ROM, int32_t size_RAM) {
	LLKERNEL_DEBUG_LOG("%s (0x%.8x, 0x%.8x)\n", __func__, (uint32_t)size_ROM, (uint32_t)size_RAM);
	// Not allocated in the stack, and ensuring that the buffer is aligned correctly.
	static uint8_t kernel_ram_buffer[LLKERNEL_RAM_BUFFER_SIZE]
	__attribute__((section(".bss.microej.llkernel")))
	__attribute__((aligned(LLKERNEL_RAM_ALIGN_SIZE)));

	int32_t result = -1;
	uint32_t status = FLASH_CTRL_OK;
	uint32_t current_feature_address = 0;
	uint32_t current_ram_address;
	feature_header_t *mem_buffer_feature_ptr;

	// Check the max number of dynamic feature allocations.
	if (0u == kernel_max_nb_dynamic_features) {
		LLKERNEL_ERROR_LOG("kernel_max_nb_dynamic_features is equal to 0. \n");
		result = 0;
	}

	// limit feature  ROM size
	if (llkernel_get_feature_slot_size_rom_bytes() < ((uint32_t)size_ROM + sizeof(mem_buffer_feature_ptr))) {
		LLKERNEL_ERROR_LOG("%s: requested ROM size larger than maximum feature size (%d bytes)\n", __func__,
		                   (int)size_ROM);
		result = 0;
	}

	// limit feature RAM size
	if (LLKERNEL_RAM_BUFFER_SIZE < (uint32_t)size_RAM) {
		LLKERNEL_ERROR_LOG("%s: requested RAM size larger than maximum feature size (%d bytes)\n", __func__,
		                   (int)size_RAM);
		result = 0;
	}

	if (0 != result) {
		// Count feature to update last_feature_ptr;
		UNUSED_RETURN(LLKERNEL_IMPL_getAllocatedFeaturesCount());

		current_feature_address = (uint32_t)llkernel_get_free_feature_slot( );

		if (NULL == (uint32_t *)current_feature_address) {
			// no more space for features
			LLKERNEL_ERROR_LOG("%s: The maximum number of features installed in flash reached (%d)\n", __func__,
			                   (int)kernel_max_nb_dynamic_features);
			result = 0;
		}
	}

	if (0 != result) {
		if (NULL == last_feature_ptr) {
			// This is the first feature
			current_ram_address = (uint32_t)&kernel_ram_buffer;
		} else {
			if (is_feature_removed(((feature_header_t *)current_feature_address)->status) &&
			    (((feature_header_t *)current_feature_address)->ram_address >= (uint32_t)&kernel_ram_buffer) &&
			    (((feature_header_t *)current_feature_address)->ram_address <
			     (uint32_t)&kernel_ram_buffer + LLKERNEL_RAM_BUFFER_SIZE) &&
			    ((uint32_t)size_RAM <= ((feature_header_t *)current_feature_address)->ram_size)
			    ) {
				// Feature for current slot has been removed, re-use the current slot.
				current_ram_address = ((feature_header_t *)current_feature_address)->ram_address;
			} else {
				// Allocate a new slot for the new feature.
				current_ram_address = llkernel_get_next_aligned_ram_address(last_feature_ptr->ram_address +
				                                                            last_feature_ptr->ram_size);
				if (current_ram_address > ((uint32_t)&kernel_ram_buffer[LLKERNEL_RAM_BUFFER_SIZE - 1u])) {
					// no more space for feature
					LLKERNEL_ERROR_LOG("%s: No more space to allocate RAM for feature (overflow of %d bytes)\n",
					                   __func__,
					                   (current_ram_address -
					                    ((uint32_t)&kernel_ram_buffer[LLKERNEL_RAM_BUFFER_SIZE - 1u])));
					result = 0;
				}
			}
		}
	}

	// cppcheck-suppress [misra-c2012-11.3] : mem_writeBuffer is a byte buffer, cast necessary to use the data.
	mem_buffer_feature_ptr = (feature_header_t *)mem_writeBuffer;
	// Clear all corresponding subsectors
	uint32_t address = current_feature_address;
	int nb_subsectors = 0;

	if (0 != result) {
		// Set rom address and size
		mem_buffer_feature_ptr->rom_address = current_feature_address + sizeof(feature_header_t);
		mem_buffer_feature_ptr->rom_size = size_ROM;
		UNUSED_RETURN(flash_ctrl_disable_memory_mapped_mode());
		while (address < (mem_buffer_feature_ptr->rom_address + mem_buffer_feature_ptr->rom_size)) {
			nb_subsectors++;
			status = flash_ctrl_erase_subsector(address);
			if (FLASH_CTRL_OK != status) {
				LLKERNEL_ERROR_LOG("%s: flash erase 0x%.8x failed\n", __func__, (unsigned int)address);
				result = 0;
				break;
			}
			address += flash_ctrl_get_subsector_size();
		}
		if (FLASH_CTRL_OK != flash_ctrl_enable_memory_mapped_mode()) {
			LLKERNEL_ERROR_LOG("%s: Could not enable the memory mapped mode \n", __func__);
		}
	}

	if (0 != result) {
		mem_buffer_feature_ptr->nb_subsectors = nb_subsectors;
		// Set ram address and size
		mem_buffer_feature_ptr->ram_address = current_ram_address;
		mem_buffer_feature_ptr->ram_size = size_RAM;

		// Set status
		mem_buffer_feature_ptr->status = LLKERNEL_FEATURE_USED_MAGIC_NUMBER;

		for (uint32_t i = sizeof(feature_header_t); i < flash_ctrl_get_page_size(); i++) {
			// cppcheck-suppress [misra-c2012-18.4]: points after the + operation
			*(((uint8_t *)mem_buffer_feature_ptr) + i) = 0xFF;
		}

		mem_buffer_feature_ptr->feature_index = nb_features;

		UNUSED_RETURN(flash_ctrl_disable_memory_mapped_mode());
		// Write feature header in flash to reserve the ROM area.
		status = flash_ctrl_page_write((uint8_t *)mem_buffer_feature_ptr, current_feature_address,
		                               flash_ctrl_get_page_size());
		if (FLASH_CTRL_OK != status) {
			LLKERNEL_ERROR_LOG("%s: flash write 0x%.8x failed\n", __func__, (int)address);
			result = 0;
		} else {
			last_feature_ptr = (feature_header_t *)current_feature_address;
			nb_features += 1u;
			result = current_feature_address;
		}
		if (FLASH_CTRL_OK != flash_ctrl_enable_memory_mapped_mode()) {
			LLKERNEL_ERROR_LOG("%s: Could not enable the memory mapped mode \n", __func__);
		}
	}

	return result;
}

// See the header file for the function documentation
int32_t LLKERNEL_IMPL_onFeatureInitializationError(int32_t handle, int32_t error_code) {
	LLKERNEL_ERROR_LOG("Failed to initialize feature handle 0x%.8x with error %d(%s)\n", (int)handle, (int)error_code,
	                   llkernel_error_code_to_str(error_code));
	switch (error_code) {
	case LLKERNEL_FEATURE_INIT_ERROR_CORRUPTED_CONTENT:
		LLKERNEL_ERROR_LOG("Feature detected but content is corrupted, uninstalled to free the memory.\n");
		LLKERNEL_IMPL_freeFeature(handle);
		break;

	case LLKERNEL_FEATURE_INIT_ERROR_INCOMPATIBLE_KERNEL_WRONG_UID:
		LLKERNEL_ERROR_LOG("Feature detected but not compatible with the Kernel, uninstalled to free the memory.\n");
		LLKERNEL_IMPL_freeFeature(handle);
		break;

	case LLKERNEL_FEATURE_INIT_ERROR_INCOMPATIBLE_KERNEL_WRONG_ADDRESSES:
		LLKERNEL_ERROR_LOG("Feature detected but addresses are wrong, uninstalled to free the memory.\n");
		LLKERNEL_IMPL_freeFeature(handle);
		break;

	default:
		break;
	}

	return LLKERNEL_OK;
}

// See the header file for the function documentation
int32_t LLKERNEL_IMPL_copyToROM(void *dest_address_ROM, void *src_address, int32_t size) {
	// cppcheck-suppress [misra-c2012-11.6]: void pointer cast to display the address targeted.
	LLKERNEL_DEBUG_LOG("%s(dest=0x%.8x, src=0x%.8x, size=0x%.8x)\n", __func__, (uint32_t)dest_address_ROM,
	                   (uint32_t)src_address, (uint32_t)size); // cppcheck-suppress [misra-c2012-11.6]: void pointer
	                                                           // cast to display the address targeted.

	int32_t result = LLKERNEL_OK;
	// cppcheck-suppress [misra-c2012-11.5]: Used for code genericity/abstraction
	uint8_t *dest_ptr = dest_address_ROM;
	// cppcheck-suppress [misra-c2012-11.5]: Used for code genericity/abstraction
	uint8_t *src_ptr = src_address;
	uint32_t remaining = size;

	if (flash_ctrl_get_kf_start_address() > (uint32_t)dest_ptr ||
	    (flash_ctrl_get_kf_end_address()) <= (uint32_t)dest_ptr) {
		LLKERNEL_ERROR_LOG("%s: feature cannot be installed outside of defined ROM area (0x%.8x)\n", __func__,
		                   (uint32_t)dest_ptr);
		result = LLKERNEL_ERROR;
	}

	if (LLKERNEL_OK == result) {
		if ((flash_ctrl_get_kf_end_address()) < ((uint32_t)dest_ptr + (uint32_t)size)) {
			LLKERNEL_ERROR_LOG("%s: feature extents outside of defined ROM area (%d bytes from 0x%.8x)\n", __func__,
			                   size, (uint32_t)dest_ptr);
			result = LLKERNEL_ERROR;
		}

		if (llkernel_get_feature_slot_size_rom_bytes() < (uint32_t)size) {
			LLKERNEL_ERROR_LOG("%s: feature size larger than maximum allowed size (%d bytes)\n", __func__, size);
			result = LLKERNEL_ERROR;
		}

		uint32_t feature_slot_size = llkernel_get_feature_slot_size_rom_bytes();
		if (0u != feature_slot_size) {
			uint32_t index_feature_slot_src_addr = ((uint32_t)dest_ptr - flash_ctrl_get_kf_start_address()) /
			                                       feature_slot_size;
			uint32_t index_feature_slot_end_addr = (((uint32_t)dest_ptr + (uint32_t)size -
			                                         flash_ctrl_get_kf_start_address())) / feature_slot_size;
			if (index_feature_slot_src_addr != index_feature_slot_end_addr) {
				LLKERNEL_ERROR_LOG(
					"%s: The ROM copy overlaps another feature slot (start addr: 0x%x ; end addr: 0x%x) \n", __func__,
					(uint32_t)dest_ptr, ((uint32_t)dest_ptr + (uint32_t)size));
				result = LLKERNEL_ERROR;
			}
		}
	}

	if (LLKERNEL_OK == result) {
		if (target_page_address != NULL) {
			// Check if there are data buffered from previous call.
			LLKERNEL_DEBUG_LOG("%s: Re-using buffered data from previous call (target=0x%x, size=0x%x)\n", __func__,
			                   (uint32_t)target_page_address, mem_writeBuffer_offset);
			// cppcheck-suppress [misra-c2012-11.6]: new address computation using the value stored in the pointer.
			uint32_t new_offset = (uint32_t)dest_address_ROM - (uint32_t)target_page_address;
			if ((new_offset > mem_writeBuffer_offset) && (new_offset < flash_ctrl_get_page_size())) {
				// Data already copied, bytes skipped to new_offset.
				LLKERNEL_DEBUG_LOG("%s: %d bytes skipped\n", __func__, new_offset - mem_writeBuffer_offset);
				mem_writeBuffer_offset = new_offset;
			} else if (new_offset != mem_writeBuffer_offset) {
				// Flushes the buffered data from the previous call.
				result = LLKERNEL_IMPL_flushCopyToROM();
			} else {
				// Nothing to do.
			}
		}
	}

	if (LLKERNEL_OK == result) {
		UNUSED_RETURN(flash_ctrl_disable_memory_mapped_mode());
		while (0u < remaining) {
			uint32_t page_address = flash_ctrl_get_page_address((uint32_t)dest_ptr);
			uint32_t buffer_offset = (uint32_t)dest_ptr - page_address;
			uint32_t copy_size = flash_ctrl_get_page_size() - buffer_offset;
			if (copy_size > remaining) {
				copy_size = remaining;
			}

			// If the buffer offset is not null, we need to read the flash to not overwrite a part of the page.
			if ((target_page_address == NULL) && (0u != buffer_offset)) {
				if (FLASH_CTRL_OK != flash_ctrl_enable_memory_mapped_mode()) {
					LLKERNEL_ERROR_LOG("%s: Could not enable the memory mapped mode \n", __func__);
				}
				const uint32_t *ptr_page_address = (uint32_t *)page_address;
				LLKERNEL_DEBUG_LOG("%s: page read (addr: 0x%.8x, len: 0x%.8x)\n", __func__,
				                   (int)ptr_page_address, flash_ctrl_get_page_size());
				UNUSED_RETURN(memcpy((void *)mem_writeBuffer, (const void *)ptr_page_address,
				                     flash_ctrl_get_page_size()));
				UNUSED_RETURN(flash_ctrl_disable_memory_mapped_mode());
			}

			// Copy into the write buffer the desired content.
			// cppcheck-suppress [misra-c2012-18.4]: points after the + operation.
			UNUSED_RETURN(memcpy((void *)((mem_writeBuffer) + buffer_offset), (const void *)src_ptr, copy_size));
			// cppcheck-suppress [misra-c2012-18.4]: points after the + operation.
			if (memcmp((uint8_t *)(mem_writeBuffer + buffer_offset), src_ptr, copy_size) != 0) {
				LLKERNEL_ERROR_LOG("%s: Write to buffer invalid\n", __func__);
			}

			if ((copy_size + buffer_offset) == flash_ctrl_get_page_size()) {
				LLKERNEL_DEBUG_LOG("%s: page write (addr: 0x%.8x, off: 0x%.8x, len: 0x%.8x)\n", __func__,
				                   (int)page_address, (int)buffer_offset, (int)(buffer_offset + copy_size);
				if (FLASH_CTRL_OK != flash_ctrl_page_write((uint8_t *)mem_writeBuffer, page_address,
				                                           flash_ctrl_get_page_size())) {
					LLKERNEL_ERROR_LOG("%s: flash write 0x%.8x failed\n", __func__, (int)page_address);
					result = LLKERNEL_ERROR;
					break; // Leaves the loop to return the error code.
				}

				UNUSED_RETURN(flash_ctrl_enable_memory_mapped_mode());
				if (memcmp((uint8_t *)(page_address + buffer_offset), src_ptr, copy_size) != 0) {
					LLKERNEL_ERROR_LOG("%s: Flash write invalid\n", __func__);
				}
				if (memcmp(mem_writeBuffer, (uint8_t *)page_address, flash_ctrl_get_page_size()) != 0) {
					LLKERNEL_ERROR_LOG("%s: Flash write from buffer invalid\n", __func__);
				}
				UNUSED_RETURN(flash_ctrl_disable_memory_mapped_mode());
			} else {
				target_page_address = (uint8_t *)page_address;
				mem_writeBuffer_offset = copy_size + buffer_offset;
			}

			// cppcheck-suppress [misra-c2012-18.4]: points after the + operation.
			// cppcheck-suppress [misra-c2012-10.3]: this is a 8 bits pointer that stores a 32 bits address.
			dest_ptr += copy_size;
			// cppcheck-suppress [misra-c2012-10.3]: this is a 8 bits pointer that stores a 32 bits address.
			// cppcheck-suppress [misra-c2012-18.4]: points after the + operation.
			src_ptr += copy_size;
			remaining -= copy_size;
		}
		if (FLASH_CTRL_OK != flash_ctrl_enable_memory_mapped_mode()) {
			LLKERNEL_ERROR_LOG("%s: Could not enable the memory mapped mode \n", __func__);
		}
	}
	return result;
}

// See the header file for the function documentation
// cppcheck-suppress [misra-c2012-5.5]: macro with same name generated in intern/LLKERNEL_impl.h.
// cppcheck-suppress [misra-c2012-8.7]: API function, external linkage mandatory.
int32_t LLKERNEL_IMPL_flushCopyToROM(void) {
	LLKERNEL_DEBUG_LOG("%s\n", __func__);
	int32_t result = LLKERNEL_OK;

	if (target_page_address != NULL) {
		UNUSED_RETURN(flash_ctrl_disable_memory_mapped_mode());
		uint32_t status = flash_ctrl_page_write((uint8_t *)mem_writeBuffer, (uint32_t)target_page_address,
		                                        flash_ctrl_get_page_size());
		UNUSED_RETURN(flash_ctrl_enable_memory_mapped_mode());
		if (FLASH_CTRL_OK != status) {
			LLKERNEL_ERROR_LOG("%s: flash write 0x%.8x failed (status=%d)\n", __func__, (uint32_t)target_page_address,
			                   status);
			result = LLKERNEL_ERROR;
		}
		target_page_address = NULL;
		mem_writeBuffer_offset = 0;
	}

	return result;
}

// -----------------------------------------------------------------------------
// EOF
// -----------------------------------------------------------------------------
