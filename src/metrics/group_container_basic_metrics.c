#include "container_collector.h"
#include "metric_group_type.h"
#include "prom.h"

static void group_container_basic_init();
static void group_container_basic_destroy();
static void group_container_basic_update();

metric_group group_container_basic = {.name = "container_basic_group",
                                      .update_period = 3,
                                      .init = group_container_basic_init,
                                      .destroy = group_container_basic_destroy,
                                      .update = group_container_basic_update};

static prom_gauge_t *cpds_container_state;

static void group_container_basic_init()
{
	metric_group *grp = &group_container_basic;
	const char *labels[] = {"container", "pid", "state", "exit_code", "ip"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_container_state = prom_gauge_new("cpds_container_state", "container basic", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_state);
}

static void group_container_basic_destroy()
{
	if (group_container_basic.metrics)
		g_list_free(group_container_basic.metrics);
}

static void update_container_basic_info(GList *plist)
{
	prom_gauge_clear(cpds_container_state);

	GList *iter = plist;
	while (iter != NULL) {
		ctn_basic_metric *cbm = iter->data;
		char str_pid[10] = {0};
		g_snprintf(str_pid, sizeof(str_pid), "%d", cbm->pid);
		char str_exit_code[10] = {0};
		g_snprintf(str_exit_code, sizeof(str_exit_code), "%d", cbm->exit_code);
		prom_gauge_set(cpds_container_state, 1, (const char *[]){cbm->cid, str_pid, cbm->status, str_exit_code, cbm->ip_addr});
		iter = iter->next;
	}
}

static void group_container_basic_update()
{
	get_ctn_basic_metric(update_container_basic_info);
}
