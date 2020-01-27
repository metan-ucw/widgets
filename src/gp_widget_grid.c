//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <ctype.h>
#include <json-c/json.h>

#include <core/gp_debug.h>
#include <core/gp_common.h>
#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_json.h>

static void init(gp_widget *self)
{
	struct gp_widget_grid *grid = self->grid;
	unsigned int i;

	for (i = 0; i < grid->cols * grid->rows; i++)
		gp_widget_init(grid->widgets[i], self);
}

static gp_widget *widget_grid_grid_get(struct gp_widget_grid *grid,
                                       unsigned int col, unsigned int row)
{
	return grid->widgets[row * grid->cols + col];
}

static gp_widget *widget_grid_get(gp_widget *self,
                                  unsigned int col, unsigned int row)
{
	return widget_grid_grid_get(self->grid, col, row);
}

static gp_widget *widget_grid_selected(gp_widget *self)
{
	return widget_grid_get(self,
	                       self->grid->selected_col,
	                       self->grid->selected_row);
}

static struct gp_widget *widget_grid_put(gp_widget *self, gp_widget *new,
		                         unsigned int x, unsigned int y)
{
	gp_widget *ret = self->grid->widgets[y * self->grid->cols + x];

	self->grid->widgets[y * self->grid->cols + x] = new;

	if (new)
		new->parent = self;

	return ret;
}

static unsigned int padd_size(int padd)
{
	return cfg->padd * padd;
}

static unsigned int min_w_uniform(gp_widget *self)
{
	struct gp_widget_grid *grid = self->grid;
	unsigned int x, sum_min_w = padd_size(grid->col_padds[0]);
	unsigned int max_cols_w = 0;

	for (x = 0; x < grid->cols; x++) {
		unsigned int y;
		for (y = 0; y < grid->rows; y++) {
			unsigned int min_w;
			min_w = gp_widget_min_w(widget_grid_get(self, x, y));
			max_cols_w = GP_MAX(max_cols_w, min_w);
		}

		sum_min_w += padd_size(grid->col_padds[x+1]);
	}

	return sum_min_w + grid->cols * max_cols_w;
}

static unsigned int min_w_(gp_widget *self)
{
	struct gp_widget_grid *grid = self->grid;
	unsigned int x, sum_min_w = padd_size(grid->col_padds[0]);

	for (x = 0; x < grid->cols; x++) {
		unsigned int y, max_col_w = 0;
		for (y = 0; y < grid->rows; y++) {
			unsigned int min_w;
			min_w = gp_widget_min_w(widget_grid_get(self, x, y));
			max_col_w = GP_MAX(max_col_w, min_w);
		}

		sum_min_w += max_col_w;
		sum_min_w += padd_size(grid->col_padds[x+1]);
	}

	return sum_min_w;
}

static unsigned int min_w(gp_widget *self)
{
	if (self->grid->uniform)
		return min_w_uniform(self);

	return min_w_(self);
}

static unsigned int min_h_uniform(gp_widget *self)
{
	struct gp_widget_grid *grid = self->grid;
	unsigned int y, sum_min_h = padd_size(grid->row_padds[0]);
	unsigned int max_rows_h = 0;

	for (y = 0; y < grid->rows; y++) {
		unsigned int x;
		for (x = 0; x < grid->cols; x++) {
			unsigned int min_h;
			min_h = gp_widget_min_h(widget_grid_get(self, x, y));
			max_rows_h = GP_MAX(max_rows_h, min_h);
		}

		sum_min_h += padd_size(grid->row_padds[y+1]);
	}

	return sum_min_h + grid->rows * max_rows_h;
}

static unsigned int min_h_(gp_widget *self)
{
	struct gp_widget_grid *grid = self->grid;
	unsigned int y, sum_min_h = padd_size(grid->row_padds[0]);

	for (y = 0; y < grid->rows; y++) {
		unsigned int x, max_row_h = 0;
		for (x = 0; x < grid->cols; x++) {
			unsigned int min_h;
			min_h = gp_widget_min_h(widget_grid_get(self, x, y));
			max_row_h = GP_MAX(max_row_h, min_h);
		}

		sum_min_h += max_row_h;
		sum_min_h += padd_size(grid->row_padds[y+1]);
	}

	return sum_min_h;
}

