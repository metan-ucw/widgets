//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_OPS_H__
#define GP_WIDGET_OPS_H__

#include <input/gp_event.h>
#include <gp_widget.h>
#include <gp_bbox.h>
#include <gp_widget_render.h>

enum gp_widget_select_flag {
	GP_SELECT_OUT,
	GP_SELECT_IN,
	GP_SELECT_LEFT,
	GP_SELECT_RIGHT,
	GP_SELECT_UP,
	GP_SELECT_DOWN,
	GP_SELECT_NEXT,
	GP_SELECT_PREV,
};

struct json_object;

typedef struct gp_offset {
	gp_coord x;
	gp_coord y;
} gp_offset;

struct gp_widget_ops {
	void (*free)(gp_widget *self);

	/**
	 * @brief Widget input event handler.
	 *
	 * @return Returns non-zero if event was handled.
	 */
	int (*event)(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev);

	/**
	 * @brief Renders (changes in) widget layout.
	 *
	 * @ctx Render configuration.
	 * @offset Ofset in pixmap to draw to
	 * @flags Force redraw whole layout.
	 */
	void (*render)(gp_widget *self, const gp_offset *offset,
	               const gp_widget_render_ctx *ctx, int flags);

	/*
	 * Moves to focused widget.
	 */
	int (*select)(gp_widget *self, int select);

	/*
	 * Moves focus to widget on x, y coordinates.
	 */
	int (*select_xy)(gp_widget *self, const gp_widget_render_ctx *ctx,
	                 unsigned int x, unsigned int y);

	/*
	 * Called once to calculate minimal widget sizes.
	 */
	unsigned int (*min_w)(gp_widget *self, const gp_widget_render_ctx *ctx);
	unsigned int (*min_h)(gp_widget *self, const gp_widget_render_ctx *ctx);

	/**
	 * @brief Recursively distributes widgets in a widget container.
	 *
	 * Implemented only for non-leaf widgets.
	 *
	 * @self   Widget layout to be distributed.
	 * @ctx    Render configuration.
	 * @new_wh Force distribute size on layout size change.
	 */
	void (*distribute_size)(gp_widget *self,
	                        const gp_widget_render_ctx *ctx,
	                        int new_wh);

	/*
	 * json_object -> widget converter.
	 */
	gp_widget *(*from_json)(struct json_object *json, void **uids);

	/* id used for debugging */
	const char *id;
};

const struct gp_widget_ops *gp_widget_ops(gp_widget *self);

const struct gp_widget_ops *gp_widget_ops_by_id(const char *id);

const char *gp_widget_type_id(gp_widget *self);

const char *gp_widget_type_name(enum gp_widget_type type);

void gp_widget_free(gp_widget *self);

unsigned int gp_widget_min_w(gp_widget *self, const gp_widget_render_ctx *ctx);

unsigned int gp_widget_min_h(gp_widget *self, const gp_widget_render_ctx *ctx);

unsigned int gp_widget_align(gp_widget *self);

int gp_widget_input_event(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev);

void gp_widget_ops_render(gp_widget *self, const gp_offset *offset,
                          const gp_widget_render_ctx *ctx, int flags);

int gp_widget_ops_event(gp_widget *self, const gp_widget_render_ctx *ctx, gp_event *ev);

int gp_widget_ops_render_select(gp_widget *self, int flag);

int gp_widget_ops_render_select_xy(gp_widget *self, const gp_widget_render_ctx *ctx,
                                   unsigned int x, unsigned int y);

void gp_widget_ops_distribute_size(gp_widget *self, const gp_widget_render_ctx *ctx,
                                   unsigned int w, unsigned int h, int new_wh);

/**
 * @brief Marks an area to be blit on the screen from a buffer.
 *
 * This function is called by widgets so that the render knows which parts of
 * the screen has to be updated after the call to the render function.
 *
 */
static inline void gp_widget_ops_blit(const gp_widget_render_ctx *ctx,
                                      gp_coord x, gp_coord y,
                                      gp_size w, gp_size h)
{
	if (!ctx->flip)
		return;

	if (gp_bbox_empty(*ctx->flip))
		*ctx->flip = gp_bbox_pack(x, y, w, h);
	else
		*ctx->flip = gp_bbox_merge(*ctx->flip, gp_bbox_pack(x, y, w, h));
}

static inline int gp_widget_should_redraw(gp_widget *self, int flags)
{
	return self->redraw || flags;
}

/**
 * @brief Calculates layout size recursively.
 *
 * The size may end up larger than WxH if there is too much widgets or smaller
 * than WxH if align is not set to fill.
 *
 * @layout Widget layout.
 * @ctx    Render context, e.g. fonts, pixel type, padding size, etc.
 * @w      Width we are trying to fit into
 * @h      Height we are trying to fit into
 * @new_wh Force to position widgets on changed layout size
 */
void gp_widget_calc_size(gp_widget *layout, const gp_widget_render_ctx *ctx,
                         unsigned int w, unsigned int h, int new_wh);

/*
 * Marks widget to be resized, redrawn on next render event.
 */
void gp_widget_redraw(gp_widget *self);
void gp_widget_resize(gp_widget *self);

/*
 * Resizes and redraws changed widgets.
 */
void gp_widget_render(gp_widget *self, const gp_widget_render_ctx *ctx, int new_wh);

#endif /* GP_WIDGET_OPS_H__ */
