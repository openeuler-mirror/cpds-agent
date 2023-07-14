#include "metric_group_type.h"
#include "prom.h"
#include "logger.h"

#include <systemd/sd-bus.h>
#include <glib.h>

static sd_bus *bus = NULL;

static void group_container_service_init();
static void group_container_service_destroy();
static void group_container_service_update();

metric_group group_container_service = {.name = "container_service_group",
                                        .update_period = 5,
                                        .init = group_container_service_init,
                                        .destroy = group_container_service_destroy,
                                        .update = group_container_service_update};

static prom_gauge_t *cpds_container_service_docker_status;
static prom_gauge_t *cpds_container_service_etcd_status;
static prom_gauge_t *cpds_container_service_keepalived_status;
static prom_gauge_t *cpds_container_service_haproxy_status;
static prom_gauge_t *cpds_container_service_kube_apiserver_status;
static prom_gauge_t *cpds_container_service_kube_controller_manager_status;
static prom_gauge_t *cpds_container_service_kube_scheduler_status;
static prom_gauge_t *cpds_container_service_kube_proxy_status;
static prom_gauge_t *cpds_container_service_kubelet_status;

static void group_container_service_init()
{
	metric_group *grp = &group_container_service;
	cpds_container_service_docker_status = prom_gauge_new("cpds_container_service_docker_status", "docker service status", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_container_service_docker_status);
	cpds_container_service_etcd_status = prom_gauge_new("cpds_container_service_etcd_status", "etcd service status", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_container_service_etcd_status);
	cpds_container_service_keepalived_status = prom_gauge_new("cpds_container_service_keepalived_status", "keepalived service status", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_container_service_keepalived_status);
	cpds_container_service_haproxy_status = prom_gauge_new("cpds_container_service_haproxy_status", "haproxy service status", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_container_service_haproxy_status);
	cpds_container_service_kube_apiserver_status = prom_gauge_new("cpds_container_service_kube_apiserver_status", "kube-apiserver service status", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_container_service_kube_apiserver_status);
	cpds_container_service_kube_controller_manager_status = prom_gauge_new("cpds_container_service_kube_controller_manager_status", "kube-controller-manager service status", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_container_service_kube_controller_manager_status);
	cpds_container_service_kube_scheduler_status = prom_gauge_new("cpds_container_service_kube_scheduler_status", "kube-scheduler service status", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_container_service_kube_scheduler_status);
	cpds_container_service_kube_proxy_status = prom_gauge_new("cpds_container_service_kube_proxy_status", "kube-proxy service status", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_container_service_kube_proxy_status);
	cpds_container_service_kubelet_status = prom_gauge_new("cpds_container_service_kubelet_status", "kubelet service status", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_container_service_kubelet_status);
}

static void group_container_service_destroy()
{
	if (group_container_service.metrics)
		g_list_free(group_container_service.metrics);

	if (bus) {
		sd_bus_unref(bus);
		bus = NULL;
	}
}

static int update_service_status_metrics(const char *service)
{
	int ret = -1;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL;
	const char *path = NULL;
	char *state = NULL;

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
		goto out;
	}

	// Parse the response message
	ret = sd_bus_message_read(m, "o", &path);
	if (ret < 0) {
		goto out;
	}

	// Get the "ActiveState" property of the service
	ret = sd_bus_get_property_string(bus, "org.freedesktop.systemd1", path, "org.freedesktop.systemd1.Unit",
	                                 "ActiveState", &error, &state);
	if (ret < 0) {
		goto out;
	}

out:
	if (state != NULL && g_strcmp0(state, "active") == 0) {
		ret = 1;
	} else {
		ret = 0;
	}

	sd_bus_error_free(&error);
	if (m)
		sd_bus_message_unref(m);
	if (state)
		free(state);

	return ret;
}

#define UPDATE_SERVICE_METRICS(_name, _service)                                         \
	if (update_service_status_metrics(_service) == 1)                                   \
		prom_gauge_set(cpds_container_service_##_name##_status, 1, NULL);               \
	else                                                                                \
		prom_gauge_set(cpds_container_service_##_name##_status, 0, NULL);

static void group_container_service_update()
{
	UPDATE_SERVICE_METRICS(docker, "docker.service");
	UPDATE_SERVICE_METRICS(etcd, "etcd.service");
	UPDATE_SERVICE_METRICS(keepalived, "keepalived.service");
	UPDATE_SERVICE_METRICS(haproxy, "haproxy.service");
	UPDATE_SERVICE_METRICS(kube_apiserver, "kube-apiserver.service");
	UPDATE_SERVICE_METRICS(kube_controller_manager, "kube-controller-manager.service");
	UPDATE_SERVICE_METRICS(kube_scheduler, "kube-scheduler.service");
	UPDATE_SERVICE_METRICS(kube_proxy, "kube-proxy.service");
	UPDATE_SERVICE_METRICS(kubelet, "kubelet.service");
}
