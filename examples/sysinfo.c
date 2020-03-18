//SPDX-License-Identifier: LGPL-2.0-or-later
/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#include <gfxprim.h>
#include <gp_widgets.h>

static gp_widget *ram_usage;
static gp_widget *swap_usage;

void init_sysinfo(gp_widget *grid)
{
	static struct utsname buf;
	static char hbuf[32];

	uname(&buf);
	gethostname(hbuf, sizeof(hbuf));

	gp_widget *label;

	label = gp_widget_label_new(buf.sysname, 0, 0);
	gp_widget_grid_put(grid, 1, 0, label);
	label = gp_widget_label_new(buf.release, 0, 0);
	gp_widget_grid_put(grid, 1, 1, label);
	label = gp_widget_label_new(buf.machine, 0, 0);
	gp_widget_grid_put(grid, 1, 2, label);
	label = gp_widget_label_new(hbuf, 0, 0);
	gp_widget_grid_put(grid, 1, 3, label);
}

static void update_mem_usage(void)
{
	FILE *f;
	char buf[129];
	unsigned long val = 0;
	unsigned long mem_total = 0, mem_free = 0;
	unsigned long swap_total = 0, swap_free = 0;

	f = fopen("/proc/meminfo", "r");
	if (!f)
		return;

	while (!feof(f)) {
		if (fscanf(f, "%128s %lu%*[^\n]\n", buf, &val) != 2)
			return;

		if (!strcmp(buf, "MemTotal:"))
			mem_total = val;

		if (!strcmp(buf, "MemFree:"))
			mem_free = val;

		if (!strcmp(buf, "SwapTotal:"))
			swap_total = val;

		if (!strcmp(buf, "SwapFree:"))
			swap_free = val;
	}

	gp_widget_pbar_set(ram_usage, 100.00 * (mem_total - mem_free) / mem_total);
	gp_widget_pbar_set(swap_usage, 100.00 * (swap_total - swap_free) / swap_total);
}

struct cpu_stat {
        unsigned long long user;
        unsigned long long nice;
        unsigned long long system;
        unsigned long long idle;
        unsigned long long iowait;
        unsigned long long irq;
        unsigned long long softirq;

        unsigned long long sum;

	unsigned int load;
};

static struct cpu_stat *cpu_stats;

static unsigned int count_cpus(void)
{
	static unsigned int ncpus;

	if (!ncpus)
		ncpus = sysconf(_SC_NPROCESSORS_ONLN) + 1;

	return ncpus;
}

static void read_cpu_proc(struct cpu_stat *s)
{
	unsigned int i, ncpus = count_cpus();

	FILE *f = fopen("/proc/stat", "r");
	if (!f)
		return;

	for (i = 0; i < ncpus; i++) {
		struct cpu_stat *c = &s[i];
		unsigned long long prev_sum = c->sum;
		unsigned long long prev_idle = c->idle;
		int ret;

		ret = fscanf(f, "%*s%llu%llu%llu%llu%llu%llu%llu%*u%*u%*u\n",
		             &c->user, &c->nice, &c->system, &c->idle, &c->iowait,
		             &c->irq, &c->softirq);

		if (!ret)
			continue;

		c->sum = c->user + c->nice + c->system + c->idle +
			 c->iowait + c->irq + c->softirq;

		unsigned long long sum_diff = c->sum - prev_sum;
		unsigned long long idle_diff = c->idle - prev_idle;

		/* Low pass */
		static const float lpass_coef = 0.6;
		c->load = lpass_coef * c->load;
		c->load += (1-lpass_coef) * (1000000.00 * (sum_diff - idle_diff) / sum_diff);
	}

	fclose(f);
}

static gp_widget *bars;

gp_widget *init_cpu_bars(void)
{
	unsigned int cpus = count_cpus();
	unsigned int i;

	cpu_stats = malloc(cpus * sizeof(struct cpu_stat));
	if (!cpu_stats)
		return NULL;

	read_cpu_proc(cpu_stats);

	bars = gp_widget_grid_new(2, cpus);

	char buf[10] = "cpu";

	for (i = 0; i < cpus; i++) {
		cpu_stats[i].load = 0;
		if (i != 0)
			sprintf(buf, "cpu%i", i);
		gp_widget_grid_put(bars, 1, i, gp_widget_pbar_new(0, 100, GP_WIDGET_PBAR_PERCENTS));
		gp_widget_grid_put(bars, 0, i, gp_widget_label_new(buf, 0, 0));
	}

	return bars;
}

static void update_cpu_usage(void)
{
	unsigned int i, cpus = count_cpus();

	read_cpu_proc(cpu_stats);

	for (i = 0; i < cpus; i++) {
		gp_widget *bar = gp_widget_grid_get(bars, 1, i);
		gp_widget_pbar_set(bar, cpu_stats[i].load / 10000.00);
	}
}

static uint32_t timer_callback(void *unused)
{
	(void) unused;
	update_mem_usage();
	update_cpu_usage();

	return 500;
}

static gp_widget_timer tmr = {
	.callback = timer_callback,
};

static void start_timer(void)
{
	gp_widgets_timer_add(&tmr, 1000);
}

int main(int argc, char *argv[])
{
	static void *uids;

	gp_widget *layout = gp_widget_layout_json("sysinfo.json", &uids);
	if (!layout)
		return 0;

	gp_widget *grid = gp_widget_by_uid(uids, "sysinfo", GP_WIDGET_GRID);

	ram_usage = gp_widget_by_uid(uids, "ram_usage", GP_WIDGET_PROGRESSBAR);
	swap_usage = gp_widget_by_uid(uids, "swap_usage", GP_WIDGET_PROGRESSBAR);

	init_sysinfo(grid);
	update_mem_usage();

	gp_widgets_main_loop(layout, "System Information", start_timer, argc, argv);

	return 0;
}
