//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2019 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_MISC_H__
#define GP_WIDGET_MISC_H__

#include <gp_widget_ops.h>

struct gp_widget_misc {
	struct gp_widget_ops *ops;
	void *priv;
};

#endif /* GP_WIDGET_MISC_H__ */
