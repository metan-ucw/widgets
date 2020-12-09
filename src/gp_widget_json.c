//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#include <json-c/json.h>

#include <gp_widget.h>
#include <gp_widgets.h>
#include <gp_widget_ops.h>

gp_widget *gp_widget_from_json(json_object *json, void **uids)
{
	json_object *json_type;
	const char *type = "grid";
	const char *uid;
	char *uid_key = NULL;
	unsigned int halign = 0;
	unsigned int valign = 0;
	int (*on_event)(gp_widget_event *) = NULL;

	if (json_object_object_length(json) == 0)
		return NULL;

	if (json_object_object_get_ex(json, "call", &json_type)) {
		const char *func_name = json_object_get_string(json_type);

		if (!func_name) {
			GP_WARN("Invalid call");
			return NULL;
		}

		gp_widget *(*func)(void) = gp_widget_callback_addr(func_name);

		if (!func) {
			GP_WARN("Function call '%s' does not exist!", func_name);
			return NULL;
		}

		GP_DEBUG(1, "Calling '%s'", func_name);

		return func();
	}

	if (json_object_object_get_ex(json, "type", &json_type)) {
		type = json_object_get_string(json_type);

		if (!type) {
			GP_WARN("Invalid type");
			return NULL;
		}
	}

	if (json_object_object_get_ex(json, "uid", &json_type)) {
		uid = json_object_get_string(json_type);

		if (!uid) {
			GP_WARN("Invalid uid");
			return NULL;
		}

		if (uids)
			uid_key = strdup(uid);

		GP_DEBUG(2, "Widget '%s' uid '%s'", type, uid);

		json_object_object_del(json, "uid");
	}

	if (json_object_object_get_ex(json, "align", &json_type)) {
		const char *align_str = json_object_get_string(json_type);

		if (!strcmp(align_str, "center")) {
			halign = GP_HCENTER;
			valign = GP_VCENTER;
		} else if (!strcmp(align_str, "fill")) {
			halign = GP_HFILL;
			valign = GP_VFILL;
		} else if (!strcmp(align_str, "hfill")) {
			halign = GP_HFILL;
		} else if (!strcmp(align_str, "vfill")) {
			valign = GP_VFILL;
		} else {
			GP_WARN("Invalid align=%s.", align_str);
		}

		json_object_object_del(json, "align");
	}

	if (json_object_object_get_ex(json, "halign", &json_type)) {
		const char *halign_str = json_object_get_string(json_type);

		if (halign)
			GP_WARN("Only one of halign and align can be defined!");

		if (!strcmp(halign_str, "center"))
			halign = GP_HCENTER;
		else if (!strcmp(halign_str, "left"))
			halign = GP_LEFT;
		else if (!strcmp(halign_str, "right"))
			halign = GP_RIGHT;
		else if (!strcmp(halign_str, "fill"))
			halign = GP_HFILL;
		else
			GP_WARN("Invalid halign=%s.", halign_str);

		json_object_object_del(json, "halign");
	}

	if (json_object_object_get_ex(json, "valign", &json_type)) {
		const char *valign_str = json_object_get_string(json_type);

		if (valign)
			GP_WARN("Only one of valign and align can be defined!");

		if (!strcmp(valign_str, "center"))
			valign = GP_VCENTER;
		else if (!strcmp(valign_str, "top"))
			valign = GP_TOP;
		else if (!strcmp(valign_str, "bottom"))
			valign = GP_BOTTOM;
		else if (!strcmp(valign_str, "fill"))
			valign = GP_VFILL;
		else
			GP_WARN("Invalid valign=%s.", valign_str);

		json_object_object_del(json, "valign");
	}

	if (json_object_object_get_ex(json, "on_event", &json_type)) {
		const char *on_event_str = json_object_get_string(json_type);

		on_event = gp_widget_callback_addr(on_event_str);

		if (!on_event)
			GP_WARN("No on_event function '%s' defined", on_event_str);

		json_object_object_del(json, "on_event");
	}

	const struct gp_widget_ops *ops = gp_widget_ops_by_id(type);

	if (!ops) {
		GP_WARN("Invalid widget type '%s'", type);
		goto err;
	}

	if (!ops->from_json) {
		GP_WARN("Unimplemented from_json for widget '%s'", type);
		goto err;
	}

	json_object_object_del(json, "type");

	gp_widget *wid = ops->from_json(json, uids);
	if (!wid)
		goto err;

	if (uid_key) {
		if (!*uids) {
			*uids = gp_htable_new(0, 0);
			if (!*uids)
				goto err;
		}

		gp_htable_put(*uids, wid, uid_key);
	}

	wid->align = halign | valign;

	wid->on_event = on_event;

	gp_widget_send_event(wid, GP_WIDGET_EVENT_NEW);

	return wid;
err:
	free(uid_key);
	return NULL;
}

