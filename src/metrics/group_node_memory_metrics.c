#include "metric_group_type.h"
#include "prom.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>

static void group_node_memory_init();
static void group_node_memory_destroy();
static void group_node_memory_update();

metric_group group_node_memory = {.name = "node_memory_group",
                                  .update_period = 10,
                                  .init = group_node_memory_init,
                                  .destroy = group_node_memory_destroy,
                                  .update = group_node_memory_update};

static prom_gauge_t *cpds_node_memory_total_bytes;
static prom_gauge_t *cpds_node_memory_usage_bytes;
static prom_gauge_t *cpds_node_memory_cache_bytes;
static prom_gauge_t *cpds_node_memory_swap_tatal_bytes;
static prom_gauge_t *cpds_node_memory_swap_usage_bytes;

static void group_node_memory_init()
{
	metric_group *grp = &group_node_memory;
	cpds_node_memory_total_bytes = prom_gauge_new("cpds_node_memory_total_bytes", "node total memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_total_bytes);
	cpds_node_memory_usage_bytes = prom_gauge_new("cpds_node_memory_usage_bytes", "node used memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_usage_bytes);
	cpds_node_memory_cache_bytes = prom_gauge_new("cpds_node_memory_cache_bytes", "node cache/buffer memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_cache_bytes);
	cpds_node_memory_swap_tatal_bytes = prom_gauge_new("cpds_node_memory_swap_tatal_bytes", "node swap total memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_swap_tatal_bytes);
	cpds_node_memory_swap_usage_bytes = prom_gauge_new("cpds_node_memory_swap_usage_bytes", "node swap used memory in bytes", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_memory_swap_usage_bytes);
}

static void group_node_memory_destroy()
{
	if (group_node_memory.metrics)
		g_list_free(group_node_memory.metrics);
}

static void group_node_memory_update()
{
	struct sysinfo s_info;
	if (sysinfo(&s_info) == 0) {
		prom_gauge_set(cpds_node_memory_total_bytes, s_info.totalram, NULL);
		prom_gauge_set(cpds_node_memory_usage_bytes, s_info.totalram - s_info.freehigh - s_info.bufferram, NULL);
		prom_gauge_set(cpds_node_memory_cache_bytes, s_info.bufferram, NULL);
		prom_gauge_set(cpds_node_memory_swap_tatal_bytes, s_info.totalswap, NULL);
		prom_gauge_set(cpds_node_memory_swap_usage_bytes, s_info.totalswap - s_info.freeswap, NULL);
	} else {
		prom_gauge_clear(cpds_node_memory_total_bytes);
		prom_gauge_clear(cpds_node_memory_usage_bytes);
		prom_gauge_clear(cpds_node_memory_cache_bytes);
		prom_gauge_clear(cpds_node_memory_swap_tatal_bytes);
		prom_gauge_clear(cpds_node_memory_swap_usage_bytes);
	}
}