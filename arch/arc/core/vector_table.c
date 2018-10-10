/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Populated exception vector table
 *
 * Vector table with exceptions filled in. The reset vector is the system entry
 * point, ie. the first instruction executed.
 *
 * The table is populated with all the system exception handlers. No exception
 * should not be triggered until the kernel is ready to handle them.
 *
 * We are using a C file instead of an assembly file (like the ARM vector table)
 * to work around an issue with the assembler where:
 *
 *   .word <function>
 *
 * statements would end up with the two half-words of the functions' addresses
 * swapped.
 */

#include <zephyr/types.h>
#include <toolchain.h>
#include "vector_table.h"

struct vector_table {
	u32_t reset;
	u32_t memory_error;
	u32_t instruction_error;
	u32_t ev_machine_check;
	u32_t ev_tlb_miss_i;
	u32_t ev_tlb_miss_d;
	u32_t ev_prot_v;
	u32_t ev_privilege_v;
	u32_t ev_swi;
	u32_t ev_trap;
	u32_t ev_extension;
	u32_t ev_div_zero;
	u32_t ev_dc_error;
	u32_t ev_maligned;
	u32_t unused_1;
	u32_t unused_2;
};

struct vector_table _VectorTable _GENERIC_SECTION(.exc_vector_table) = {
	(u32_t)__reset,
	(u32_t)__memory_error,
	(u32_t)__instruction_error,
	(u32_t)__ev_machine_check,
	(u32_t)__ev_tlb_miss_i,
	(u32_t)__ev_tlb_miss_d,
	(u32_t)__ev_prot_v,
	(u32_t)__ev_privilege_v,
	(u32_t)__ev_swi,
	(u32_t)__ev_trap,
	(u32_t)__ev_extension,
	(u32_t)__ev_div_zero,
	(u32_t)__ev_dc_error,
	(u32_t)__ev_maligned,
	0,
	0
};