static void *ld_handle;

void *gp_widget_callback_addr(const char *fn_name)
{
	if (!ld_handle)
		return NULL;

	dlerror();

	void *addr = dlsym(ld_handle, fn_name);

	GP_DEBUG(3, "Function '%s' address is %p", fn_name, addr);

	const char *err = dlerror();

	if (err)
		GP_WARN("%s", err);

	return addr;
}

static gp_widget *gp_widgets_from_json(json_object *json, void **uids)
{
	gp_widget *ret;

	ld_handle = dlopen(NULL, RTLD_LAZY);

	if (!ld_handle)
		GP_WARN("Failed to dlopen()");

	ret = gp_widget_from_json(json, uids);

	dlclose(ld_handle);

	ld_handle = NULL;

	return ret;
}

static void print_err(const char *path, const char *err, size_t bytes_consumed)
{
	FILE *f;
	char line[2048];
	size_t off = 0, lineno = 0, errlineno = 0;

	f = fopen(path, "r");
	if (!f)
		goto err;

	for (;;) {
		if (!fgets(line, sizeof(line), f))
			goto err;

		off += strlen(line);
		errlineno++;

		if (off >= bytes_consumed)
			break;
	}

	rewind(f);

	fprintf(stderr, "%s:\n\n", path);

	for (;;) {
		if (!fgets(line, sizeof(line), f))
			goto err;

		lineno++;

		if (lineno + 5 > errlineno)
			fprintf(stderr, "%3zu: %s", lineno, line);

		if (lineno == errlineno)
			break;
	}

	fprintf(stderr, "\nerror: %zu: %s\n\n", errlineno, err);
	return;
err:
	fclose(f);
	GP_WARN("json_tokener_parse_ex(): %s: %zu", err, bytes_consumed);
}

gp_widget *gp_widget_layout_json(const char *path, void **uids)
{
	struct json_tokener *tok;
	int fd;
	json_object *json = NULL;
	char buf[2048];
	size_t size, bytes_consumed = 0;
	gp_widget *ret = NULL;

	if (uids)
		*uids = NULL;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		GP_WARN("Failed to open '%s': %s", path, strerror(errno));
		goto err;
	}

	tok = json_tokener_new();
	if (!tok) {
		GP_WARN("json_tokener_new() failed :-(");
		goto err1;
	}

	while ((size = read(fd, buf, sizeof(buf))) > 0) {
		enum json_tokener_error err;
		json = json_tokener_parse_ex(tok, buf, size);
		err = json_tokener_get_error(tok);

		bytes_consumed += tok->char_offset;

		switch (err) {
		case json_tokener_continue:
		break;
		case json_tokener_success:
			if ((size_t)tok->char_offset != size) {
				print_err(path, "garbage after end", bytes_consumed + 1);
				goto err2;
			}

			goto done;
		default:
			print_err(path, json_tokener_error_desc(err), bytes_consumed);
			goto err2;
		}
	}
done:
	ret = gp_widgets_from_json(json, uids);
err2:
	if (json)
		json_object_put(json);

	json_tokener_free(tok);
err1:
	close(fd);
err:
	return ret;
}

gp_widget *gp_widget_by_uid(void *uids, const char *uid, enum gp_widget_type type)
{
	gp_widget *ret = gp_htable_get(uids, uid);

	if (!ret)
		return NULL;

	if (ret->type != type) {
		GP_WARN("Unexpected widget (uid='%s') type %i expected %i",
		        uid, ret->type, type);
		return NULL;
	}

	return ret;
}
