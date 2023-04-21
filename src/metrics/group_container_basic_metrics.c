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
	const char *labels[] = {"container", "pid", "state"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_container_state = prom_gauge_new("cpds_container_state", "container basic", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_state);
}

static void group_container_basic_destroy()
{
	if (group_container_basic.metrics)
		g_list_free(group_container_basic.metrics);
}

static void group_container_basic_update()
{
	prom_gauge_clear(cpds_container_state);

	GList *plist = get_container_basic_info_list();
	GList *iter = plist;
	while (iter != NULL) {
		ctn_basic_metric *cbm = iter->data;
		char str_pid[10] = {0};
		g_snprintf(str_pid, sizeof(str_pid), "%d", cbm->pid);
		prom_gauge_set(cpds_container_state, 1, (const char *[]){cbm->cid, str_pid, cbm->status});
		g_free(cbm->cid);
		g_free(cbm->status);
		g_free(cbm);
		plist = g_list_delete_link(plist, iter);
		iter = plist;
	}
}
