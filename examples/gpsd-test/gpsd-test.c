//SPDX-License-Identifier: LGPL-2.0-or-later

/*

   Copyright (c) 2014-2020 Cyril Hrubis <metan@ucw.cz>

 */

#include <string.h>
#include <errno.h>

#include <gfxprim.h>
#include <gp_widgets.h>
#include <gps.h>

static struct gps_data_t gpsdata;

static gp_widget *table, *gps_time, *gps_lat, *gps_lon, *gps_alt, *gps_epx, *gps_epy, *gps_epv;

static gp_widget *gps_speed, *gps_track, *gps_climb, *gps_eps, *gps_ept, *gps_epc;

static gp_widget *server_host, *server_port, *server_status;

static const char *prn_to_str(int prn)
{
	switch (prn) {
	/* US GPS slots */
	case 1 ... 64:
		return "US";
	/* GLONASS slots */
	case 65 ... 95:
	/* SDCM part of GLONASS */
	case 125:
	case 140:
	case 141:
		return "RU";
	/* EGNOS slots */
	case 120:
	case 121:
	case 123:
	case 124:
	case 126:
	case 136:
		return "EU";
	default:
		return "??";
	}
}

const char *sats_get_elem(gp_widget *self, unsigned int col)
{
	static char buf[100];
	struct satellite_t *sat = &gpsdata.skyview[self->tbl->row_idx];

	switch (col) {
	case 0:
		snprintf(buf, sizeof(buf), "%s %i", prn_to_str(sat->PRN), sat->PRN);
	break;
	case 1:
#if GPSD_API_MAJOR_VERSION >= 10
		snprintf(buf, sizeof(buf), "%.0f %.0f", sat->elevation, sat->azimuth);
#else
		snprintf(buf, sizeof(buf), "%i %i", sat->elevation, sat->azimuth);
#endif
	break;
	case 2:
		snprintf(buf, sizeof(buf), "%.1f", sat->ss);
	break;
	}

	return buf;
}

int sats_set_row(gp_widget *self, int op, unsigned int pos)
{
	switch (op) {
	case GP_TABLE_ROW_RESET:
		self->tbl->row_idx = 0;
	break;
	case GP_TABLE_ROW_ADVANCE:
		self->tbl->row_idx += pos;
	break;
	}

	if (self->tbl->row_idx < (unsigned long)gpsdata.satellites_visible)
		return 1;

	return 0;
}

static int sat_sort_col = -1;
static int sat_sort_desc = 1;

void sats_sort(gp_widget *self, unsigned int col, int desc)
{
	sat_sort_col = col;
	sat_sort_desc = desc;
}

int sat_cmp_signal_desc(const void *sat1, const void *sat2)
{
	return ((struct satellite_t*)sat1)->ss < ((struct satellite_t*)sat2)->ss;
}

int sat_cmp_signal_asc(const void *sat1, const void *sat2)
{
	return ((struct satellite_t*)sat1)->ss > ((struct satellite_t*)sat2)->ss;
}

static void do_sort(void)
{
	void *cmp = NULL;

	switch (sat_sort_col) {
	case 2:
		if (sat_sort_desc)
			cmp = sat_cmp_signal_desc;
		else
			cmp = sat_cmp_signal_asc;
	break;
	}

	if (!cmp)
		return;

	qsort(gpsdata.skyview, gpsdata.satellites_visible, sizeof(struct satellite_t), cmp);
}

static int event_gps(struct gp_fd *self, struct pollfd *pfd)
{
	(void)self;
	(void)pfd;
	int ret;

	if (gpsdata.gps_fd == 0)
		return 1;

#if GPSD_API_MAJOR_VERSION >= 10
	ret = gps_read(&gpsdata, NULL, 0);
#else
	ret = gps_read(&gpsdata);
#endif
	if (ret < 0) {
		if (server_status)
			gp_widget_label_printf(server_status, "Read %s", strerror(errno));
		gps_close(&gpsdata);
		gpsdata.gps_fd = 0;
		return 1;
	}

	printf("GPS! %i\n", ret);

	printf("%i\n", gpsdata.satellites_visible);

	do_sort();
	gp_widget_redraw(table);

	char tmp[64];
#if GPSD_API_MAJOR_VERSION >= 10
	gp_widget_label_set(gps_time, timespec_to_iso8601(gpsdata.fix.time, tmp, sizeof(tmp)));
#else
	if (!isnan(gpsdata.fix.time)) {
		gp_widget_label_set(gps_time, unix_to_iso8601(gpsdata.fix.time, tmp, sizeof(tmp)));
	}
#endif

	if (gpsdata.fix.mode >= MODE_2D) {
		if (gps_lat && !isnan(gpsdata.fix.latitude)) {
			gp_widget_label_printf(gps_lat, "%.5f %c", fabs(gpsdata.fix.latitude),
			                       (gpsdata.fix.latitude < 0) ? 'S' : 'N');
		}

		if (gps_epy)
			gp_widget_markup_set_var(gps_epy, 0, "%.3f", gpsdata.fix.epy);

		if (gps_lon && !isnan(gpsdata.fix.latitude)) {
			gp_widget_label_printf(gps_lon, "%.5f %c", fabs(gpsdata.fix.longitude),
			                       (gpsdata.fix.longitude < 0) ? 'W' : 'E');
		}

		if (gps_epx)
			gp_widget_markup_set_var(gps_epx, 0, "%.3f", gpsdata.fix.epx);

		if (gps_speed)
			gp_widget_markup_set_var(gps_speed, 0, "%.2f", gpsdata.fix.speed);

		if (gps_eps)
			gp_widget_markup_set_var(gps_eps, 0, "%.3f", gpsdata.fix.eps);

		if (gps_track)
			gp_widget_markup_set_var(gps_track, 0, "%.2f", gpsdata.fix.track);

		if (gps_ept)
			gp_widget_markup_set_var(gps_ept, 0, "%.3f", gpsdata.fix.ept);

	}

	if (gpsdata.fix.mode >= MODE_3D) {
		if (gps_alt && !isnan(gpsdata.fix.altitude))
			gp_widget_markup_set_var(gps_alt, 0, "%.3f", gpsdata.fix.altitude);

		if (gps_epv)
			gp_widget_markup_set_var(gps_epv, 0, "%.3f", gpsdata.fix.epv);

		if (gps_climb)
			gp_widget_markup_set_var(gps_climb, 0, "%.2f", gpsdata.fix.climb);

		if (gps_epc)
			gp_widget_markup_set_var(gps_epc, 0, "%.3f", gpsdata.fix.epc);

	}

	gp_widget_table_refresh(table);

	return 0;
}

