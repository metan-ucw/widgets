//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <gp_widgets.h>

static void *uids;

int button_callback(gp_widget_event *ev)
{
	if (ev->type == GP_WIDGET_EVENT_NEW)
		return 0;

	gp_widget *pass = gp_widget_by_uid(uids, "passwd", GP_WIDGET_TEXTBOX);

	if (pass)
		printf("Password: %s\n", pass->tbox->buf);

	return 0;
}

int main(void)
{
	gp_widget *layout = gp_widget_layout_json("t0.json", &uids);
	if (!layout)
		return 0;

	gp_widgets_main_loop(layout, "t0", NULL);

	return 0;
}
