//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>

static int check_val(int min, int max, int val)
{
	if (val < min || val > max) {
		GP_WARN("Val %i outside of Min %i Max %i", val, min, max);
		return 1;
	}

	return 0;
}

static int check_min_max(int min, int max)
{
	if (min > max) {
		GP_WARN("Min %i > Max %i", min, max);
		return 1;
	}

	return 0;
}


static gp_widget *widget_int_new(enum gp_widget_type type,
                                 int min, int max, int val,
                                 int (*on_event)(gp_widget_event *), void *event_ptr)
{
	gp_widget *ret;

	if (check_min_max(min, max))
		return NULL;

	if (check_val(min, max, val))
		return NULL;

	ret = gp_widget_new(type, sizeof(struct gp_widget_int));
	if (!ret)
		return NULL;

	ret->i->min = min;
	ret->i->max = max;
	ret->i->val = val;

	ret->i->event_ptr = event_ptr;
	ret->i->on_event = on_event;

	gp_widget_send_event(on_event, ret, event_ptr, GP_WIDGET_EVENT_NEW);

	return ret;
}

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

	int wd = w * self->i->val / (self->i->max - self->i->min);

	gp_fill_rect_xywh(render->buf, x, y, wd, h, cfg->fg_color);
	gp_fill_rect_xywh(render->buf, x+wd, y, w - wd, h, cfg->bg_color);

	gp_rect_xywh(render->buf, x, y, w, h, cfg->text_color);

	gp_print(render->buf, cfg->font, x + w/2, y + cfg->padd,
		 GP_ALIGN_CENTER | GP_VALIGN_BELOW,
		 cfg->text_color, cfg->bg_color, "%.2f%%",
		 100.00 * self->i->val / (self->i->max - self->i->min));
}

static gp_widget *json_to_int(enum gp_widget_type type, json_object *json, void **uids)
{
	const char *on_event = NULL;
	void *on_event_fn = NULL;
	int min = 0, max = 0, ival = 0;

	(void)uids;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "on_event"))
			on_event = json_object_get_string(val);
		else if (!strcmp(key, "min"))
			min = json_object_get_int(val);
		else if (!strcmp(key, "max"))
			max = json_object_get_int(val);
		else if (!strcmp(key, "val"))
			ival = json_object_get_int(val);
		else
			GP_WARN("Invalid int key '%s'", key);
	}

	if (check_min_max(min, max))
		return NULL;

	if (check_val(min, max, ival))
		return NULL;

	if (on_event) {
		on_event_fn = gp_widget_callback_addr(on_event);

		if (!on_event_fn)
			GP_WARN("No on_event function '%s' defined", on_event);
	}

	return widget_int_new(type, min, max, ival, on_event_fn, NULL);
}

static gp_widget *json_to_pbar(json_object *json, void **uids)
{
	return json_to_int(GP_WIDGET_PROGRESSBAR, json, uids);
}

struct gp_widget_ops gp_widget_progress_bar_ops = {
	.min_w = pbar_min_w,
	.min_h = pbar_min_h,
	.render = pbar_render,
	.from_json = json_to_pbar,
	.id = "progressbar",
};

gp_widget *gp_widget_progress_bar_new(int min, int max, int val)
{
	return widget_int_new(GP_WIDGET_PROGRESSBAR, min, max, val, NULL, NULL);
}

int gp_widget_progress_bar_set_max(gp_widget *self, int max)
{
	int ret;
	int redraw = 0;

	GP_WIDGET_ASSERT(self, GP_WIDGET_PROGRESSBAR, 0);

	ret = self->i->max;

	GP_DEBUG(3, "Setting widget (%p) progressbar max '%i' -> '%i'",
		 self, ret, max);

	if (check_min_max(self->i->min, max))
		return 0;

	if (self->i->val > max) {
		self->i->val = max;
		redraw = 1;
	}

	if (self->i->max != max) {
		self->i->max = max;
		redraw = 1;
	}

	if (redraw)
		gp_widget_redraw(self);

	return ret;
}

int gp_widget_progress_bar_set_min(gp_widget *self, int min)
{
	int ret;
	int redraw = 0;

	GP_WIDGET_ASSERT(self, GP_WIDGET_PROGRESSBAR, 0);

	ret = self->i->min;

	GP_DEBUG(3, "Setting widget (%p) progressbar min '%i' -> '%i'",
		 self, ret, min);

	if (check_min_max(min, self->i->max))
		return 0;

	if (self->i->val < min) {
		self->i->val = min;
		redraw = 1;
	}

	if (self->i->min != min) {
		self->i->min = min;
		redraw = 1;
	}

	if (redraw)
		gp_widget_redraw(self);

	return ret;
}

int gp_widget_progress_bar_set(gp_widget *self, int val)
{
	int ret;

	GP_WIDGET_ASSERT(self, GP_WIDGET_PROGRESSBAR, 0);

	ret = self->i->val;

	GP_DEBUG(3, "Setting widget (%p) progressbar val '%i' -> '%i'",
		 self, ret, val);

	if (check_val(self->i->min, self->i->max, val))
		return 0;

	if (self->i->val != val) {
		gp_widget_redraw(self);
		self->i->val = val;
	}

	return ret;
}

