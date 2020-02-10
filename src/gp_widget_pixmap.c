//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>
#include <gp_widget_json.h>

static unsigned int min_w(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	(void)cfg;

	return self->pixmap->min_w;
}

static unsigned int min_h(gp_widget *self, const gp_widget_render_cfg *cfg)
{
	(void)cfg;

	return self->pixmap->min_h;
}

static void render(gp_widget *self, const gp_widget_render_cfg *cfg, int flags)
{
	unsigned int x = self->x;
	unsigned int y = self->y;

	(void)flags;

	if (!self->pixmap->pixmap) {
		gp_pixmap pix;
		gp_sub_pixmap(cfg->buf, &pix, self->x, self->y, self->w, self->h);
		self->pixmap->pixmap = &pix;
		gp_widget_send_event(self, GP_WIDGET_EVENT_REDRAW, cfg);
		self->pixmap->pixmap = NULL;
		return;
	}

	gp_blit_xywh(self->pixmap->pixmap, 0, 0,
	             self->w, self->h,
	             cfg->buf, x, y);
}

/*
 * Dummy event handler so that events are propagated to app event handler since
 * widgets without event handler can't get focus and events.
 */
static int event(gp_widget *self, const gp_widget_render_cfg *cfg, gp_event *ev)
{
	(void)self;
	(void)cfg;
	(void)ev;
	return 0;
}

static gp_widget *json_to_pixmap(json_object *json, void **uids)
{
	unsigned int w = 0;
	unsigned int h = 0;

	(void)uids;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "w"))
			w = json_object_get_int(val);
		else if (!strcmp(key, "h"))
			h = json_object_get_int(val);
		else
			GP_WARN("Invalid pixmap key '%s'", key);
	}

	if (w <= 0 || h <= 0) {
		GP_WARN("Invalid pixmap size %ux%u\n", w, h);
		return NULL;
	}

	return gp_widget_pixmap_new(w, h, NULL, NULL);
}

struct gp_widget_ops gp_widget_pixmap_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.render = render,
	.event = event,
	.from_json = json_to_pixmap,
	.id = "pixmap",
};

struct gp_widget *gp_widget_pixmap_new(unsigned int w, unsigned int h,
                                       int (*on_event)(gp_widget_event *ev),
                                       void *event_ptr)
{
	gp_widget *ret;

	ret = gp_widget_new(GP_WIDGET_PIXMAP, sizeof(struct gp_widget_pixmap));
	if (!ret)
		return NULL;

	ret->on_event = on_event;
	ret->on_event_ptr = event_ptr;
	ret->pixmap->min_w = w;
	ret->pixmap->min_h = h;
	ret->pixmap->pixmap = NULL;

	return ret;
}
