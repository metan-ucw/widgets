//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <gp_widgets.h>

int callback(gp_widget_event *ev)
{
	if (ev->type == GP_WIDGET_EVENT_NEW)
		return 0;

	switch (ev->self->type) {
	case GP_WIDGET_TEXTBOX:
		if (ev->type == GP_WIDGET_EVENT_EDIT)
			printf("Text box edit '%s'\n", ev->self->tbox->buf);

		if (ev->type == GP_WIDGET_EVENT_ACTION)
			printf("Text box enter\n");
	break;
	case GP_WIDGET_BUTTON:
		printf("Button pressed!\n");
	break;
	case GP_WIDGET_CHECKBOX:
		printf("Checkbox set to %i\n", ev->self->chbox->val);
	break;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	gp_widget *layout = gp_widget_layout_json("t4.json", NULL);
	if (!layout)
		return 0;

	gp_widgets_main_loop(layout, "t4", NULL, argc, argv);

	return 0;
}
