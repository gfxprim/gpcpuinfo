//SPDX-License-Identifier: LGPL-2.1-or-later

/*

   Copyright (C) 2023 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef CPU_ARCH_H
#define CPU_ARCH_H

#include "cpu_vendor.h"
#include "cpu_uarch.h"

struct cpu_arch {
	enum cpu_vendor vendor;
	enum cpu_uarch uarch;

	int processors;
	int cores;

	char model_name[128];
};

void cpu_arch_get(struct cpu_arch *arch);

void cpu_arch_print(struct cpu_arch *arch);

#endif /* CPU_ARCH_H */
