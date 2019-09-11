//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>

static unsigned int pbar_min_w(gp_widget *self)
{
	(void)self;

	return 2 * cfg->padd + gp_text_max_width(cfg->font, 7);
}

static unsigned int pbar_min_h(gp_widget *self)
{
	(void)self;

	return 2 * cfg->padd + gp_text_ascent(cfg->font);
}

static void pbar_render(gp_widget *self,
                   struct gp_widget_render *render, int flags)
{
	unsigned int x = self->x;
	unsigned int y = self->y;
	unsigned int w = self->w;
	unsigned int h = self->h;

	(void)flags;

	unsigned int wd = self->pbar->val * w / 100;

	gp_pixmap p;

	gp_sub_pixmap(render->buf, &p, x, y, wd, h);
	if (p.w > 0) {
		gp_fill_rrect_xywh(&p, 0, 0, w, h, cfg->bg_color,
		                   cfg->fg2_color, cfg->text_color);
	}

	gp_sub_pixmap(render->buf, &p, x+wd, y, w-wd, h);
	if (p.w > 0) {
		gp_fill_rrect_xywh(&p, -wd, 0, w, h, cfg->bg_color,
		                   cfg->fg_color, cfg->text_color);
	}

	gp_print(render->buf, cfg->font, x + w/2, y + cfg->padd,
		 GP_ALIGN_CENTER | GP_VALIGN_BELOW | GP_TEXT_NOBG,
		 cfg->text_color, cfg->bg_color, "%.2f%%",
		 self->pbar->val);
}

static int check_val(double val)
{
	if (val < 0 || val > 100) {
		GP_WARN("Invalid progressbar value %lf", val);
		return 1;
	}

	return 0;
}

static gp_widget *json_to_pbar(json_object *json, void **uids)
{
	double val = 0;

	(void)uids;

	json_object_object_foreach(json, key, jval) {
		if (!strcmp(key, "val"))
			val = json_object_get_double(jval);
		else
			GP_WARN("Invalid int pbar '%s'", key);
	}

	if (check_val(val))
		val = 0;

	return gp_widget_pbar_new(val);
}

struct gp_widget_ops gp_widget_progress_bar_ops = {
	.min_w = pbar_min_w,
	.min_h = pbar_min_h,
	.render = pbar_render,
	.from_json = json_to_pbar,
	.id = "progressbar",
};

gp_widget *gp_widget_pbar_new(float val)
{
	gp_widget *ret;

	if (check_val(val))
		val = 0;

	ret = gp_widget_new(GP_WIDGET_PROGRESSBAR, sizeof(struct gp_widget_pbar));
	if (!ret)
		return NULL;

	ret->pbar->val = val;

	return ret;
}

void gp_widget_pbar_set(gp_widget *self, float val)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_PROGRESSBAR, );

	GP_DEBUG(3, "Setting widget (%p) progressbar val '%.2f' -> '%.2f'",
		 self, self->pbar->val, val);

	if (check_val(val))
		return;

	if (self->pbar->val != val) {
		gp_widget_redraw(self);
		self->pbar->val = val;
	}
}
