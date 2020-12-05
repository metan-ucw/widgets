//SPDX-License-Identifier: LGPL-2.0-or-later
/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_OVERLAY_H__
#define GP_WIDGET_OVERLAY_H__

struct gp_widget_overlay_elem {
	int hidden:1;
	struct gp_widget *widget;
};

struct gp_widget_overlay {
	int selected;
	struct gp_widget_overlay_elem *stack;
};

/*
 * @brief
 */
gp_widget *gp_widget_overlay_new(unsigned int stack_size);

void gp_widget_overlay_hide(gp_widget *self, unsigned int stack_pos);

void gp_widget_overlay_show(gp_widget *self, unsigned int stack_pos);

unsigned int gp_widget_overlay_widgets(gp_widget *self);

/* @brief Sets a switch layout.
 *
 * @self A switch widget.
 * @layout_nr Number of layout to set.
 * @layout A layout to set.
 * @return Previous layout occupying the slot.
 */
gp_widget *gp_widget_overlay_set(gp_widget *self, unsigned int stack_pos,
                                 gp_widget *widget);

#endif /* GP_WIDGET_OVERLAY_H__ */
