//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>

static unsigned int header_min_w(gp_widget_table *tbl,
                                 const gp_widget_render_cfg *cfg,
                                 unsigned int col)
{
	const char *text = tbl->headers[col].text;
	unsigned int text_size = gp_text_width(cfg->font_bold, text);

	if (tbl->headers[col].sortable)
		text_size += cfg->padd + gp_text_ascent(cfg->font);

	return text_size;
}

static unsigned int min_w(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	struct gp_widget_table *tbl = self->tbl;
	unsigned int i, sum_cols_w = 0;

	if (tbl->headers) {
		for (i = 0; i < tbl->cols; i++)
			tbl->cols_w[i] = header_min_w(tbl, cfg, i);
	}

	for (i = 0; i < tbl->cols; i++) {
		unsigned int col_size;
		col_size = gp_text_max_width(cfg->font, tbl->col_min_sizes[i]);
		tbl->cols_w[i] = GP_MAX(tbl->cols_w[i], col_size);
		sum_cols_w += tbl->cols_w[i];
	}

	return sum_cols_w + (2 * tbl->cols) * cfg->padd;
}

static unsigned int header_h(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	unsigned int text_a = gp_text_ascent(cfg->font);

	if (!self->tbl->headers)
		return 0;

	return text_a + 2 * cfg->padd;
}

static unsigned int row_h(const gp_widget_render_cfg *cfg)
{
	unsigned int text_a = gp_text_ascent(cfg->font);

	return text_a + cfg->padd;
}

static unsigned int min_h(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	unsigned int h = row_h(cfg) * self->tbl->min_rows;

	if (self->tbl->headers)
		h += header_h(self, cfg);

	return h;
}

static unsigned int display_rows(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	unsigned int text_a = gp_text_ascent(cfg->font);
	unsigned int header = header_h(self, cfg);

	return (self->h - header) / (text_a + cfg->padd);
}

static void distribute_size(gp_widget *self, const gp_widget_render_cfg *cfg, int new_wh)
{
	gp_widget_table *tbl = self->tbl;
	unsigned int i, sum_cols_w = 0, sum_fills = 0;

	(void)new_wh;

	for (i = 0; i < tbl->cols; i++) {
		sum_cols_w += tbl->cols_w[i];
		sum_fills += tbl->col_fills[i];
	}

	if (!sum_fills)
		return;

	unsigned int table_w = sum_cols_w + (2 * tbl->cols) * cfg->padd;
	unsigned int diff = self->w - table_w;

	for (i = 0; i < tbl->cols; i++)
		tbl->cols_w[i] += tbl->col_fills[i] * (diff/sum_fills);
}

static void header_render(gp_widget *self, gp_coord x, gp_coord y,
                          const gp_widget_render_cfg *cfg)
{
	gp_widget_table *tbl = self->tbl;
	const gp_widget_table_header *headers = tbl->headers;
	unsigned int text_a = gp_text_ascent(cfg->font);
	unsigned int cx = x + cfg->padd;
	unsigned int cy = y + cfg->padd;
	unsigned int i;

	for (i = 0; i < tbl->cols; i++) {
		if (tbl->headers[i].sortable) {
			gp_size symbol_size = text_a/1.5;
			gp_size sx = cx + tbl->cols_w[i] - cfg->padd;
			gp_size sy = cy + text_a/2;

			if (i == tbl->sorted_by_col) {
				if (tbl->sorted_desc)
					gp_triangle_down(cfg->buf, sx, sy, symbol_size, cfg->text_color);
				else
					gp_triangle_up(cfg->buf, sx, sy, symbol_size, cfg->text_color);
			} else {
				gp_triangle_updown(cfg->buf, sx, sy, symbol_size, cfg->text_color);
			}
		}

		gp_print(cfg->buf, cfg->font_bold, cx, cy,
			GP_ALIGN_RIGHT|GP_VALIGN_BELOW,
			cfg->text_color, cfg->bg_color, "%s", headers[i].text);

		cx += tbl->cols_w[i] + cfg->padd;

		if (i < tbl->cols - 1) {
			gp_vline_xyh(cfg->buf, cx, y+1,
			            text_a + 2 * cfg->padd-1, cfg->bg_color);
		}

		cx += cfg->padd;
	}

	cy += text_a + cfg->padd;

	gp_pixel color = self->selected ? cfg->sel_color : cfg->text_color;

	gp_hline_xyw(cfg->buf, x, cy, self->w, color);
}

