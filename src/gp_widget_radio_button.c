//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */
#include <string.h>
#include <json-c/json.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>
#include <gp_string.h>

static unsigned int min_w(gp_widget *self)
{
	unsigned int text_a = gp_text_ascent(cfg->font);
	unsigned int text_w = 0;
	unsigned int i, w;

	for (i = 0; i < self->choice->max; i++) {
		w = gp_text_width(cfg->font, self->choice->choices[i]);
		text_w = GP_MAX(text_w, w);
	}

	return cfg->padd + text_a + text_w;
}

static unsigned int min_h(gp_widget *self)
{
	return cfg->padd +
	       self->choice->max * (gp_text_ascent(cfg->font) + cfg->padd);
}

static void render(gp_widget *self,
                   struct gp_widget_render *render, int flags)
{
	unsigned int text_a = gp_text_ascent(cfg->font);
	unsigned int x = self->x;
	unsigned int y = self->y;
	unsigned int w = self->w;
	unsigned int h = self->h;
	unsigned int i;

	(void)flags;

	gp_fill_rect_xywh(render->buf, x, y, w, h, cfg->bg_color);

	y += cfg->padd;

	for (i = 0; i < self->choice->max; i++) {
		unsigned int r = text_a/2;
		unsigned int cy = y + r;
		unsigned int cx = x + r;

		gp_fill_circle(render->buf, cx, cy, r, cfg->fg_color);
		gp_pixel color = self->selected ? cfg->sel_color : cfg->text_color;
		gp_circle(render->buf, cx, cy, r, color);

		if (i == self->choice->selected) {
			gp_fill_circle(render->buf, cx, cy, r - 3,
			              cfg->text_color);
		}

		gp_text(render->buf, cfg->font,
			x + cfg->padd + text_a, y,
		        GP_ALIGN_RIGHT|GP_VALIGN_BELOW,
                        cfg->text_color, cfg->bg_color,
			self->choice->choices[i]);

		y += text_a + cfg->padd;
	}
}

static void select_choice(gp_widget *self, unsigned int select)
{
	if (self->choice->selected == select)
		return;

	if (select >= self->choice->max) {
		GP_BUG("Widget %p trying to select %i max %i",
			self, select, self->choice->max);
		return;
	}

	self->choice->selected = select;

	gp_widget_redraw(self);

	gp_widget_send_event(self->choice->on_event, self, self->choice->priv,
	                     GP_WIDGET_EVENT_ENTER);
}

static void key_up(gp_widget *self)
{
	if (self->choice->selected > 0)
		select_choice(self, self->choice->selected - 1);
	else
		select_choice(self, self->choice->max - 1);
}

static void key_down(gp_widget *self)
{
	if (self->choice->selected + 1 >= self->choice->max)
		select_choice(self, 0);
	else
		select_choice(self, self->choice->selected + 1);
}

static void radio_click(gp_widget *self, gp_event *ev)
{
	unsigned int min_x = self->x;
	unsigned int max_x = self->x + self->w;
	unsigned int min_y = self->y + cfg->padd;
	unsigned int max_y = self->y + self->h - cfg->padd;
	unsigned int text_h = gp_text_ascent(cfg->font) + cfg->padd;

	if (ev->cursor_x < min_x || ev->cursor_x > max_x)
		return;

	if (ev->cursor_y < min_y || ev->cursor_y > max_y)
		return;

	unsigned int select = (ev->cursor_y - min_y) / text_h;

	select_choice(self, select);
}

static int event(gp_widget *self, gp_event *ev)
{
	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code == GP_EV_KEY_UP)
			return 0;

		switch (ev->val.val) {
		case GP_BTN_LEFT:
			radio_click(self, ev);
			return 1;
		case GP_KEY_DOWN:
			key_down(self);
			return 1;
		case GP_KEY_UP:
			key_up(self);
			return 1;
		case GP_KEY_HOME:
			select_choice(self, 0);
			return 1;
		case GP_KEY_END:
			select_choice(self, self->choice->max - 1);
			return 1;
		}
	}

	return 0;
}

static gp_widget *json_to_radiobutton(json_object *json, void **uids)
{
	json_object *labels = NULL;
	const char *on_event = NULL;
	int sel_label = 0;
	void *on_event_fn = NULL;

	(void)uids;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "buttons"))
			labels = val;
		else if (!strcmp(key, "selected"))
			sel_label = json_object_get_int(val);
		else if (!strcmp(key, "on_event"))
			on_event = json_object_get_string(val);
		else
			GP_WARN("Invalid radiobutton key '%s'", key);
	}

	if (!labels) {
		GP_WARN("Missing labels array!");
		return NULL;
	}

	if (!json_object_is_type(labels, json_type_array)) {
		GP_WARN("Buttons has to be array of strings!");
		return NULL;
	}

	unsigned int i, label_cnt = json_object_array_length(labels);
	const char *labels_arr[label_cnt];

	if (sel_label < 0 || (unsigned int)sel_label >= label_cnt) {
		GP_WARN("Invalid selected button %i", sel_label);
		sel_label = 0;
	}

	for (i = 0; i < label_cnt; i++) {
		json_object *label = json_object_array_get_idx(labels, i);
		labels_arr[i] = json_object_get_string(label);

		if (!labels_arr[i])
			GP_WARN("Button %i must be string!", i);
	}

	if (on_event) {
		on_event_fn = gp_widget_callback_addr(on_event);

		if (!on_event_fn)
			GP_WARN("No on_event function '%s' defined", on_event);
	}

	return gp_widget_choice_new(labels_arr, label_cnt, sel_label, on_event_fn, NULL);
}

struct gp_widget_ops gp_widget_radio_button_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.render = render,
	.event = event,
	.from_json = json_to_radiobutton,
	.id = "radiobutton",
};

struct gp_widget *gp_widget_choice_new(const char *choices[],
                                       unsigned int choice_cnt,
                                       unsigned int selected,
				       int (*on_event)(gp_widget_event *self),
				       void *priv)
{
	size_t size = sizeof(struct gp_widget_choice)
	              + gp_string_arr_size(choices, choice_cnt);

	gp_widget *ret = gp_widget_new(GP_WIDGET_RADIOBUTTON, size);
	if (!ret)
		return NULL;


	ret->choice->selected = selected;
	ret->choice->choices = gp_string_arr_copy(choices, choice_cnt, ret->choice->payload);
	ret->choice->max = choice_cnt;
	ret->choice->on_event = on_event;
	ret->choice->priv = priv;

	gp_widget_send_event(on_event, ret, priv, GP_WIDGET_EVENT_NEW);

	return ret;
}