static unsigned int min_h(gp_widget *self)
{
	if (self->grid->uniform)
		return min_h_uniform(self);

	return min_h_(self);
}

static void compute_cols_rows_min_wh(struct gp_widget_grid *grid)
{
	unsigned int x, y;

	for (y = 0; y < grid->rows; y++)
		grid->rows_h[y] = 0;

	for (x = 0; x < grid->cols; x++)
		grid->cols_w[x] = 0;

	for (y = 0; y < grid->rows; y++) {
		for (x = 0; x < grid->cols; x++) {
			struct gp_widget *widget = widget_grid_grid_get(grid, x, y);

			grid->cols_w[x] = GP_MAX(grid->cols_w[x], gp_widget_min_w(widget));
			grid->rows_h[y] = GP_MAX(grid->rows_h[y], gp_widget_min_h(widget));
		}
	}
}

static void compute_cols_rows_min_uniform_wh(struct gp_widget_grid *grid)
{
	unsigned int x, y;
	unsigned int min_cols_w = 0, min_rows_h = 0;

	for (y = 0; y < grid->rows; y++) {
		for (x = 0; x < grid->cols; x++) {
			struct gp_widget *widget = widget_grid_grid_get(grid, x, y);

			min_cols_w = GP_MAX(min_cols_w, gp_widget_min_w(widget));
			min_rows_h = GP_MAX(min_rows_h, gp_widget_min_h(widget));
		}
	}

	for (x = 0; x < grid->cols; x++)
		grid->cols_w[x] = min_cols_w;

	for (y = 0; y < grid->rows; y++)
		grid->rows_h[y] = min_rows_h;
}

static void distribute_size(gp_widget *self, int new_wh)
{
	struct gp_widget_grid *grid = self->grid;
	unsigned int x, y;

	if (grid->uniform)
		compute_cols_rows_min_uniform_wh(grid);
	else
		compute_cols_rows_min_wh(grid);

	unsigned int sum_row_fills = 0;
	unsigned int sum_col_fills = 0;

	/* cell fills */
	for (y = 0; y < grid->rows; y++)
		sum_row_fills += grid->row_fills[y];

	for (x = 0; x < grid->cols; x++)
		sum_col_fills += grid->col_fills[x];

	/* padding fills */
	for (y = 0; y <= grid->rows; y++)
		sum_row_fills += grid->row_pfills[y];

	for (x = 0; x <= grid->cols; x++)
		sum_col_fills += grid->col_pfills[x];

	/* size to be distributed */
	unsigned int dx = self->w - self->min_w;
	unsigned int dy = self->h - self->min_h;

	if (sum_col_fills) {
		for (x = 0; x < grid->cols; x++)
			grid->cols_w[x] += dx * grid->col_fills[x] / sum_col_fills;
	}

	if (sum_row_fills) {
		for (y = 0; y < grid->rows; y++)
			grid->rows_h[y] += dy * grid->row_fills[y] / sum_row_fills;
	}

	/* Compute colum/row offsets */
	unsigned int cur_x = self->x + padd_size(grid->col_padds[0]);
	if (sum_col_fills)
		cur_x += dx * grid->col_pfills[0] / sum_col_fills;


	for (x = 0; x < grid->cols; x++) {
		grid->cols_off[x] = cur_x;
		cur_x += grid->cols_w[x] + padd_size(grid->col_padds[x+1]);
		if (sum_col_fills)
			cur_x += dx * grid->col_pfills[x+1] / sum_col_fills;
	}

	unsigned int cur_y = self->y + padd_size(grid->row_padds[0]);
	if (sum_row_fills)
		cur_y += dy * grid->row_pfills[0] / sum_row_fills;

	for (y = 0; y < grid->rows; y++) {
		grid->rows_off[y] = cur_y;
		cur_y += grid->rows_h[y] + padd_size(grid->row_padds[y+1]);
		if (sum_row_fills)
			cur_y += dy * grid->row_pfills[y+1] / sum_row_fills;
	}

	/* Place the widgets */
	for (y = 0; y < grid->rows; y++) {
		for (x = 0; x < grid->cols; x++) {
			struct gp_widget *widget = widget_grid_get(self, x, y);

			if (widget) {
				widget->x = grid->cols_off[x];
				widget->y = grid->rows_off[y];
				widget->w = grid->cols_w[x];
				widget->h = grid->rows_h[y];

				gp_widget_ops_distribute_size(widget,
				                           widget->x, widget->y,
				                           grid->cols_w[x],
				                           grid->rows_h[y],
				                           new_wh);
			}
		}
	}
}

