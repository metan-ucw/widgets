//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <gp_widgets.h>

static uint32_t menu_callback(gp_timer *self)
{
	gp_widget *overlay = self->priv;

	gp_widget_overlay_hide(overlay, 1);

	return 0;
}

static gp_timer tmr = {
	.expires = 1000,
	.period = 0,
	.callback = menu_callback,
	.id = "Menu callback"
};

static int event_handler(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_INPUT)
		return 0;

	if (ev->input_ev->type == GP_EV_KEY)
		return 0;

	if (ev->input_ev->cursor_y < ev->self->y + ev->self->h/2)
		return 0;

	gp_widget_overlay_show(ev->self, 1);

	tmr.expires = 3000;

	gp_widgets_timer_rem(&tmr);
	gp_widgets_timer_ins(&tmr);

	return 1;
}

int main(int argc, char *argv[])
{
	gp_widget *layout = gp_widget_overlay_new(2);
	gp_widget *markup = gp_widget_markup_new("#Bottom widget#\n\nThis widget is on the bottom", NULL);
	gp_widget *btn_grid = gp_widget_grid_new(2, 1);
	gp_widget *markup_grid = gp_widget_grid_new(1, 1);

	//gp_widget_grid_border_set(btn_grid, 0, 0);
	gp_widget_grid_put(btn_grid, 0, 0, gp_widget_button_new("< Prev", NULL, NULL));
	gp_widget_grid_put(btn_grid, 1, 0, gp_widget_button_new("Next >", NULL, NULL));
	btn_grid->align = GP_BOTTOM;

	gp_widget_grid_put(markup_grid, 0, 0, markup);

	gp_widget_overlay_put(layout, 0, markup_grid);
	gp_widget_overlay_put(layout, 1, btn_grid);

	gp_widget_event_handler_set(layout, event_handler, layout);
	gp_widget_event_unmask(layout, GP_WIDGET_EVENT_INPUT);

	tmr.priv = layout;

	gp_widgets_timer_ins(&tmr);

	gp_widgets_main_loop(layout, "Layout overlay test", NULL, argc, argv);

	return 0;
}
