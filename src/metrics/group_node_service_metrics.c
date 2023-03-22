#include "metric_group_type.h"
#include "prom.h"
#include "logger.h"

#include <systemd/sd-bus.h>
#include <glib.h>

static sd_bus *bus = NULL;

static void group_node_service_init();
static void group_node_service_destroy();
static void group_node_service_update();

metric_group group_node_service = {.name = "node_service_group",
                                   .update_period = 10,
                                   .init = group_node_service_init,
                                   .destroy = group_node_service_destroy,
                                   .update = group_node_service_update};

static prom_gauge_t *cpds_systemd_journald_status;

static void group_node_service_init()
{
	metric_group *grp = &group_node_service;
	cpds_systemd_journald_status = prom_gauge_new("cpds_systemd_journald_status", "journald status", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_systemd_journald_status);
}

static void group_node_service_destroy()
{
	if (group_node_service.metrics)
		g_list_free(group_node_service.metrics);

	if (bus) {
		sd_bus_unref(bus);
		bus = NULL;
	}
}

static int update_systemd_journald_status_metrics()
{
	int ret = -1;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL;
	const char *path = NULL;
	char *state = NULL;
	const char *service = "systemd-journald.service";

	if (bus == NULL) {
		// Connect to the system bus (only once)
		ret = sd_bus_open_system(&bus);
		if (ret < 0) {
			CPDS_LOG_ERROR("Failed to connect to system bus: %s", strerror(errno));
			goto out;
		}
	}

	// Issue the method call and store the response message in m
	ret = sd_bus_call_method(bus,
	                         "org.freedesktop.systemd1",         // service to contact
	                         "/org/freedesktop/systemd1",        // object path
	                         "org.freedesktop.systemd1.Manager", // interface name
	                         "GetUnit",                          // method name
	                         &error,                             // object to return error in
	                         &m,                                 // return message on success
	                         "s",                                // input signature
	                         service);                           // first argument

	if (ret < 0) {
		CPDS_LOG_ERROR("Failed to issue method call: %s", error.message);
		goto out;
	}

	// Parse the response message
	ret = sd_bus_message_read(m, "o", &path);
	if (ret < 0) {
		CPDS_LOG_ERROR("Failed to parse response message: %s", strerror(errno));
		goto out;
	}

	// Get the "ActiveState" property of the service
	ret = sd_bus_get_property_string(bus, "org.freedesktop.systemd1", path, "org.freedesktop.systemd1.Unit",
	                                 "ActiveState", &error, &state);
	if (ret < 0) {
		CPDS_LOG_ERROR("Failed to get property: %s", error.message);
		goto out;
	}

out:
	if (state != NULL && g_strcmp0(state, "active") == 0) {
		prom_gauge_set(cpds_systemd_journald_status, 1, NULL);
	} else {
		prom_gauge_clear(cpds_systemd_journald_status);
	}

	sd_bus_error_free(&error);
	if (m)
		sd_bus_message_unref(m);
	if (state)
		free(state);

	return ret;
}

static void group_node_service_update()
{
	update_systemd_journald_status_metrics();
}
