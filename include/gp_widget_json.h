//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef GP_WIDGET_JSON_H__
#define GP_WIDGET_JSON_H__

#include <gp_widget.h>

struct json_object;

gp_widget *gp_widget_from_json(struct json_object *json, void **uids);

gp_widget *gp_widget_layout_json(const char *fname, void **uids);

void *gp_widget_callback_addr(const char *fn_name);

gp_widget *gp_widget_by_uid(void *uids, const char *id, enum gp_widget_type type);

#endif /* GP_WIDGET_H__ */
