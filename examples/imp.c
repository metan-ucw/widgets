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
#include <gp_string.h>
#include <gp_widgets.h>
#include <gp_dir_cache.h>
#include <gp_date_time.h>
#include <gp_file_size.h>

static gp_widget *table;
static gp_widget *hidden;
static gp_widget *filter;
static gp_widget *path;

static int redraw_table(gp_widget_event *ev)
{
	(void) ev;

	gp_widget_redraw(table);

	return 0;
}

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
	int show_hidden = hidden ? hidden->chbox->val : 0;
	char *str = filter ? filter->tbox->buf : "";
	size_t str_len = strlen(str);
	gp_dir_entry *entry;

	for (;;) {
		if (tbl->row_idx >= cache->used)
			return 0;

		entry = gp_dir_cache_get(cache, tbl->row_idx);

		if (str_len) {
			if (strstr(entry->name, str)) {
				gp_dir_cache_set_filter(cache, tbl->row_idx, 0);
				return 1;
			} else {
				goto next;
			}
		}

		if (show_hidden || (entry->name[0] != '.' || entry->name[1] == '.')) {
			gp_dir_cache_set_filter(cache, tbl->row_idx, 0);
			return 1;
		}
next:
		gp_dir_cache_set_filter(cache, tbl->row_idx, 1);
		tbl->row_idx++;
	}
}

static int notify_callback(gp_widget_poll *self)
{
	if (gp_dir_cache_inotify(self->priv))
		redraw_table(NULL);

	return 0;
}

static struct gp_widget_poll notify_poll;

static gp_dir_cache *load_dir_cache(void)
{
	gp_dir_cache *cache = gp_dir_cache_new(path->tbox->buf);

	if (cache->inotify_fd > 0) {
		notify_poll.events = POLLIN,
		notify_poll.fd = cache->inotify_fd;
		notify_poll.priv = cache;
		notify_poll.callback = notify_callback;
		gp_widgets_poll_add(&notify_poll);
	}

	return cache;
}

static void free_dir_cache(struct gp_dir_cache *self)
{
	if (self->inotify_fd > 0)
		gp_widgets_poll_rem(&notify_poll);

	gp_dir_cache_free(self);
}

int set_row(gp_widget *self, int op, unsigned int pos)
{
	gp_dir_cache *cache = self->tbl->priv;
	unsigned int i;

	if (!cache)
		cache = self->tbl->priv = load_dir_cache();

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

static void table_event(gp_widget_table *tbl)
{
	gp_dir_entry *entry = gp_dir_cache_get_filtered(tbl->priv, tbl->selected_row);

	if (!entry) {
		GP_BUG("Empty entry!");
		return;
	}

	if (!entry->is_dir) {
		printf("%s\n", entry->name);
		return;
	}

	char *dpath = gp_aprintf("%s/%s", path->tbox->buf, entry->name);
	char *dir = realpath(dpath, NULL);

	gp_widget_textbox_printf(path, "%s", dir);

	free(dpath);
	free(dir);

	free_dir_cache(table->tbl->priv);
	table->tbl->priv = load_dir_cache();
	gp_widget_textbox_clear(filter);
	gp_widget_redraw(table);
}

static int table_on_event(gp_widget_event *ev)
{
	switch (ev->type) {
	case GP_WIDGET_EVENT_ACTION:
		table_event(ev->self->tbl);
		return 0;
	case GP_WIDGET_EVENT_INPUT:
		if (ev->input_ev->type == GP_EV_KEY &&
		    ev->input_ev->val.val == GP_KEY_ESC) {
			gp_widget_textbox_clear(filter);
			gp_widget_redraw(ev->self);
			return 1;
		}

		return gp_widget_ops_event(filter, ev->ctx, ev->input_ev);
	default:
		return 0;
	}
}

int main(int argc, char *argv[])
{
	static void *uids;
	gp_widget *layout = gp_widget_layout_json("imp.json", &uids);

	hidden = gp_widget_by_uid(uids, "hidden", GP_WIDGET_CHECKBOX);
	filter = gp_widget_by_uid(uids, "filter", GP_WIDGET_TEXTBOX);

	if (hidden)
		hidden->on_event = redraw_table;

	if (filter)
		filter->on_event = redraw_table;

	path = gp_widget_by_uid(uids, "path", GP_WIDGET_TEXTBOX);

	gp_widget_textbox_printf(path, ".");

	table = gp_widget_table_new(3, 25, headers, set_row, get_elem);
	table->align = GP_FILL;
	table->tbl->col_fills[0] = 1;
	table->tbl->col_min_sizes[0] = 20;
	table->tbl->col_min_sizes[1] = 7;
	table->tbl->col_min_sizes[2] = 7;

	table->input_events = 1;

	table->tbl->priv = NULL;
	table->tbl->sort = sort;
	table->on_event = table_on_event;

	gp_widget_event_unmask(table, GP_WIDGET_EVENT_INPUT);

	gp_widget_grid_put(layout, 0, 1, table);

	gp_widgets_main_loop(layout, "Open File", NULL, argc, argv);

	return 0;
}
