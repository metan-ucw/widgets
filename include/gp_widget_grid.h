//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_GRID_H__
#define GP_WIDGET_GRID_H__

#include <stdint.h>

struct gp_widget_grid {
	unsigned int cols, rows;

	unsigned int focused_col;
	unsigned int focused_row;

	/* if set a widget in a grid is focused */
	int focused:1;
	/* if set frame is rendered around the grid */
	int frame:1;
	/* if set the grid all columns and all rows have the same size */
	int uniform:1;

	/** column/row sizes */
	unsigned int *cols_w;
	unsigned int *rows_h;

	/** column/row offsets */
	unsigned int *cols_off;
	unsigned int *rows_off;

	/* padding and padding fills */
	uint8_t *col_padds;
	uint8_t *row_padds;
	uint8_t *col_pfills;
	uint8_t *row_pfills;

	/* cell fill coeficients */
	uint8_t *col_fills;
	uint8_t *row_fills;

	gp_widget **widgets;
};

gp_widget *gp_widget_grid_new(unsigned int cols, unsigned int rows);

/*
 * Puts wiget to be placed inside of the grid, returns previous widget in the
 * grid.
 */
gp_widget *gp_widget_grid_put(gp_widget *self, unsigned int col, unsigned int row,
		              gp_widget *widget);

/*
 * Add a new (empty) row to the grid.
 */
void gp_widget_grid_add_row(gp_widget *self);

/*
 * Removes widget at col, row.
 */
gp_widget *gp_widget_grid_rem(gp_widget *self, unsigned int col, unsigned int row);

/*
 * Just returns pointer at col, row.
 */
gp_widget *gp_widget_grid_get(gp_widget *self, unsigned int col, unsigned int row);

/*
 * @brief Sets both horizontal and vertical border padding and filling coeficients.
 *
 * @self A grid widget.
 * @padd Paddin coeficient.
 * @fill A filling coeficient.
 */
void gp_widget_grid_border_set(gp_widget *self, unsigned int padd, unsigned int fill);

/*
 * @brief Sets horizontal border padding and filling coeficients.
 *
 * @self A grid widget.
 * @padd Paddin coeficient.
 * @fill A filling coeficient.
 */
void gp_widget_grid_hborder_set(gp_widget *self, unsigned int padd, unsigned int fill);

/*
 * @brief Sets vertical border padding and filling coeficients.
 *
 * @self A grid widget.
 * @padd Paddin coeficient.
 * @fill A filling coeficient.
 */
void gp_widget_grid_vborder_set(gp_widget *self, unsigned int padd, unsigned int fill);

#endif /* GP_WIDGET_GRID_H__ */
