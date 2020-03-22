//SPDX-License-Identifier: LGPL-2.0-or-later
/*

   Copyright (C) 2007-2020 Cyril Hrubis <metan@ucw.cz>

 */

#include <gp_widgets.h>
#include <mupdf/fitz.h>

struct document {
	int page_count;
	int cur_page;
	fz_context *fz_ctx;
	fz_document *fz_doc;
	fz_page *fz_pg;
};

static struct controls {
	gp_widget *page;
	gp_widget *pg_cnt;
	gp_widget *pg_nr;
	struct document *doc;
} controls;

static void draw_page(gp_widget_event *ev)
{
	gp_widget *self = ev->self;
	gp_pixmap *pixmap = self->pixmap->pixmap;

	GP_DEBUG(1, "Redrawing canvas %ux%u", pixmap->w, pixmap->h);

	struct document *doc = controls.doc;

	if (!doc->fz_ctx) {
		gp_fill(pixmap, ev->ctx->fg_color);
		return;
	}

	// Determines page size at 72 DPI
	fz_rect rect = fz_bound_page(doc->fz_ctx, doc->fz_pg);

	GP_DEBUG(1, "Page bounding box %fx%f - %fx%f",
	         rect.x0, rect.y0, rect.x1, rect.y1);

	// Find ZOOM fit size
	float x_rat = 1.00 * pixmap->w / (rect.x1 - rect.x0);
	float y_rat = 1.00 * pixmap->h / (rect.y1 - rect.y0);
	float rat = GP_MIN(x_rat, y_rat);

	fz_matrix transform = fz_scale(rat, rat);

	fz_pixmap *pix;

	pix = fz_new_pixmap_from_page_number(doc->fz_ctx, doc->fz_doc, doc->cur_page,
	                                     transform, fz_device_bgr(doc->fz_ctx), 0);

	// Blit the pixmap
	GP_DEBUG(1, "Blitting context");
	gp_pixmap page;
	//TODO: Fill only the corners
	gp_fill(pixmap, ev->ctx->bg_color);

	gp_pixmap_init(&page, pix->w, pix->h, GP_PIXEL_RGB888, pix->samples);

	uint32_t cx = (pixmap->w - page.w)/2;
	uint32_t cy = (pixmap->h - page.h)/2;

	gp_blit(&page, 0, 0, page.w, page.h, pixmap, cx, cy);

	fz_drop_pixmap(doc->fz_ctx, pix);
}

static void load_page(struct document *doc, int page)
{
	if (page < 0 || page >= doc->page_count) {
		GP_WARN("Page %i out of max pages %i",
		         page, doc->page_count);
		return;
	}

	if (doc->cur_page != -1)
		fz_drop_page(doc->fz_ctx, doc->fz_pg);

	doc->cur_page = page;
	doc->fz_pg = fz_load_page(doc->fz_ctx, doc->fz_doc, doc->cur_page);

	gp_widget_textbox_printf(controls.pg_nr, "%i", doc->cur_page+1);
}

static int load_document(struct document *doc, const char *filename)
{
	fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);

	if (!ctx)
		return 1;

	fz_register_document_handlers(ctx);

	doc->fz_ctx = ctx;
	doc->fz_doc = fz_open_document(ctx, (char*)filename);
	doc->page_count = fz_count_pages(ctx, doc->fz_doc);
	doc->cur_page = -1;

	load_page(doc, 0);

	return 0;
}

static void load_next_page(struct document *doc, int i)
{
	if (doc->cur_page + i < 0 || doc->cur_page + i >= doc->page_count) {
		GP_DEBUG(1, "No next/prev page.");
		return;
	}

	load_page(doc, doc->cur_page + i);
}

static void load_and_redraw(struct document *doc, int i)
{
	load_next_page(doc, i);
	gp_widget_redraw(controls.page);
	gp_widget_pixmap_update(controls.page);
}

int load_page_event(gp_widget_event *ev)
{
	gp_widget *tbox = ev->self;

	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	load_page(controls.doc, atoi(tbox->tbox->buf)-1);
	gp_widget_redraw(controls.page);
	gp_widget_pixmap_update(controls.page);
	return 1;
}

int button_prev_event(gp_widget_event *ev)
{
	if (ev->type == GP_WIDGET_EVENT_ACTION)
		load_and_redraw(controls.doc, -1);

	return 0;
}

int button_next_event(gp_widget_event *ev)
{
	if (ev->type == GP_WIDGET_EVENT_ACTION)
		load_and_redraw(controls.doc, 1);

	return 0;
}

static void allocate_backing_pixmap(gp_widget_event *ev)
{
	gp_widget *w = ev->self;

	gp_pixmap_free(w->pixmap->pixmap);

	w->pixmap->pixmap = gp_pixmap_alloc(w->w, w->h, ev->ctx->pixel_type);
}

int pixmap_on_event(gp_widget_event *ev)
{
	gp_widget_event_dump(ev);

	switch (ev->type) {
	case GP_WIDGET_EVENT_RESIZE:
		allocate_backing_pixmap(ev);
		draw_page(ev);
	break;
	case GP_WIDGET_EVENT_REDRAW:
		draw_page(ev);
	break;
	default:
	break;
	}

	return 0;
}

/*
static void canvas_callback(struct MW_Widget *self, GP_Event *ev)
{
	struct document *doc = self->callback_priv;

	switch (ev->type) {
	case GP_EV_KEY:
		if (ev->code == GP_EV_KEY_UP)
			return;

		switch (ev->val.key.key) {
		case GP_KEY_SPACE:
		case GP_KEY_ENTER:
		case GP_KEY_RIGHT:
		case GP_KEY_UP:
			load_and_redraw(doc, 1);
		break;
		case GP_KEY_LEFT:
		case GP_KEY_DOWN:
		case GP_KEY_BACKSPACE:
			load_and_redraw(doc, -1);
		break;
		case GP_KEY_F:
			toggle_fullscreen();
		break;
		}
	break;
	default:
	break;
	}
}
*/

int main(int argc, char *argv[])
{
	void *uids;
	struct document doc = {};

	gp_widgets_getopt(&argc, &argv);

	if (argc && load_document(&doc, argv[0])) {
		GP_WARN("Can't load document '%s'", argv[0]);
		return 1;
	}

	gp_widget *layout = gp_widget_layout_json("gppdf.json", &uids);

	controls.doc = &doc;
	controls.page = gp_widget_by_uid(uids, "page", GP_WIDGET_PIXMAP);
	controls.pg_cnt = gp_widget_by_uid(uids, "pg_cnt", GP_WIDGET_LABEL);
	controls.pg_nr = gp_widget_by_uid(uids, "pg_nr", GP_WIDGET_TEXTBOX);

	gp_widget_label_printf(controls.pg_cnt, "of %i", doc.page_count);

	gp_widget_event_unmask(controls.page, GP_WIDGET_EVENT_RESIZE);
	gp_widget_event_unmask(controls.page, GP_WIDGET_EVENT_REDRAW);

	controls.page->on_event = pixmap_on_event;

	gp_widgets_main_loop(layout, "gppdf", NULL, 0, NULL);

	return 0;
}
