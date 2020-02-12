//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_H__
#define GP_WIDGET_H__

#include <stdlib.h>
#include <stdint.h>
#include <gp_common.h>

struct gp_widget_event;

typedef struct gp_widget {
	unsigned int type;
	struct gp_widget *parent;

	/*
	 * Widget event handler.
	 */
	int (*on_event)(struct gp_widget_event *);

	/*
	 * User provided pointer to arbitrary data; useful for event handlers
	 * and other user defined functions. The library will not access or
	 * modify the memory pointed to by this.
	 */
	void *priv;

	/*
	 * Relative offset to the parent widget.
	 */
	unsigned int x, y;

	/*
	 * Current widget size.
	 */
	unsigned int w, h;

	/*
	 * Cached widget minimal size.
	 */
	unsigned int min_w, min_h;

	unsigned int align:16;
	unsigned int no_resize:1;
	unsigned int no_redraw:1;
	unsigned int no_redraw_child:1;
	unsigned int redraw_subtree:1;
	unsigned int selected:1;
	unsigned int input_events:1;

	uint32_t event_mask;

	union {
		struct gp_widget_grid *grid;
		struct gp_widget_tabs *tabs;

		struct gp_widget_bool *b;
		struct gp_widget_bool *btn;
		struct gp_widget_bool *chbox;

		struct gp_widget_label *label;

		struct gp_widget_int *i;
		struct gp_widget_pbar *pbar;
		struct gp_widget_int *spin;
		struct gp_widget_int *slider;

		struct gp_widget_misc *misc;

		struct gp_widget_textbox *tbox;

		struct gp_widget_choice *choice;

		struct gp_widget_table *tbl;

		struct gp_widget_pixmap *pixmap;

		void *payload;
	};
	char buf[];
} gp_widget;

enum gp_widget_type {
	GP_WIDGET_GRID,
	GP_WIDGET_TABS,
	GP_WIDGET_MISC,
	GP_WIDGET_BUTTON,
	GP_WIDGET_CHECKBOX,
	GP_WIDGET_LABEL,
	GP_WIDGET_PROGRESSBAR,
	GP_WIDGET_SPINNER,
	GP_WIDGET_SLIDER,
	GP_WIDGET_TEXTBOX,
	GP_WIDGET_RADIOBUTTON,
	GP_WIDGET_TABLE,
	GP_WIDGET_PIXMAP,
	GP_WIDGET_MAX,
};

enum gp_widget_alignment {
	/** Default overridable alignment. */
	GP_HCENTER_WEAK = 0x00,
	GP_HCENTER = 0x01,
	GP_LEFT    = 0x02,
	GP_RIGHT   = 0x03,
	GP_HFILL   = 0x08,
	/** Default overridable alignment. */
	GP_VCENTER_WEAK = 0x00,
	GP_VCENTER = 0x10,
	GP_TOP     = 0x20,
	GP_BOTTOM  = 0x30,
	GP_VFILL   = 0x80,
};

#define GP_FILL (GP_VFILL | GP_HFILL)

#define GP_HALIGN_MASK 0x0f
#define GP_VALIGN_MASK 0xf0

/*
 * Allocates widget structure and initializes the payload pointer.
 */
gp_widget *gp_widget_new(enum gp_widget_type type, size_t payload_size);

#define GP_WIDGET_ASSERT(self, wtype, ret) do { \
		if (!self) {\
			GP_BUG("NULL widget!"); \
			return ret; \
		} else if (self->type != wtype) {\
			GP_BUG("Invalid widget type %s != %s", \
				gp_widget_type_id(self), gp_widget_type_name(wtype)); \
			return ret; \
		} \
	} while (0)

#include <gp_widget_event.h>

#endif /* GP_WIDGET_H__ */
