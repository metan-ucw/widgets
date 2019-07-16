//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <gp_widgets.h>

static void *uids;

int login_callback(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	gp_widget *pass = gp_widget_by_uid(uids, "pass", GP_WIDGET_TEXTBOX);
	gp_widget *uname = gp_widget_by_uid(uids, "uname", GP_WIDGET_TEXTBOX);

	if (uname)
		printf("Username: '%s'\n", uname->tbox->buf);

	if (pass)
		printf("Password: '%s'\n", pass->tbox->buf);

	return 0;
}

int cancel_callback(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	//TODO: Inject exit into main loop?
	exit(0);
}

int show_password(gp_widget_event *ev)
{
	gp_widget *pass = gp_widget_by_uid(uids, "pass", GP_WIDGET_TEXTBOX);

	if (ev->self->b->val)
		pass->tbox->hidden = 0;
	else
		pass->tbox->hidden = 1;

	gp_widget_redraw(pass);

	return 0;
}

int main(int argc, char *argv[])
{
	const char *layout_path = "test_login_1.json";

	if (argv[1]) {
		layout_path = "test_login_2.json";
	}

	gp_widget *layout = gp_widget_layout_json(layout_path, &uids);
	if (!layout)
		return 0;

	gp_widgets_main_loop(layout, "Login!", NULL);

	return 0;
}
