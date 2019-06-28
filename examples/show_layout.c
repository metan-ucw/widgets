//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <gp_widgets.h>

int on_event(gp_widget_event *ev)
{
	gp_widget_event_dump(ev);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s layout.json\n", argv[0]);
		return 1;
	}

	gp_widget *layout = gp_widget_layout_json(argv[1], NULL);
	if (!layout) {
		fprintf(stderr, "Layout cannot be loaded!\n");
		return 1;
	}

	gp_widgets_main_loop(layout, "Layout", NULL);

	return 0;
}
