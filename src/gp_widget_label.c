//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>

static unsigned int min_w(gp_widget *self)
{
	gp_text_style *font = self->label->bold ? cfg->font_bold : cfg->font;
	unsigned int max_width;

	if (self->label->width)
		max_width = gp_text_max_width(font, self->label->width);
	else
		max_width = gp_text_width(font, self->label->text);

	return max_width;
}

static unsigned int min_h(gp_widget *self)
{
	(void)self;
	return 2 * cfg->padd + gp_text_ascent(cfg->font);
}

static void render(gp_widget *self,
                   struct gp_widget_render *render, int flags)
{
	(void) flags;

	gp_text_style *font = self->label->bold ? cfg->font_bold : cfg->font;

	gp_fill_rect_xywh(render->buf, self->x, self->y,
	                self->w, self->h, cfg->bg_color);

	gp_text(render->buf, font, self->x, self->y + cfg->padd,
		GP_ALIGN_RIGHT|GP_VALIGN_BELOW,
		cfg->text_color, cfg->bg_color, self->label->text);
}

static gp_widget *json_to_label(json_object *json, void **uids)
{
	const char *label = NULL;
	int bold = 0;

	(void)uids;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "label"))
			label = json_object_get_string(val);
		else if (!strcmp(key, "bold"))
			bold = json_object_get_boolean(val);
		else
			GP_WARN("Invalid label key '%s'", key);
	}

	if (!label) {
		GP_WARN("Missing label");
		label = "Missing label";
	}

	return gp_widget_label_new(label, 0, bold);
}

struct gp_widget_ops gp_widget_label_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.render = render,
	.from_json = json_to_label,
	.id = "label",
};

static void copy_text(struct gp_widget_label *label, const char *text, size_t len)
{
	strncpy(label->text, text, len);

	if (label->text[len - 1]) {
		GP_WARN("Truncating label text '%s'", text);
		label->text[len - 1] = '\0';
	}
}

char *gp_widget_label_set(gp_widget *self, char *text)
{
	struct gp_widget_label *label = self->label;

	GP_WIDGET_ASSERT(self, GP_WIDGET_LABEL, NULL);

	char *ret = label->text;

	GP_DEBUG(3, "Setting widget label (%p) text '%s' -> '%s'",
		 self, ret, text);

	if (label->width)
		copy_text(label, text, label->width);
	else
		label->text = text;

	if (label->width)
		gp_widget_redraw(self);
	else
		gp_widget_resize(self);

	return ret;
}

gp_widget *gp_widget_label_new(const char *text, unsigned int size, int bold)
{
	size_t payload_size = sizeof(struct gp_widget_label);
	gp_widget *ret;
	size_t strsize = size ? size : strlen(text)+1;

	payload_size += strsize;

	ret = gp_widget_new(GP_WIDGET_LABEL, payload_size);
	if (!ret)
		return NULL;

	ret->label->text = ret->label->payload;
	ret->label->bold = !!bold;

	if (text)
		copy_text(ret->label, text, strsize);

	return ret;
}

gp_widget *gp_widget_label_printf_new(int bold, const char *fmt, ...)
{
	va_list ap;
	size_t len;

	va_start(ap, fmt);
	len = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

	gp_widget *ret = gp_widget_label_new(NULL, len+1, bold);

	va_start(ap, fmt);
	vsnprintf(ret->label->text, len+1, fmt, ap);
	va_end(ap);

	return ret;
}