static void align_text(gp_pixmap *buf, gp_widget_table *tbl,
                       const gp_widget_render_cfg *cfg,
		       unsigned int x, unsigned int y,
		       unsigned int col, gp_pixel bg, const char *str)
{
	gp_text_fit(buf, cfg->font, x, y, tbl->cols_w[col],
	           GP_ALIGN_RIGHT|GP_VALIGN_BELOW,
	           cfg->text_color, bg, str);
}

static void render(gp_widget *self, const gp_offset *offset,
                   const gp_widget_render_cfg *cfg, int flags)
{
	struct gp_widget_table *tbl = self->tbl;
	unsigned int text_a = gp_text_ascent(cfg->font);
	unsigned int x = self->x + offset->x;
	unsigned int y = self->y + offset->y;
	unsigned int w = self->w;
	unsigned int h = self->h;
	unsigned int cy = y + cfg->padd;
	unsigned int i, j;

	(void)flags;

	gp_pixel color = self->selected ? cfg->sel_color : cfg->text_color;
	gp_fill_rrect_xywh(cfg->buf, x, y, w, h, cfg->bg_color, cfg->fg_color, color);

	if (tbl->headers) {
		header_render(self, x, y, cfg);
		cy = y + header_h(self, cfg);
	}

	tbl->row(self, GP_TABLE_ROW_RESET, 0);
	tbl->row(self, GP_TABLE_ROW_ADVANCE, tbl->start_row);

	unsigned int cur_row = tbl->start_row;
	unsigned int rows = display_rows(self, cfg);

	unsigned int cx = x + cfg->padd;

	for (j = 0; j < tbl->cols-1; j++) {
		cx += tbl->cols_w[j] + cfg->padd;
		gp_vline_xyy(cfg->buf, cx, cy+1, self->y + self->h - 2, cfg->bg_color);
		cx += cfg->padd;
	}

	cy += cfg->padd/2;
	for (i = 0; i < rows; i++) {
		cx = x + cfg->padd;
		gp_pixel bg_col = cfg->fg_color;

		if (tbl->row_selected && cur_row == tbl->selected_row) {
			bg_col = self->selected ? cfg->sel_color : cfg->bg_color;

			gp_fill_rect_xywh(cfg->buf, x+1, cy - cfg->padd/2+1,
					self->w - 2,
					text_a + cfg->padd-1, bg_col);
		}

		for (j = 0; j < tbl->cols; j++) {
			const char *str = tbl->get(self, j);

			align_text(cfg->buf, tbl, cfg, cx, cy, j, bg_col, str);

			cx += tbl->cols_w[j] + cfg->padd;

		//	if (j < tbl->cols - 1) {
		//		gp_vline_xyh(cfg->buf, cx, cy,
		//			    text_a, cfg->text_color);
		//	}

			cx += cfg->padd;
		}

		cy += text_a + cfg->padd;

		tbl->row(self, GP_TABLE_ROW_ADVANCE, 1);
		cur_row++;

		gp_hline_xyw(cfg->buf, x+1, cy - cfg->padd/2, w-2, cfg->bg_color);
	}

	while (tbl->row(self, GP_TABLE_ROW_ADVANCE, 1))
		cur_row++;

	tbl->last_max_row = cur_row;
}

static void fix_selected_row(gp_widget_table *tbl)
{
	if (tbl->selected_row >= tbl->last_max_row)
		tbl->selected_row = tbl->last_max_row - 1;
}

static int move_down(gp_widget *self, const gp_widget_render_cfg *cfg,
                     unsigned int rows)
{
	gp_widget_table *tbl = self->tbl;

	if (!tbl->row_selected) {
		tbl->row_selected = 1;
		tbl->selected_row = tbl->start_row;

		fix_selected_row(tbl);

		goto redraw;
	}

	if (tbl->selected_row < tbl->last_max_row) {
		tbl->selected_row += rows;

		fix_selected_row(tbl);

		goto redraw;
	}

	return 0;

redraw:
	rows = display_rows(self, cfg);

	if (tbl->selected_row > tbl->start_row + rows)
		tbl->start_row = tbl->selected_row - rows + 1;

	gp_widget_redraw(self);
	return 1;
}

static int move_up(gp_widget *self, const gp_widget_render_cfg *cfg, unsigned int rows)
{
	gp_widget_table *tbl = self->tbl;

	if (!tbl->row_selected) {
		tbl->row_selected = 1;
		tbl->selected_row = tbl->start_row + display_rows(self, cfg) - 1;

		goto redraw;
	}

	if (tbl->selected_row > 0) {
		if (tbl->selected_row > rows)
			tbl->selected_row -= rows;
		else
			tbl->selected_row = 0;

		goto redraw;
	}

	return 0;

redraw:
	if (tbl->selected_row < tbl->start_row)
		tbl->start_row = tbl->selected_row;

	gp_widget_redraw(self);
	return 1;
}

