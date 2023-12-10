//SPDX-License-Identifier: GPL-2.0-or-later

/*

    Copyright (C) 2023 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <sysinfo/cpu_stats.h>
#include <sysinfo/cpu_arch.h>

static struct cpu_stats *cpu_stats;
static gp_widget *cpu_temp;
static gp_widget *cpu_temp_graph;
static gp_widget *cpu_load;

gp_app_info app_info = {
	.name = "gpcpuinfo",
	.desc = "CPU information",
	.version = "1.0",
	.license = "GPL-2.0-or-later",
	.url = "http://github.com/gfxprim/gpcpuinfo",
	.authors = (gp_app_info_author []) {
		{.name = "Cyril Hrubis", .email = "metan@ucw.cz", .years = "2023"},
		{}
	}
};

static void update(void)
{
	cpu_stats_update(cpu_stats);

	if (cpu_load)
		gp_widget_graph_point_add(cpu_load, gp_time_stamp(), cpu_stats_load_perc(cpu_stats, 0));

	if (cpu_stats_temp_supported(cpu_stats)) {
		float cpu_t;

		if (cpu_temp || cpu_temp_graph)
			cpu_t = cpu_stats_temp(cpu_stats);

		if (cpu_temp)
			gp_widget_label_printf(cpu_temp, "%3.1f\u00b0C", cpu_t);

		if (cpu_temp_graph)
			gp_widget_graph_point_add(cpu_temp_graph, gp_time_stamp(), cpu_t);
	}
}

static uint32_t refresh_callback(gp_timer *self)
{
	(void)self;

	update();

	return self->period;
}

static gp_timer refresh_tmr = {
	.period = 100,
	.callback = refresh_callback,
	.id = "Refresh timer"
};

static void set_info(gp_htable *uids)
{
	struct cpu_arch arch;
	gp_widget *widget;

	cpu_arch_get(&arch);

	widget = gp_widget_by_uid(uids, "cpu_vendor", GP_WIDGET_LABEL);
	if (widget)
		gp_widget_label_set(widget, cpu_vendor_name(arch.vendor));

	widget = gp_widget_by_uid(uids, "cpu_uarch", GP_WIDGET_LABEL);
	if (widget)
		gp_widget_label_set(widget, cpu_uarch_name(arch.uarch));

	widget = gp_widget_by_uid(uids, "cpu_processors", GP_WIDGET_LABEL);
	if (widget)
		gp_widget_label_printf(widget, "%i", arch.processors);

	widget = gp_widget_by_uid(uids, "cpu_cores", GP_WIDGET_LABEL);
	if (widget)
		gp_widget_label_printf(widget, "%i", arch.cores);

	widget = gp_widget_by_uid(uids, "cpu_name", GP_WIDGET_LABEL);
	if (widget)
		gp_widget_label_set(widget, arch.model_name);
}

int main(int argc, char *argv[])
{
	gp_htable *uids;
	gp_widget *layout = gp_app_layout_load("gpcpuinfo", &uids);

	cpu_stats = cpu_stats_create();

	cpu_load = gp_widget_by_uid(uids, "cpu_load", GP_WIDGET_GRAPH);
	if (cpu_stats)
		gp_widget_graph_yrange_set(cpu_load, 0, 100);

	cpu_temp_graph = gp_widget_by_uid(uids, "cpu_temp_graph", GP_WIDGET_GRAPH);
	if (cpu_temp_graph)
		gp_widget_graph_yrange_set(cpu_temp_graph, 0, 120);

	cpu_temp = gp_widget_by_uid(uids, "cpu_temp", GP_WIDGET_LABEL);

	set_info(uids);

	gp_htable_free(uids);
	gp_widgets_timer_ins(&refresh_tmr);

	gp_widgets_main_loop(layout, NULL, argc, argv);

	return 0;
}
