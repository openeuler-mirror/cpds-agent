#include "metric_group_type.h"
#include "prom.h"
#include "logger.h"

static void group_node_basic_init();
static void group_node_basic_destroy();
static void group_node_basic_update();

metric_group group_node_basic = {.name = "node_basic_group",
                                 .update_period = 0, // DO NOT need update
                                 .init = group_node_basic_init,
                                 .destroy = group_node_basic_destroy,
                                 .update = group_node_basic_update};

static prom_gauge_t *cpds_node_basic_info;

static void group_node_basic_init()
{
	metric_group *grp = &group_node_basic;
	const char *labels[] = {"os_version", "kernel_version", "arch"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_node_basic_info = prom_gauge_new("cpds_node_basic_info", "node basic information", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_basic_info);

	group_node_basic_update();
}

static void group_node_basic_destroy()
{
	if (group_node_basic.metrics)
		g_list_free(group_node_basic.metrics);
}

static void group_node_basic_update()
{
	gchar *os_version = NULL;
	if (g_file_get_contents("/etc/issue", &os_version, NULL, NULL) == FALSE)
		CPDS_LOG_WARN("Failed to get /etc/issue content");
	else
		g_strstrip(os_version);

	gchar *kernel_version = NULL;
	if (g_spawn_command_line_sync("uname -r", &kernel_version, NULL, NULL, NULL) == FALSE)
		CPDS_LOG_WARN("Failed to get kernel version");

	gchar *arch = NULL;
	if (g_spawn_command_line_sync("uname -m", &arch, NULL, NULL, NULL) == FALSE)
		CPDS_LOG_WARN("Failed to get machine architecture");

	prom_gauge_set(cpds_node_basic_info, 1, (const char *[]){os_version, kernel_version, arch});

	if (os_version)
		g_free(os_version);
	if (kernel_version)
		g_free(kernel_version);
	if (arch)
		g_free(arch);
}