static const char *gps_netlib_err(int err)
{
	switch (err) {
	case NL_NOSERVICE:
		return "Can't resolve host";
	case NL_NOHOST:
		return "Host does not exist";
	case NL_NOSOCK:
		return "Can't create socket";
	case NL_NOSOCKOPT:
		return "Can't set SO_REUSEADDR";
	case NL_NOCONNECT:
		return "Connection failed";
	default:
		return "Unknown error";
	}
}

static void init_gps(void)
{
	const char *host = gp_widget_textbox_str(server_host);
	const char *port = gp_widget_textbox_str(server_port);

	if (gpsdata.gps_fd)
		return;

	if (gps_open(host, port, &gpsdata)) {
		if (server_status)
			gp_widget_label_set(server_status, gps_netlib_err(errno));
		gpsdata.gps_fd = 0;
		return;
	}

	if (server_status)
		gp_widget_label_set(server_status, "Connected");

	gps_stream(&gpsdata, WATCH_ENABLE, NULL);

	gp_fds_add(gp_widgets_fds, gpsdata.gps_fd, POLLIN, event_gps, NULL);
}

int connect_btn(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	gp_widget_label_set(server_status, "Connecting...");

	init_gps();

	return 1;
}

static void exit_gps(void)
{
	if (gpsdata.gps_fd == 0)
		return;

	if (server_status)
		gp_widget_label_set(server_status, "Disconnected");

	gps_stream(&gpsdata, WATCH_DISABLE, NULL);
	gps_close(&gpsdata);

	gpsdata.gps_fd = 0;
}

int disconnect_btn(gp_widget_event *ev)
{
	if (ev->type != GP_WIDGET_EVENT_ACTION)
		return 0;

	exit_gps();

	return 1;
}

int main(int argc, char *argv[])
{
	void *uids;

	gp_widget *layout = gp_widget_layout_json("gpsd-test.json", &uids);
	if (!layout)
		return 0;

	table = gp_widget_by_uid(uids, "sat_table", GP_WIDGET_TABLE);
	if (table) {
		table->tbl->col_min_sizes[0] = 5;
		table->tbl->col_min_sizes[1] = 6;
		table->tbl->col_min_sizes[2] = 4;
	}

	gps_time = gp_widget_by_uid(uids, "time", GP_WIDGET_LABEL);
	gps_lat = gp_widget_by_uid(uids, "lat", GP_WIDGET_LABEL);
	gps_lon = gp_widget_by_uid(uids, "lon", GP_WIDGET_LABEL);
	gps_alt = gp_widget_by_uid(uids, "alt", GP_WIDGET_MARKUP);

	gps_epy = gp_widget_by_uid(uids, "epy", GP_WIDGET_MARKUP);
	gps_epx = gp_widget_by_uid(uids, "epx", GP_WIDGET_MARKUP);
	gps_epv = gp_widget_by_uid(uids, "epv", GP_WIDGET_MARKUP);

	gps_speed = gp_widget_by_uid(uids, "speed", GP_WIDGET_MARKUP);
	gps_track = gp_widget_by_uid(uids, "track", GP_WIDGET_MARKUP);
	gps_climb = gp_widget_by_uid(uids, "climb", GP_WIDGET_MARKUP);

	gps_eps = gp_widget_by_uid(uids, "eps", GP_WIDGET_MARKUP);
	gps_ept = gp_widget_by_uid(uids, "ept", GP_WIDGET_MARKUP);
	gps_epc = gp_widget_by_uid(uids, "epc", GP_WIDGET_MARKUP);

	server_host = gp_widget_by_uid(uids, "server_host", GP_WIDGET_TEXTBOX);
	server_port = gp_widget_by_uid(uids, "server_port", GP_WIDGET_TEXTBOX);
	server_status = gp_widget_by_uid(uids, "server_status", GP_WIDGET_LABEL);

	gp_widgets_main_loop(layout, "gpsd-test", init_gps, argc, argv);

	return 0;
}
