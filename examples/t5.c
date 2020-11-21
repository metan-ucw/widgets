//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <gp_widgets.h>

static char *get(unsigned int var_id, char *buf)
{
	switch (var_id) {
	case 0:
		return gp_vec_printf(buf, "abc");
	break;
	case 1:
		return gp_vec_printf(buf, "%.2f", 1.00 * random() / 10000);
	break;
	}

	return NULL;
}

static int button_callback(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return -1;

	gp_widget_markup_refresh(ev->self->priv);
	return 0;
}

int main(int argc, char *argv[])
{
	gp_widget *layout = gp_widget_grid_new(1, 2);
	gp_widget *markup = gp_widget_markup_new("{mmm} test {0.0} m/s\nSecond Line\n*Bold* \\{}\\*", get);
	gp_widget *button = gp_widget_button_new("Refresh", button_callback, layout);

	gp_widget_grid_put(layout, 0, 0, markup);
        gp_widget_grid_put(layout, 0, 1, button);

	button->priv = markup;

	gp_widgets_main_loop(layout, "Markup test", NULL, argc, argv);

	return 0;
}
