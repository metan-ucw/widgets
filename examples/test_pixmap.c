//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <gp_widgets.h>

static void *uids;

static gp_pixel fg_rgb;
static gp_pixel bg_rgb;

static void draw(gp_widget *pixmap, gp_event *ev)
{
	if (ev->type != GP_EV_KEY || ev->val.key.key != GP_BTN_LEFT)
		return;

	gp_pixmap *p = pixmap->pixmap->pixmap;

	gp_pixel col = gp_rgb_to_pixmap_pixel((fg_rgb >> 16) & 0xff, (fg_rgb >> 8) & 0xff, fg_rgb & 0xff, p);

	gp_putpixel(p, ev->cursor_x, ev->cursor_y, col);
	gp_circle(p, ev->cursor_x, ev->cursor_y, 10, col);
	gp_widget_redraw(pixmap);
}

static void fill_pixmap(gp_pixmap *p)
{
	gp_pixel col = gp_rgb_to_pixmap_pixel((bg_rgb >> 16) & 0xff, (bg_rgb >> 8) & 0xff, bg_rgb & 0xff, p);
	gp_fill(p, col);
}

int pixmap_on_event(gp_widget_event *ev)
{
	gp_widget_event_dump(ev);

	switch (ev->type) {
	case GP_WIDGET_EVENT_INPUT:
		draw(ev->self, ev->input_ev);
	break;
	case GP_WIDGET_EVENT_REDRAW:
		fill_pixmap(ev->self->pixmap->pixmap);
	break;
	default:
	break;
	}

	return 0;
}

int set_fg_color(gp_widget_event *ev)
{
	gp_widget_event_dump(ev);

	switch (ev->type) {
	case GP_WIDGET_EVENT_NEW:
		ev->self->tbox->filter = GP_TEXT_BOX_FILTER_HEX;
		/* fall through */
	case GP_WIDGET_EVENT_EDIT:
		fg_rgb = strtol(ev->self->tbox->buf, NULL, 16);
		printf("fg_color = 0x%x\n", fg_rgb);
	break;
	default:
	break;
	}

	return 0;
}

int set_bg_color(gp_widget_event *ev)
{
	gp_widget *pixmap;

	gp_widget_event_dump(ev);

	switch (ev->type) {
	case GP_WIDGET_EVENT_NEW:
		ev->self->tbox->filter = GP_TEXT_BOX_FILTER_HEX;
	/* fall through */
	case GP_WIDGET_EVENT_EDIT:
		bg_rgb = strtol(ev->self->tbox->buf, NULL, 16);
		printf("bg_color = 0x%06x\n", bg_rgb);
	break;
	case GP_WIDGET_EVENT_ACTION:
		pixmap = gp_widget_by_uid(uids, "pixmap", GP_WIDGET_PIXMAP);

		if (pixmap) {
			fill_pixmap(pixmap->pixmap->pixmap);
			gp_widget_redraw(pixmap);
		}
	break;
	default:
	break;
	}

	return 0;
}

int button_on_event(gp_widget_event *ev)
{
	gp_widget_event_dump(ev);

	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	gp_widget *pixmap = gp_widget_by_uid(uids, "pixmap", GP_WIDGET_PIXMAP);

	(void)ev;

	if (pixmap) {
		fill_pixmap(pixmap->pixmap->pixmap);
		gp_widget_redraw(pixmap);
	}

	return 0;
}

int main(void)
{
	gp_widget *layout = gp_widget_layout_json("test_pixmap.json", &uids);
	if (!layout)
		return 0;

	gp_widgets_main_loop(layout, "Pixmap test", NULL);

	return 0;
}
