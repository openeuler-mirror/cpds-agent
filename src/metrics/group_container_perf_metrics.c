#include "container_collector.h"
#include "metric_group_type.h"
#include "prom.h"

static void group_container_perf_init();
static void group_container_perf_destroy();
static void group_container_perf_update();

metric_group group_container_perf = {.name = "container_perf_group",
                                     .update_period = 3,
                                     .init = group_container_perf_init,
                                     .destroy = group_container_perf_destroy,
                                     .update = group_container_perf_update};

static prom_counter_t *cpds_container_alloc_memory_fail_cnt_total;
static prom_counter_t *cpds_container_alloc_memory_time_seconds_total;
static prom_counter_t *cpds_container_alloc_memory_bytes_total;
static prom_counter_t *cpds_container_alloc_memory_count_total;
static prom_counter_t *cpds_container_create_process_fail_cnt_total;

static void group_container_perf_init()
{
	metric_group *grp = &group_container_perf;
	const char *labels[] = {"container"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_container_alloc_memory_fail_cnt_total = prom_counter_new("cpds_container_alloc_memory_fail_cnt_total", "total failure counts for container memory allocation", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_alloc_memory_fail_cnt_total);
	cpds_container_alloc_memory_time_seconds_total = prom_counter_new("cpds_container_alloc_memory_time_seconds_total", "total seconds for container memory allocation", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_alloc_memory_time_seconds_total);
	cpds_container_alloc_memory_bytes_total = prom_counter_new( "cpds_container_alloc_memory_bytes_total", "total bytes for container memory allocation", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_alloc_memory_bytes_total);
	cpds_container_alloc_memory_count_total = prom_counter_new( "cpds_container_alloc_memory_count_total", "total counts for container memory allocation", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_alloc_memory_count_total);
	cpds_container_create_process_fail_cnt_total = prom_counter_new( "cpds_container_create_process_fail_cnt_total", "total failure counts for container process creation", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_create_process_fail_cnt_total);
}

static void group_container_perf_destroy()
{
	if (group_container_perf.metrics)
		g_list_free(group_container_perf.metrics);
}

static void group_container_perf_update()
{
	prom_counter_clear(cpds_container_alloc_memory_fail_cnt_total);
	prom_counter_clear(cpds_container_alloc_memory_time_seconds_total);
	prom_counter_clear(cpds_container_alloc_memory_bytes_total);
	prom_counter_clear(cpds_container_alloc_memory_count_total);
	prom_counter_clear(cpds_container_create_process_fail_cnt_total);

	GList *plist = get_container_perf_info_list();
	GList *iter = plist;
	while (iter != NULL) {
		ctn_perf_metric *cpm = iter->data;
		prom_counter_set(cpds_container_alloc_memory_fail_cnt_total, cpm->total_mmap_fail_count, (const char *[]){cpm->cid});
		prom_counter_set(cpds_container_alloc_memory_time_seconds_total, cpm->total_mmap_time_seconds, (const char *[]){cpm->cid});
		prom_counter_set(cpds_container_alloc_memory_bytes_total, cpm->total_mmap_size, (const char *[]){cpm->cid});
		prom_counter_set(cpds_container_alloc_memory_count_total, cpm->total_mmap_count, (const char *[]){cpm->cid});
		prom_counter_set(cpds_container_create_process_fail_cnt_total, cpm->total_fork_fail_cnt, (const char *[]){cpm->cid});
		g_free(cpm->cid);
		g_free(cpm);
		plist = g_list_delete_link(plist, iter);
		iter = plist;
	}
}