static void fill_padding(gp_widget *self,
                         struct gp_widget_render *render, int flags)
{
	struct gp_widget_grid *grid = self->grid;

	if ((self->no_redraw && flags != 1))
		return;

	GP_DEBUG(3, "Filling grid %p padding", self);

	unsigned int y, cur_y = self->y;
	for (y = 0; y < grid->rows; y++) {
		gp_fill_rect_xyxy(render->buf, self->x, cur_y,
		                  self->x + self->w - 1, grid->rows_off[y] - 1,
				  cfg->bg_color);

		if (y < grid->rows)
			cur_y = grid->rows_h[y] + grid->rows_off[y];
	}

	gp_fill_rect_xyxy(render->buf, self->x, cur_y,
	                  self->x + self->w - 1, self->y + self->h - 1,
			  cfg->bg_color);

	unsigned int x, cur_x = self->x;
	for (x = 0; x < grid->cols; x++) {
		gp_fill_rect_xyxy(render->buf, cur_x, self->y,
		                  grid->cols_off[x] - 1, self->y + self->h - 1,
				  cfg->bg_color);

		if (x < grid->cols)
			cur_x = grid->cols_off[x] + grid->cols_w[x];
	}

	gp_fill_rect_xyxy(render->buf, cur_x, self->y,
		          self->x + self->w - 1, self->y + self->h - 1,
	                  cfg->bg_color);

	if (grid->frame) {
		gp_rrect_xywh(render->buf, self->x, self->y,
			      self->w, self->h, cfg->text_color);
	}
}

static void render(gp_widget *self,
                   struct gp_widget_render *render, int flags)
{
	struct gp_widget_grid *grid = self->grid;
	unsigned int x, y, cur_x, cur_y;

	fill_padding(self, render, flags);

	for (y = 0; y < grid->rows; y++) {
		cur_y = grid->rows_off[y];

		for (x = 0; x < grid->cols; x++) {
			cur_x = grid->cols_off[x];

			struct gp_widget *widget = widget_grid_get(self, x, y);

			if (!widget) {
				gp_fill_rect_xywh(render->buf, cur_x, cur_y,
						  grid->cols_w[x], grid->rows_h[y],
						  cfg->bg_color);
				continue;
			}

			if (widget->no_redraw && widget->no_redraw_child && flags != 1)
				continue;

			GP_DEBUG(3, "rendering widget %s [%u:%u]",
			         gp_widget_type_id(widget), x, y);

			/* Fill unused space between widget */
			unsigned int bw = widget->x - grid->cols_off[x];
			unsigned int aw = grid->cols_w[x] - bw - widget->w;
			gp_pixel bg = cfg->bg_color;

			if (bw) {
				gp_fill_rect_xywh(render->buf, cur_x, cur_y, bw,
				                  grid->rows_h[y], bg);
			}

			if (aw) {
				gp_fill_rect_xywh(render->buf, widget->x + widget->w,
						cur_y, aw, grid->rows_h[y], bg);
			}

			unsigned int bh = widget->y - cur_y;
			unsigned int ah = grid->rows_h[y] - bh - widget->h;

			if (bh) {
				gp_fill_rect_xywh(render->buf, widget->x, cur_y, widget->w,
				                  bh, cfg->bg_color);
			}

			if (ah) {
				gp_fill_rect_xywh(render->buf, widget->x, widget->y + widget->h,
						widget->w, ah, cfg->bg_color);
			}

			gp_widget_ops_render(widget, render, flags);
		}
	}
/*
	gp_pixel col = random();

	unsigned int sx = grid->cols_off[0];
	unsigned int ex = grid->cols_off[grid->cols-1] + grid->cols_w[grid->cols-1];

	for (y = 0; y < grid->rows; y++) {
		gp_hline_xxy(render->buf, sx, ex, grid->rows_off[y], col);
		gp_hline_xxy(render->buf, sx, ex, grid->rows_off[y] + grid->rows_h[y], col);
	}

	unsigned int sy = grid->rows_off[0];
	unsigned int ey = grid->rows_off[grid->rows-1] + grid->rows_h[grid->rows-1];

	for (x = 0; x < grid->cols; x++) {
		gp_vline_xyy(render->buf, grid->cols_off[x], sy, ey, col);
		gp_vline_xyy(render->buf, grid->cols_off[x] + grid->cols_w[x], sy, ey, col);
	}
*/
}

