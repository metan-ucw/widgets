//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <json-c/json.h>

#include <utils/gp_vec_str.h>

#include <gp_widgets.h>
#include <gp_widget_ops.h>
#include <gp_widget_render.h>

static const gp_text_style *label_font(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	return self->label->bold ? ctx->font_bold : ctx->font;
}

static unsigned int min_w(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	unsigned int max_width;
	const gp_text_style *font = label_font(self, ctx);

	if (self->label->width)
		max_width = gp_text_max_width_chars(font, self->label->set, self->label->width);
	else
		max_width = gp_text_width(font, self->label->text);

	if (self->label->frame)
		max_width += 2 * ctx->padd;

	return max_width;
}

static unsigned int min_h(gp_widget *self, const gp_widget_render_ctx *ctx)
{
	(void) self;
	return 2 * ctx->padd + gp_text_ascent(ctx->font);
}

static void render(gp_widget *self, const gp_offset *offset,
                   const gp_widget_render_ctx *ctx, int flags)
{
	(void) flags;
	unsigned int align;

	unsigned int x = self->x + offset->x;
	unsigned int y = self->y + offset->y;
	unsigned int w = self->w;
	unsigned int h = self->h;

	gp_widget_ops_blit(ctx, x, y, w, h);

	gp_text_style *font = self->label->bold ? ctx->font_bold : ctx->font;

	if (self->label->frame) {
		gp_fill_rrect_xywh(ctx->buf, x, y, w, h, ctx->bg_color,
		                   ctx->fg_color, ctx->text_color);

		x += ctx->padd;
		w -= 2 * ctx->padd;
	} else {
		gp_fill_rect_xywh(ctx->buf, x, y, w, h, ctx->bg_color);
	}

	if (self->label->ralign) {
		x += w - 1;
		align = GP_ALIGN_LEFT;
	} else {
		align = GP_ALIGN_RIGHT;
	}

	gp_text(ctx->buf, font, x, y + ctx->padd,
		align|GP_VALIGN_BELOW,
		ctx->text_color, ctx->bg_color, self->label->text);
}

static gp_widget *json_to_label(json_object *json, void **uids)
{
	const char *label = NULL;
	int bold = 0;
	int size = 0;
	int ralign = 0;
	int frame = 0;

	(void)uids;

	json_object_object_foreach(json, key, val) {
		if (!strcmp(key, "text"))
			label = json_object_get_string(val);
		else if (!strcmp(key, "bold"))
			bold = json_object_get_boolean(val);
		else if (!strcmp(key, "size"))
			size = json_object_get_int(val);
		else if (!strcmp(key, "ralign"))
			ralign = json_object_get_boolean(val);
		else if (!strcmp(key, "frame"))
			frame = json_object_get_boolean(val);
		else
			GP_WARN("Invalid label key '%s'", key);
	}

	if (!label) {
		GP_WARN("Missing label");
		label = "Missing label";
	}

	gp_widget *ret = gp_widget_label_new(label, size, bold);

	ret->label->ralign = ralign;
	ret->label->frame = frame;

	return ret;
}

struct gp_widget_ops gp_widget_label_ops = {
	.min_w = min_w,
	.min_h = min_h,
	.render = render,
	.from_json = json_to_label,
	.id = "label",
};

static void try_resize(gp_widget *self)
{
	const gp_widget_render_ctx *ctx = gp_widgets_render_ctx();

	if (self->min_w < min_w(self, ctx)) {
		GP_DEBUG(0, "Resizing widget label (%p) %i -> %i",
		         self, self->min_w, min_w(self, ctx));
		gp_widget_resize(self);
	}
}

/*
 * Do not shrink by default so that the layout will not jump up and down all
 * the time.
 */
void gp_widget_label_set(gp_widget *self, const char *text)
{
	GP_WIDGET_ASSERT(self, GP_WIDGET_LABEL, );

	GP_DEBUG(3, "Setting widget label (%p) text '%s' -> '%s'",
		 self, self->label->text, text);

	self->label->text = gp_vec_printf(self->label->text, "%s", text);

	gp_widget_redraw(self);

	if (self->label->width)
		return;

	try_resize(self);
}

gp_widget *gp_widget_label_new(const char *text, unsigned int width, int bold)
{
	gp_widget *ret;

	ret = gp_widget_new(GP_WIDGET_LABEL, sizeof(struct gp_widget_label));
	if (!ret)
		return NULL;

	if (text)
		ret->label->text = gp_vec_strdup(text);
	else
		ret->label->text = gp_vec_str_new();

	ret->label->bold = !!bold;
	ret->label->width = width;

	return ret;
}

static char *vasprintf(const char *fmt, va_list ap)
{
	va_list ac;
	size_t len;

	va_copy(ac, ap);
	len = vsnprintf(NULL, 0, fmt, ac);
	va_end(ac);

	char *buf = malloc(len+1);
	if (!buf) {
		GP_WARN("Malloc failed :(");
		return NULL;
	}

	vsnprintf(buf, len+1, fmt, ap);

	return buf;
}

gp_widget *gp_widget_label_printf_new(int bold, const char *fmt, ...)
{
	va_list ap;
	char *buf;

	va_start(ap, fmt);
	buf = vasprintf(fmt, ap);
	va_end(ap);

	gp_widget *ret = gp_widget_label_new(buf, 0, bold);

	free(buf);

	return ret;
}

int gp_widget_label_printf(gp_widget *self, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	self->label->text = gp_vec_vprintf(self->label->text, fmt, ap);
	va_end(ap);

	try_resize(self);

	return 0;
}
