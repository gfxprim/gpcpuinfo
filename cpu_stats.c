//SPDX-License-Identifier: LGPL-2.1-or-later

/*

   Copyright (C) 2007-2023 Cyril Hrubis <metan@ucw.cz>

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#include "cpu_stats.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define STATS "/proc/stat"

static void load_proc(struct cpu_stat *data, unsigned int cnt)
{
	FILE *f;

	f = fopen(STATS, "r");

	if (!f)
		return;

	unsigned int i;

	for (i = 0; i < cnt; i++) {
		unsigned long long user, nice, syst, idle, iowt, irq, sirq, sum;
		int ret;

		ret = fscanf(f, "%*s%llu%llu%llu%llu%llu%llu%llu%*u%*u%*u\n",
		             &user, &nice, &syst, &idle, &iowt, &irq, &sirq);

		if (ret <= 0)
			return;

		sum = user + nice + syst + idle + iowt + irq + sirq;

		data[i].user_diff = user - data[i].user;
		data[i].nice_diff = nice - data[i].nice;
		data[i].syst_diff = syst - data[i].syst;
		data[i].idle_diff = idle - data[i].idle;
		data[i].iowt_diff = iowt - data[i].iowt;
		data[i].irq_diff  = irq  - data[i].irq;
		data[i].sirq_diff = sirq - data[i].sirq;
		data[i].sum_diff  = sum  - data[i].sum;

		data[i].user = user;
		data[i].nice = nice;
		data[i].syst = syst;
		data[i].idle = idle;
		data[i].iowt = iowt;
		data[i].irq  = irq;
		data[i].sirq = sirq;
		data[i].sum  = sum;
	}

	fclose(f);
}

static void open_hwmon(struct cpu_temp *temp)
{
	DIR *d;
	struct dirent *ent;
	int i;

	d = opendir("/sys/class/hwmon");

	if (!d)
		return;

	while ((ent = readdir(d))) {
		char path[2048];
		char name[256];
		ssize_t size;
		int fd;

		snprintf(path, sizeof(path), "/sys/class/hwmon/%s/name", ent->d_name);

		fd = open(path, O_RDONLY);
		if (fd < 0)
			continue;

		size = read(fd, name, sizeof(name));
		close(fd);

		if (size < 0)
			continue;

		if (strncmp(name, "k10temp", size-1) &&
		    strncmp(name, "k8temp", size-1) &&
		    strncmp(name, "coretemp", size-1) &&
		    strncmp(name, "cpu_thermal", size-1)) {
			continue;
		}

		for (i = 0; i < 10; i++) {
			snprintf(path, sizeof(path),
			         "/sys/class/hwmon/%s/temp%i_input",
			         ent->d_name, i);

			fd = open(path, O_RDONLY);
			if (fd >= 0) {
				size_t size = MIN(sizeof(*temp->driver)-1, size-1);

				memset(temp->driver, 0, sizeof(*temp->driver));
				strncpy(temp->driver, name, size);

				temp->fd = fd;
				temp->temp = 0;

				return;
			}
		}
	}

	closedir(d);

	temp->fd = -1;
	temp->temp = INT_MIN;
	temp->driver[0] = 0;
}

static void close_hwmon(struct cpu_temp *temp)
{
	if (temp->fd >= 0)
		close(temp->fd);
}

static void read_hwmon(struct cpu_temp *temp)
{
	char buf[32];
	ssize_t ret;

	ret = pread(temp->fd, buf, sizeof(buf)-1, 0);

	if (ret <= 0)
		return;

	buf[ret] = 0;

	temp->temp = atoi(buf);
}

void cpu_stats_update(struct cpu_stats *self)
{
	load_proc(self->stats, self->nr_cpu + 1);

	if (self->temp.fd >= 0)
		read_hwmon(&self->temp);
}

void cpu_stats_destroy(struct cpu_stats *self)
{
	close_hwmon(&self->temp);

	free(self);
}

static unsigned int count_cpus(void)
{
	FILE *f;

	f = fopen(STATS, "r");

	if (!f)
		return 0;

	int flag = 1;
	unsigned int cnt = 0;

	for (;;) {
		switch (fgetc(f)) {
		case EOF:
			fclose(f);
			return cnt;
		case 'c':
			if (flag) {
				if (fgetc(f) == 'p' && fgetc(f) == 'u')
					cnt++;
				flag = 0;
			}
		break;
		case '\n':
			flag = 1;
		break;
		default:
			flag = 0;
		}
	}

	fclose(f);
	return cnt;
}

struct cpu_stats *cpu_stats_create(void)
{
	struct cpu_stats *stats;
	unsigned int cpus = count_cpus();

	stats = malloc(sizeof(struct cpu_stats) +
		       cpus * sizeof(struct cpu_stat));

	if (!stats)
		return NULL;

	stats->nr_cpu = cpus - 1;

	load_proc(stats->stats, cpus);

	open_hwmon(&stats->temp);
	read_hwmon(&stats->temp);

	return stats;
}