static int event(gp_widget *self, gp_event *ev)
{
	struct gp_widget *w = widget_grid_selected(self);

	GP_DEBUG(3, "event widget %p (%s)", w, gp_widget_type_id(w));

	return gp_widget_ops_event(w, ev);
}

static int try_select(gp_widget *self, unsigned int col, unsigned int row, int sel)
{
	gp_widget *w = widget_grid_get(self, col, row);

	GP_DEBUG(4, "Trying to select widget %p (%s) %ux%u",
		 w, gp_widget_type_id(w), col, row);

	if (!gp_widget_ops_render_select(widget_grid_get(self, col, row), sel))
		return 0;

	gp_widget_ops_render_select(widget_grid_selected(self), GP_SELECT_OUT);

	self->grid->selected_col = col;
	self->grid->selected_row = row;

	return 1;
}

static int select_left(gp_widget *self, int sel)
{
	unsigned int col = self->grid->selected_col;
	unsigned int row = self->grid->selected_row;

	for (;;) {
		if (col == 0)
			return 0;

		if (try_select(self, --col, row, sel))
			return 1;
	}
}

static int select_right(gp_widget *self, int sel)
{
	unsigned int col = self->grid->selected_col;
	unsigned int row = self->grid->selected_row;

	for (;;) {
		if (++col >= self->grid->cols)
			return 0;

		if (try_select(self, col, row, sel))
			return 1;
	}
}

static int select_up(gp_widget *self, int sel)
{
	unsigned int col = self->grid->selected_col;
	unsigned int row = self->grid->selected_row;

	for (;;) {
		if (row == 0)
			return 0;

		if (try_select(self, col, --row, sel))
			return 1;
	}
}

static int select_down(gp_widget *self, int sel)
{
	unsigned int col = self->grid->selected_col;
	unsigned int row = self->grid->selected_row;

	for (;;) {
		if (++row >= self->grid->rows)
			return 0;

		if (try_select(self, col, row, sel))
			return 1;
	}
}

static int select_prev(gp_widget *self, int sel)
{
	unsigned int col = self->grid->selected_col;
	unsigned int row = self->grid->selected_row;

	for (;;) {
		if (col > 0) {
			col--;
		} else if (row > 0) {
			row--;
			col = self->grid->cols - 1;
		} else {
			return 0;
		}

		if (try_select(self, col, row, sel))
			return 1;
	}
}

static int select_next(gp_widget *self, int sel)
{
	unsigned int col = self->grid->selected_col;
	unsigned int row = self->grid->selected_row;

	for (;;) {
		if (col + 1 < self->grid->cols) {
			col++;
		} else if (row + 1 < self->grid->rows) {
			col = 0;
			row++;
		} else {
			return 0;
		}

		if (try_select(self, col, row, sel))
			return 1;
	}
}

static int select_event(gp_widget *self, int sel)
{
	gp_widget *w = widget_grid_selected(self);

	if (gp_widget_ops_render_select(w, sel))
		return 1;

	switch (sel) {
	case GP_SELECT_IN:
		return try_select(self, self->grid->selected_col, self->grid->selected_row, sel);
	case GP_SELECT_NEXT:
		return select_next(self, sel);
	case GP_SELECT_PREV:
		return select_prev(self, sel);
	case GP_SELECT_UP:
		return select_up(self, sel);
	case GP_SELECT_DOWN:
		return select_down(self, sel);
	case GP_SELECT_LEFT:
		return select_left(self, sel);
	case GP_SELECT_RIGHT:
		return select_right(self, sel);
	}

	return 0;
}