static int header_click(gp_widget *self, const gp_widget_render_cfg *cfg, unsigned int x)
{
	gp_widget_table *tbl = self->tbl;
	unsigned int i, cx = self->x;

	//TODO: inverval division?
	for (i = 0; i < tbl->cols-1; i++) {
		cx += tbl->cols_w[i] + 2 * cfg->padd;

		if (x <= cx)
			break;
	}

	if (!tbl->headers[i].sortable)
		return 0;

	if (!tbl->sort) {
		GP_BUG("No sort fuction defined!");
		return 1;
	}

	if (tbl->sorted_by_col == i)
		tbl->sorted_desc = !tbl->sorted_desc;
	else
		tbl->sorted_by_col = i;

	tbl->sort(self, tbl->sorted_by_col, tbl->sorted_desc);

	gp_widget_redraw(self);
	return 1;
}

static int row_click(gp_widget *self, const gp_widget_render_cfg *cfg, gp_event *ev)
{
	gp_widget_table *tbl = self->tbl;
	unsigned int row = ev->cursor_y - self->y - header_h(self, cfg);

	row /= row_h(cfg) + tbl->start_row;
	tbl->selected_row = row;

	if (!tbl->row_selected)
		tbl->row_selected = 1;

	gp_widget_redraw(self);
	return 1;
}

static int click(gp_widget *self, const gp_widget_render_cfg *cfg, gp_event *ev)
{
	if (ev->cursor_y <= self->y + header_h(self, cfg))
		return header_click(self, cfg, ev->cursor_x);

	return row_click(self, cfg, ev);
}

static int enter(gp_widget *self)
{
	gp_widget_table *tbl = self->tbl;

	if (!tbl->row_selected)
		return 0;

	gp_widget_send_event(self, GP_WIDGET_EVENT_ACTION);

	return 1;
}

static int event(gp_widget *self, const gp_widget_render_cfg *cfg, gp_event *ev)
{
	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code == GP_EV_KEY_UP)
			return 0;

		switch (ev->val.val) {
		case GP_KEY_DOWN:
			if (gp_event_get_key(ev, GP_KEY_LEFT_SHIFT))
				return move_down(self, cfg, 10);

			return move_down(self, cfg, 1);
		break;
		case GP_KEY_UP:
			if (gp_event_get_key(ev, GP_KEY_LEFT_SHIFT))
				return move_up(self, cfg, 10);

			return move_up(self, cfg, 1);
		break;
		//TODO: Better page up/down
		case GP_KEY_PAGE_UP:
			return move_up(self, cfg, 10);
		case GP_KEY_PAGE_DOWN:
			return move_down(self, cfg, 10);
		case GP_BTN_LEFT:
			return click(self, cfg, ev);
		case GP_KEY_ENTER:
			return enter(self);
		}
	}

	return 0;
}

struct gp_widget_ops gp_widget_table_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.distribute_size = distribute_size,
	.render = render,
	.event = event,
	.id = "Table",
};

struct gp_widget *gp_widget_table_new(unsigned int cols, unsigned int min_rows,
                                    const gp_widget_table_header *headers,
				    int (*row)(struct gp_widget *self,
                                               int op, unsigned int pos),
				    const char *(get)(struct gp_widget *self,
					              unsigned int col))
{
	gp_widget *ret;
	size_t size = sizeof(struct gp_widget_table);

	size += 2 * cols * sizeof(unsigned int);
	size += cols;

	ret = gp_widget_new(GP_WIDGET_TABLE, size);
	if (!ret)
		return NULL;

	ret->tbl->cols = cols;
	ret->tbl->min_rows = min_rows;
	ret->tbl->start_row = 0;
	ret->tbl->cols_w = (void*)ret->tbl->buf;
	ret->tbl->col_min_sizes = (void*)(ret->tbl->buf + cols * sizeof(unsigned int));
	ret->tbl->col_fills = (void*)(ret->tbl->buf + 2 * cols * sizeof(unsigned int));
	ret->tbl->headers = headers;

	ret->tbl->get = get;
	ret->tbl->row = row;

	return ret;
}

void gp_widget_table_refresh(gp_widget *self)
{
	gp_widget_redraw(self);
}
