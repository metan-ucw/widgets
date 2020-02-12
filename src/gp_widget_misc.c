//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <gp_widgets.h>
#include <gp_widget_ops.h>

static unsigned int min_w(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	return self->misc->ops->min_w(self, ctx);
}

static unsigned int min_h(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	return self->misc->ops->min_h(self, ctx);
}

struct gp_widget_ops gp_widget_misc_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.id = "Misc",
};
