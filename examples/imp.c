//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>

#include <gfxprim.h>
#include <gp_widgets.h>
#include <gp_dir_cache.h>
#include <gp_date_time.h>
#include <gp_file_size.h>

static gp_widget *table;

static int redraw_table(gp_widget_event *ev)
{
	(void) ev;

	gp_widget_redraw(table);

	return 0;
}

#include "imp.h"

static const gp_widget_table_header headers[] = {
	{.text = "File", .sortable = 1},
	{.text = "Size", .sortable = 1},
	{.text = "Modified", .sortable = 1},
};

static void sort(gp_widget *self, unsigned int col, int desc)
{
	int sort_type = 0;

	switch (col) {
	case 0:
		sort_type = GP_DIR_SORT_BY_NAME;
	break;
	case 1:
		sort_type = GP_DIR_SORT_BY_SIZE;
	break;
	case 2:
		sort_type = GP_DIR_SORT_BY_MTIME;
	break;
	}

	if (desc)
		sort_type |= GP_DIR_SORT_DESC;
	else
		sort_type |= GP_DIR_SORT_ASC;

	gp_dir_cache_sort(self->tbl->priv, sort_type);
}

static int find_next(gp_widget_table *tbl)
{
	gp_dir_cache *cache = tbl->priv;
	int show_hidden = hidden.chbox->val;
	char *str = filter.tbox->buf;
	size_t str_len = strlen(str);
	gp_dir_entry *entry;

	for (;;) {
		if (tbl->row_idx >= cache->used)
			return 0;

		entry = gp_dir_cache_get(cache, tbl->row_idx);

		if (str_len) {
			if (strstr(entry->name, str))
				return 1;
			else
				goto next;
		}

		if (show_hidden)
			return 1;

		if (entry->name[0] != '.' || entry->name[1] == '.')
			return 1;

next:
		tbl->row_idx++;
	}
}

int set_row(gp_widget *self, int op, unsigned int pos)
{
	gp_dir_cache *cache = self->tbl->priv;
	unsigned int i;

	switch (op) {
	case GP_TABLE_ROW_RESET:
		self->tbl->row_idx = 0;
		find_next(self->tbl);
	break;
	case GP_TABLE_ROW_ADVANCE:
		for (i = 0; i < pos; i++) {
			self->tbl->row_idx++;
			if (!find_next(self->tbl))
				return 0;
		}
	break;
	}

	if (self->tbl->row_idx < cache->used)
		return 1;

	return 0;
}

static const char *get_elem(gp_widget *self, unsigned int col)
{
	static char buf[100];
	gp_dir_cache *cache = self->tbl->priv;

	gp_dir_entry *ent = gp_dir_cache_get(cache, self->tbl->row_idx);

	if (!ent)
		return "";

	switch (col) {
	case 0:
		return ent->name;
	case 1:
		return gp_str_file_size(buf, sizeof(buf), ent->size);
	case 2:
		return gp_str_time_diff(buf, sizeof(buf), ent->mtime, time(NULL));
	}

	return "";
}

void table_on_event(gp_widget *self)
{
	gp_widget_table *tbl = self->tbl;
	//TODO: filtering - selected_row != cache entry
	gp_dir_entry *entry = gp_dir_cache_get(tbl->priv, tbl->selected_row);

	if (!entry->is_dir) {
		printf("%s\n", entry->name);
		return;
	}

	//TODO:!!!
}

int main(void)
{
	table = gp_widget_table_new(3, 25, headers, set_row, get_elem);
	table->align = GP_FILL(1, 1);
	table->tbl->col_fills[0] = 1;
	table->tbl->col_min_sizes[0] = 20;
	table->tbl->col_min_sizes[1] = 7;
	table->tbl->col_min_sizes[2] = 7;

	table->tbl->priv = gp_dir_cache_new(".");
	table->tbl->sort = sort;
	table->tbl->on_event = table_on_event;

	gp_widget_grid_put(&layout, 0, 1, table);

	gp_widgets_init(&layout);
	gp_widgets_main_loop(&layout, "IMP", NULL);

	return 0;
}