static int coord_search(unsigned int coord,
                        unsigned int *sizes, unsigned int *offsets,
                        unsigned int len)
{
	unsigned int i;

	if (coord < offsets[0])
		return -1;

	//TODO interval divison?
	for (i = 0; i < len; i++) {
		if (coord >= offsets[i] && coord <= offsets[i] + sizes[i])
			return i;
	}

	return -1;
}

static int select_xy(gp_widget *self, unsigned int x, unsigned int y)
{
	int col, row;
	struct gp_widget_grid *grid = self->grid;

	col = coord_search(x, grid->cols_w, grid->cols_off, grid->cols);
	row = coord_search(y, grid->rows_h, grid->rows_off, grid->rows);

	if (col < 0 || row < 0)
		return 0;

	if (!gp_widget_ops_render_select_xy(widget_grid_get(self, col, row), x, y))
		return 0;

	if (grid->selected_col != (unsigned int)col || grid->selected_row != (unsigned int)row)
		gp_widget_ops_render_select(widget_grid_selected(self), GP_SELECT_OUT);

	grid->selected_col = col;
	grid->selected_row = row;

	return 1;
}

static void set_hborder(gp_widget *self, uint8_t border)
{
	self->grid->col_padds[0] = border;
	self->grid->col_padds[self->grid->cols] = border;
}

static void set_vborder(gp_widget *self, uint8_t border)
{
	self->grid->row_padds[0] = border;
	self->grid->row_padds[self->grid->rows] = border;
}

static void set_border(gp_widget *self, uint8_t border)
{
	set_vborder(self, border);
	set_hborder(self, border);
}

static int get_uint8(const char *str, uint8_t *val, char **end,
                     const char *sarray, const char *name)
{
	long v = strtol(str, end, 10);

	if (str == *end) {
		GP_WARN("%s: Expected number '%s':%li",
			name, sarray, (long)(str-sarray));
		return 1;
	}

	if (v < 0 || v > 0xff) {
		GP_WARN("%s: Number outside of bounds '%li' at '%s':%li",
			name, v, sarray, (long)(str - sarray));
		return 1;
	}

	*val = v;

	return 0;
}

static void parse_strarray(const char *sarray, uint8_t *array, unsigned int len,
                           const char *name)
{
	const char *str = sarray;
	unsigned int i = 0;

	if (!sarray)
		return;

	for (;;) {
		char *end;
		uint8_t val;

		if (get_uint8(str, &val, &end, sarray, name))
			return;

		str = end;

		while (isspace(*str))
			str++;

		switch (*str) {
		case ',':
			if (i >= len) {
				GP_WARN("%s: Index out of bounds at '%s':%li",
				        name, sarray, (long)(str - sarray));
				return;
			}
			array[i++] = val;
			str++;
			continue;
		break;
		case '*':
		break;
		case '\0':
			if (i >= len) {
				GP_WARN("%s: Index out of bounds at '%s':%li",
				        name, sarray, (long)(str - sarray));
				return;
			}

			array[i++] = val;

			if (i != len) {
				GP_WARN("%s: Array too short expected %u numbers.",
					name, len);
			}

			return;
		default:
			GP_WARN("%s: Unexpected character '%c' at '%s':%li",
			        name, *str, sarray, (long)(str - sarray));
			return;
		}

		unsigned int j, mul = val;

		str++;

		if (get_uint8(str, &val, &end, sarray, name))
			return;

		for (j = 0; j < mul; j++) {
			if (i >= len) {
				GP_WARN("%s: Index out of bounds at '%s':%li",
			                name, sarray, (long)(str - sarray));
				return;
			}
			array[i++] = val;
		}

		str = end;

		while (isspace(*str))
			str++;

		if (*str != ',' && *str != '\0') {
			GP_WARN("%s: Unexpected character '%c' at '%s':%li",
			        name, *str, sarray, (long)(str - sarray));
			return;
		}

		if (*str == '\0') {
			if (i != len) {
				GP_WARN("%s: Array too short expected %u numbers.",
					name, len);
			}
			return;
		}

		str++;
	}
}

