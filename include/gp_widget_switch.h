//SPDX-License-Identifier: LGPL-2.0-or-later
/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_SWITCH_H__
#define GP_WIDGET_SWITCH_H__

struct gp_widget_switch {
	unsigned int active_layout;
	gp_widget **layouts;
};

/*
 * @brief Allocates a switch widget.
 *
 * @layouts Number of layouts to switch between.
 * @return Newly allocated switch widget.
 */
gp_widget *gp_widget_switch_new(unsigned int layouts);

/*
 * @brief Returns number of switch layouts.
 *
 * @self A switch widget.
 * @return Number of switch layouts.
 */
unsigned int gp_widget_switch_layouts(gp_widget *self);

/* @brief Switches to a different layout.
 *
 * @self A switch widget.
 * @layout_nr Number of layout to switch to.
 */
void gp_widget_switch_layout(gp_widget *self, unsigned int layout_nr);

/* @brief Moves layout by where.
 *
 * @self A switch widget.
 * @where How much we should move.
 */
void gp_widget_switch_move(gp_widget *self, int where);

/*
 * @brief Returns a pointer to active layout.
 *
 * @self A switch widget.
 * @return A pointer to active layout.
 */
gp_widget *gp_widget_switch_active(gp_widget *self);

/* @brief Sets a switch layout.
 *
 * @self A switch widget.
 * @layout_nr Number of layout to set.
 * @layout A layout to set.
 * @return Previous layout occupying the slot.
 */
gp_widget *gp_widget_switch_set(gp_widget *self, unsigned int layout_nr,
                                gp_widget *layout);

#endif /* GP_WIDGET_SWITCH_H__ */
