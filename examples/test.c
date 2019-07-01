//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <stdio.h>
#include <gfxprim.h>
#include <gp_widgets.h>

int btn_ok_callback(gp_widget_event *ev)
{
	if (ev->type == GP_WIDGET_EVENT_NEW)
		return 0;

	printf("Callback OK!\n");
	return 0;
}

int btn_cancel_callback(gp_widget_event *ev)
{
	if (ev->type == GP_WIDGET_EVENT_NEW)
		return 0;

	printf("Callback Cancel!\n");
	return 0;
}

int textbox_filter(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_FILTER)
		return 0;

	char ch = ev->val;

	if (ch < '0' || ch > '9') {
		printf("Input filtered: '%c'\n", ch);
		return 1;
	}

	printf("Input: '%s'+'%c'\n", ev->self->tbox->buf, ch);

	return 0;
}

int radiobutton_callback(gp_widget_event *ev)
{
	if (ev->type == GP_WIDGET_EVENT_NEW)
		return 0;

	printf("selected choice %i\n", ev->self->choice->selected);

	return 0;
}

int slider_event(gp_widget_event *ev)
{
	if (ev->type == GP_WIDGET_EVENT_NEW)
		return 0;

	printf("Slider value=%i\n", ev->self->slider->val);

	return 0;
}

int main(void)
{
	gp_widget *layout = gp_widget_layout_json("test.json", NULL);
	if (!layout)
		return 0;

	gp_widgets_main_loop(layout, "Test", NULL);

	return 0;
}
