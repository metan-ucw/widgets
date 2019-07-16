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

static unsigned int min_w(gp_widget *self)
{
	return 2 * cfg->padd + gp_text_width(cfg->font, self->b->label);
}

static unsigned int min_h(gp_widget *self)
{
	(void)self;

	return 2 * cfg->padd + gp_text_ascent(cfg->font);
}

static void render(gp_widget *self,
                   struct gp_widget_render *render, int flags)
{
	unsigned int x = self->x;
	unsigned int y = self->y;
	unsigned int w = self->w;
	unsigned int h = self->h;

	(void)flags;

	gp_pixel bg_color = self->b->val ? cfg->bg_color : cfg->fg_color;
	gp_pixel fr_color = self->selected ? cfg->sel_color : cfg->text_color;

	gp_fill_rrect_xywh(render->buf, x, y, w, h, cfg->bg_color, bg_color, fr_color);

	unsigned int cx = self->x + self->w/2;
	unsigned int cy = self->y + self->h/2 - gp_text_ascent(cfg->font)/2;

	gp_text(render->buf, cfg->font,
		cx, cy, GP_ALIGN_CENTER|GP_VALIGN_BELOW,
		cfg->text_color, bg_color, self->b->label);

	if (self->b->val)
		gp_widget_render_timer(self, 0, 200);

}

static void set(gp_widget *self)
{
	if (self->b->val)
		return;

	self->b->val = 1;

	gp_widget_redraw(self);

	gp_widget_send_event(self, GP_WIDGET_EVENT_ACTION);
}

static void click(gp_widget *self, gp_event *ev)
{
	unsigned int min_x = self->x + cfg->padd;
	unsigned int max_x = self->x + self->w - cfg->padd;
	unsigned int min_y = self->y + cfg->padd;
	unsigned int max_y = self->y + self->w - cfg->padd;

	if (ev->cursor_x < min_x || ev->cursor_x > max_x)
		return;

	if (ev->cursor_y < min_y || ev->cursor_y > max_y)
		return;

	set(self);
}

static int event(gp_widget *self, gp_event *ev)
{
	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code == GP_EV_KEY_UP)
			return 0;

		switch (ev->val.val) {
		case GP_KEY_SPACE:
		case GP_KEY_ENTER:
			set(self);
			return 1;
		case GP_BTN_LEFT:
			click(self, ev);
			return 1;
		}
	break;
	case GP_EV_TMR:
		self->b->val = 0;
		gp_widget_redraw(self);
		return 1;
	break;
	}

	return 0;
}

static gp_widget *json_to_button(json_object *json, void **uids)
{
	const char *label = NULL;

	(void)uids;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "label"))
			label = json_object_get_string(val);
		else
			GP_WARN("Invalid button key '%s'", key);
	}

	return gp_widget_button_new(label, NULL, NULL);
}

struct gp_widget_ops gp_widget_button_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.render = render,
	.event = event,
	.from_json = json_to_button,
	.id = "button",
};

struct gp_widget *gp_widget_button_new(const char *label,
                                       int (*on_event)(gp_widget_event *ev),
                                       void *event_ptr)
{
	gp_widget *ret;
	size_t size = sizeof(struct gp_widget_bool) + strlen(label) + 1;

	ret = gp_widget_new(GP_WIDGET_BUTTON, size);
	if (!ret)
		return NULL;

	ret->btn->label = ret->btn->payload;
	ret->on_event = on_event;
	ret->on_event_ptr = event_ptr;

	strcpy(ret->btn->payload, label);

	return ret;
}
