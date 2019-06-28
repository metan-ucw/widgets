//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <gp_widgets.h>

struct gp_widget layout = {
	.type = GP_WIDGET_GRID,
	.align = GP_FILL(1, 1),
	.grid = &(struct gp_widget_grid) {
		.cols = 3, .rows = 1,
		.cols_w = (unsigned int []){0, 0, 0},
		.rows_h = (unsigned int []){0},
		.col_padds = (int []){1, 1, 1, 1},
		.row_padds = (int []){1, 1},
		.widgets = (gp_widget* []) {
			&(gp_widget) {
				.type = GP_WIDGET_LABEL,
				.label = &(struct gp_widget_label) {
					.text = "-------",
				}
			},
			&(gp_widget) {
				.type = GP_WIDGET_VOID,
				.align = GP_FILL(1, 1)
			},
			&(gp_widget) {
				.type = GP_WIDGET_LABEL,
				.label = &(struct gp_widget_label) {
					.text = "-------",
				}
			}
		}
	}
};

int main(void)
{
	gp_widgets_init(&layout);
	gp_widgets_main_loop(&layout, "t2", NULL);

	return 0;
}