static gp_widget *json_to_grid(json_object *json, void **uids)
{
	int cols = 0, rows = 0, frame = 0;
	json_object *widgets = NULL;
	const char *border = NULL;
	const char *cpad = NULL;
	const char *rpad = NULL;
	const char *cpadf = NULL;
	const char *rpadf = NULL;
	const char *cfill = NULL;
	const char *rfill = NULL;
	int uniform = 0;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "cols"))
			cols = json_object_get_int(val);
		else if (!strcmp(key, "rows"))
			rows = json_object_get_int(val);
		else if (!strcmp(key, "widgets"))
			widgets = val;
		else if (!strcmp(key, "border"))
			border = json_object_get_string(val);
		else if (!strcmp(key, "cpad"))
			cpad = json_object_get_string(val);
		else if (!strcmp(key, "rpad"))
			rpad = json_object_get_string(val);
		else if (!strcmp(key, "cpadf"))
			cpadf = json_object_get_string(val);
		else if (!strcmp(key, "rpadf"))
			rpadf = json_object_get_string(val);
		else if (!strcmp(key, "cfill"))
			cfill = json_object_get_string(val);
		else if (!strcmp(key, "rfill"))
			rfill = json_object_get_string(val);
		else if (!strcmp(key, "frame"))
			frame = !!json_object_get_int(val);
		else if (!strcmp(key, "uniform"))
			uniform = 1;
		else
			GP_WARN("Invalid grid key '%s'", key);
	}

	if (!cols)
		cols = 1;

	if (!rows)
		rows = 1;

	if (cols <= 0 || rows <= 0 || !widgets) {
		GP_WARN("Invalid grid widget!");
		return NULL;
	}

	gp_widget *grid = gp_widget_grid_new(cols, rows);
	if (!grid)
		return NULL;

	grid->grid->frame = frame;
	grid->grid->uniform = uniform;

	parse_strarray(cpad, grid->grid->col_padds, cols+1, "Grid cpad");
	parse_strarray(rpad, grid->grid->row_padds, rows+1, "Grid rpad");
	parse_strarray(cpadf, grid->grid->col_pfills, cols+1, "Grid cpadf");
	parse_strarray(rpadf, grid->grid->row_pfills, rows+1, "Grid rpadf");
	parse_strarray(cfill, grid->grid->col_fills, cols, "Grid cfill");
	parse_strarray(rfill, grid->grid->row_fills, rows, "Grid rfill");

	if (border) {
		if (!strcmp(border, "horiz")) {
			set_vborder(grid, 0);
		} else if (!strcmp(border, "vert")) {
			set_hborder(grid, 0);
		} else if (!strcmp(border, "none")) {
			set_border(grid, 0);
		} else if (!strcmp(border, "all")) {
			//default
		} else {
			int b = atoi(border);

			if (b > 0)
				set_border(grid, b);
			else
				GP_WARN("Invalid border '%s'", border);
		}
	}

	if (!json_object_is_type(widgets, json_type_array)) {
		GP_WARN("Grid key widgets has to be array!");
		return grid;
	}

	int col, row;

	for (col = 0; col < cols; col++) {
		for (row = 0; row < rows; row++) {
			json_object *json_widget = json_object_array_get_idx(widgets, col * rows + row);

			if (!json_widget) {
				GP_WARN("Not enough widgets to fill grid!");
				return grid;
			}

			gp_widget *widget = gp_widget_from_json(json_widget, uids);

			if (widget)
				gp_widget_grid_put(grid, col, row, widget);
		}
	}

	if (json_object_array_get_idx(widgets, cols * rows))
		GP_WARN("Too many widgets in grid!");

	return grid;
}

static void free_(gp_widget *self)
{
	unsigned int x, y;

	for (y = 0; y < self->grid->rows; y++) {
		for (x = 0; x < self->grid->cols; x++)
			gp_widget_free(widget_grid_get(self, x, y));
	}
}

