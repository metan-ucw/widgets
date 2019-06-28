//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <gp_widgets.h>

static int callback(gp_widget_event *ev)
{
	printf("Callback %p %s!\n", ev->self, ev->self->btn->label);

	return 0;
}

#include "t5.h"

int main(void)
{
	gp_widgets_init(&layout);
	gp_widgets_main_loop(&layout, "t0", NULL);

	return 0;
}
