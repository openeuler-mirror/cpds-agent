/* 
 *  Copyright 2023 CPDS Author
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *       https://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License. 
 */

#define _XOPEN_SOURCE
#define __USE_XOPEN
#include <time.h>

#include "metric_group_type.h"
#include "prom.h"
#include "logger.h"

#include <systemd/sd-bus.h>
#include <glib.h>

static sd_bus *bus = NULL;

static void group_kernel_fault_init();
static void group_kernel_fault_destroy();
static void group_kernel_fault_update();

metric_group group_kernel_fault = {.name = "kernel_fault_group",
                                   .update_period = 5,
                                   .init = group_kernel_fault_init,
                                   .destroy = group_kernel_fault_destroy,
                                   .update = group_kernel_fault_update};

static prom_gauge_t *cpds_kernel_crash;

static void group_kernel_fault_init()
{
	metric_group *grp = &group_kernel_fault;
	const char *labels[] = {"time", "reason"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_kernel_crash = prom_gauge_new("cpds_kernel_crash", "docker service status", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_kernel_crash);
}

static void group_kernel_fault_destroy()
{
	if (group_kernel_fault.metrics)
		g_list_free(group_kernel_fault.metrics);

	if (bus) {
		sd_bus_unref(bus);
		bus = NULL;
	}
}

static int update_service_status(const char *service)
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
	if (state != NULL && g_strcmp0(state, "active") == 0)
		ret = 1;

	sd_bus_error_free(&error);
	if (m)
		sd_bus_message_unref(m);
	if (state)
		free(state);

	return ret;
}

static char *get_crash_reason(const char *dir_name)
{
	char *reason = NULL;
	char *dmesg_file = g_strdup_printf("/var/crash/%s/vmcore-dmesg.txt", dir_name);
	FILE *fp = fopen(dmesg_file, "r");
	if (fp == NULL)
		goto out;

	static const char *tags[] = {"BUG: ", "Oops: ", "sysrq: ", "BUG ", "crash ", NULL};
	char line[300] = {0};
	while (fgets(line, sizeof(line), fp) != NULL) {
		int i = 0;
		while (tags[i] != NULL) {
			char *sub_str = g_strstr_len(line, strlen(line), tags[i]);
			if (sub_str != NULL) {
				reason = g_strdup(sub_str);
				g_strstrip(reason);
				break;
			}
			i++;
		}
		if (reason != NULL)
			break;
	}

	if (reason == NULL) {
		reason = g_strdup("Kernel crash");
	}

out:
	if (dmesg_file)
		g_free(dmesg_file);
	if (fp)
		fclose(fp);

	return reason;
}

// 转换成unix时间戳
static long convert_time_stamp(const char *crash_time)
{
	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));

	strptime(crash_time, "%Y-%m-%d-%H:%M:%S", &tm);
	tm.tm_isdst = -1;

	return mktime(&tm);
}

static void update_kernel_crash_metrics()
{
	GDir *carsh_dir = NULL;
	const char *crash_dir_name = "/var/crash";

	prom_gauge_clear(cpds_kernel_crash);

	carsh_dir = g_dir_open(crash_dir_name, 0, NULL);
	if (carsh_dir == NULL) {
		goto out;
	}

	const char *sub_name = NULL;
	while ((sub_name = g_dir_read_name(carsh_dir)) != NULL) {
		// 提取时间信息
		const char *c_time = sub_name;
		while (*c_time != '\0') {
			if (*c_time++ == '-')
				break;
		}
		// 转换成时间戳
		long time_stamp = convert_time_stamp(c_time);
		// 提取原因
		char *c_reason = get_crash_reason(sub_name);
		prom_gauge_set(cpds_kernel_crash, time_stamp, (const char *[]){c_time, c_reason});
		CPDS_LOG_INFO("kernel crash time: '%s', reason: '%s'", c_time, c_reason);
		if (c_reason)
			g_free(c_reason);
	}

out:
	if (carsh_dir)
		g_dir_close(carsh_dir);
}

static void group_kernel_fault_update()
{
	static int update_flag = 1;

	// 内核crash重启后只需要读一次信息
	if (update_flag == 0)
		return;

	// 如果部署的cpds-agent比kdump.service先启动，则继续等待服务拉起；
	// 如过kdump.service不存在（没有安装），则不支持内核crash检测，没有对应metrics上报；
	if (update_service_status("kdump.service") != 1)
		return;

	update_kernel_crash_metrics();

	update_flag = 0;
}