struct gp_widget_ops gp_widget_grid_ops = {
	.init = init,
	.min_w = min_w,
	.min_h = min_h,
	.render = render,
	.event = event,
	.free = free_,
	.select = select_event,
	.select_xy = select_xy,
	.distribute_size = distribute_size,
	.from_json = json_to_grid,
	.id = "grid",
};

gp_widget *gp_widget_grid_new(unsigned int cols, unsigned int rows)
{
	size_t payload_size = sizeof(struct gp_widget_grid);
	unsigned int i;
	gp_widget *ret;

	/* cols + rows widths heights and offsets */
	payload_size += 2 * (cols + rows) * sizeof(unsigned int);
	/* pads and pfills */
	payload_size += 2 * (cols + rows + 2) * sizeof(uint8_t);
	/* cell fills */
	payload_size += (cols + rows) * sizeof(uint8_t);
	/* widget pointers */
	payload_size += (cols * rows) * sizeof(void*);

	ret = gp_widget_new(GP_WIDGET_GRID, payload_size);
	if (!ret)
		return NULL;

	void *buf = ret->grid->payload;

	ret->grid->cols = cols;
	ret->grid->rows = rows;
	ret->grid->widgets = buf;
	buf += (cols * rows) * sizeof(void*);
	ret->grid->cols_w = buf;
	buf += cols * sizeof(unsigned int);
	ret->grid->rows_h = buf;
	buf += rows * sizeof(unsigned int);
	ret->grid->cols_off = buf;
	buf += cols * sizeof(unsigned int);
	ret->grid->rows_off = buf;
	buf += rows * sizeof(unsigned int);
	ret->grid->col_padds = buf;
	buf += (cols + 1) * sizeof(uint8_t);
	ret->grid->row_padds = buf;
	buf += (rows + 1) * sizeof(uint8_t);
	ret->grid->col_pfills = buf;
	buf += (cols + 1) * sizeof(uint8_t);
	ret->grid->row_pfills = buf;
	buf += (rows + 1) * sizeof(uint8_t);
	ret->grid->col_fills = buf;
	buf += cols * sizeof(uint8_t);
	ret->grid->row_fills = buf;

	for (i = 0; i <= cols; i++)
		ret->grid->col_padds[i] = 1;

	for (i = 0; i < cols; i++)
		ret->grid->col_fills[i] = 1;

	for (i = 0; i <= rows; i++)
		ret->grid->row_padds[i] = 1;

	for (i = 0; i < rows; i++)
		ret->grid->row_fills[i] = 1;

	return ret;
}

static int assert_col_row(gp_widget *self, unsigned int col, unsigned int row)
{
	if (col >= self->grid->cols) {
		GP_BUG("Invalid column index %u Grid %p %ux%u",
			col, self, self->grid->cols, self->grid->rows);
		return 1;
	}

	if (row >= self->grid->rows) {
		GP_BUG("Invalid row index %u Grid %p %ux%u",
			row, self, self->grid->cols, self->grid->rows);
		return 1;
	}

	return 0;
}

gp_widget *gp_widget_grid_put(gp_widget *self, unsigned int col, unsigned int row,
                            gp_widget *widget)
{
	gp_widget *ret;

	GP_WIDGET_ASSERT(self, GP_WIDGET_GRID, NULL);

	if (assert_col_row(self, col, row))
		return NULL;

	if (widget && widget->parent) {
		//TODO: remove from parent!!!
		return NULL;
	}

	ret = widget_grid_put(self, widget, col, row);
	if (ret)
		ret->parent = NULL;

	gp_widget_resize(self);

	return ret;
}

gp_widget *gp_widget_grid_rem(gp_widget *self, unsigned int col, unsigned int row)
{
	gp_widget *ret;

	GP_WIDGET_ASSERT(self, GP_WIDGET_GRID, NULL);

	if (assert_col_row(self, col, row))
		return NULL;

	ret = widget_grid_put(self, NULL, col, row);
	if (ret)
		ret->parent = NULL;

	gp_widget_resize(self);

	return ret;
}

gp_widget *gp_widget_grid_get(gp_widget *self, unsigned int col, unsigned int row)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_GRID, NULL);

	if (assert_col_row(self, col, row))
		return NULL;

	return widget_grid_get(self, col, row);
}