static unsigned int spin_min_w(gp_widget *self)
{
	unsigned int min_digits = snprintf(NULL, 0, "%i", self->spin->min);
	unsigned int max_digits = snprintf(NULL, 0, "%i", self->spin->max);

	unsigned int ret = 2 * cfg->padd;

	ret += gp_text_max_width_chars(cfg->font, "-0123456789",
			               GP_MAX(min_digits, max_digits));

	ret += GP_ODD_UP(gp_text_max_width(cfg->font, 1));

	return ret;
}

static unsigned int spin_min_h(gp_widget *self)
{
	(void)self;

	return 2 * cfg->padd + gp_text_ascent(cfg->font);
}

static void spin_render(gp_widget *self,
                        struct gp_widget_render *render, int flags)
{
	unsigned int x = self->x;
	unsigned int y = self->y;
	unsigned int w = self->w;
	unsigned int h = self->h;
	unsigned int s = GP_ODD_UP(gp_text_max_width(cfg->font, 1));

	(void)flags;

	gp_fill_rect_xywh(render->buf, x, y, w, h, cfg->fg_color);

	gp_print(render->buf, cfg->font, self->x + w - s - cfg->padd, y + cfg->padd,
		 GP_ALIGN_LEFT | GP_VALIGN_BELOW,
		 cfg->text_color, cfg->bg_color, "%i", self->spin->val);

	gp_pixel color = self->selected ? cfg->sel_color : cfg->text_color;

	if (self->spin->alert) {
		color = cfg->alert_color;
		gp_widget_render_timer(self, GP_TIMER_RESCHEDULE, 500);
	}

	gp_rect_xywh(render->buf, x, y, w, h, color);

	x += w - s;

	gp_vline_xyh(render->buf, x-1, y, h, color);
	gp_hline_xyw(render->buf, x, y + h/2, s, color);

	gp_triangle_up(render->buf, self->x + w - s/2 - 1, y + h/4, s/2, cfg->text_color);
	gp_triangle_down(render->buf, self->x + w - s/2 - 1, y + (h/4) * 3, s/2, cfg->text_color);
}

static void schedule_alert(gp_widget *self)
{
	self->spin->alert = 1;
	gp_widget_redraw(self);
}

static void spin_inc(gp_widget *self)
{
	if (self->spin->val >= self->spin->max) {
		schedule_alert(self);
		return;
	}

	self->spin->val++;
	gp_widget_redraw(self);
}

static void spin_dec(gp_widget *self)
{
	if (self->spin->val <= self->spin->min) {
		schedule_alert(self);
		return;
	}

	self->spin->val--;
	gp_widget_redraw(self);
}

static void spin_click(gp_widget *self, gp_event *ev)
{
	unsigned int s = gp_text_max_width(cfg->font, 1);
	unsigned int min_x = self->x + self->w - cfg->padd - s;
	unsigned int max_x = self->x + self->w - cfg->padd;
	unsigned int min_y = self->y + cfg->padd;
	unsigned int max_y = self->y + self->h - cfg->padd;
	unsigned int mid_y = (min_y + max_y) / 2;

	if (ev->cursor_x < min_x || ev->cursor_x > max_x)
		return;

	if (ev->cursor_y < min_y || ev->cursor_y > max_y)
		return;

	if (ev->cursor_y < mid_y)
		spin_inc(self);
	else
		spin_dec(self);
}

static void spin_min(gp_widget *self)
{
	self->spin->val = self->spin->min;
	gp_widget_redraw(self);
}

static void spin_max(gp_widget *self)
{
	self->spin->val = self->spin->max;
	gp_widget_redraw(self);
}
static int spin_event(gp_widget *self, gp_event *ev)
{
	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code == GP_EV_KEY_UP)
			return 0;

		switch (ev->val.val) {
		case GP_BTN_LEFT:
			spin_click(self, ev);
			return 1;
		//TODO: Inc by 10 with Shift
		case GP_KEY_UP:
			spin_inc(self);
			return 1;
		case GP_KEY_DOWN:
			spin_dec(self);
			return 1;
		case GP_KEY_HOME:
			spin_min(self);
			return 1;
		case GP_KEY_END:
			spin_max(self);
			return 1;
		}
	break;
	case GP_EV_TMR:
		self->spin->alert = 0;
		gp_widget_redraw(self);
		return 1;
	break;
	}

	return 0;
}

static gp_widget *json_to_spin(json_object *json, void **uids)
{
	return json_to_int(GP_WIDGET_SPINNER, json, uids);
}

struct gp_widget_ops gp_widget_spinner_ops = {
	.min_w = spin_min_w,
	.min_h = spin_min_h,
	.render = spin_render,
	.event = spin_event,
	.from_json = json_to_spin,
	.id = "spinner",
};

gp_widget *gp_widget_spinner_new(int min, int max, int val)
{
	return widget_int_new(GP_WIDGET_SPINNER, min, max, val, NULL, NULL);
}

