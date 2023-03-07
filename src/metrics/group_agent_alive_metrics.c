#include "metric_group_type.h"
#include "prom.h"

static void group_agent_alive_init();
static void group_agent_alive_destroy();
static void group_agent_alive_update();

metric_group group_agent_alive = {.name = "agent_alive_group",
                                  .update_period = 10,
                                  .init = group_agent_alive_init,
                                  .destroy = group_agent_alive_destroy,
                                  .update = group_agent_alive_update};

static prom_counter_t *cpds_agent_alive_count;

static void group_agent_alive_init()
{
	metric_group *grp = &group_agent_alive;
	static const char *labels[] = {"version"};
	cpds_agent_alive_count = prom_counter_new("cpds_agent_alive_count", "agent alive heartbeat", 1, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_agent_alive_count);
}

static void group_agent_alive_destroy()
{
	if (group_agent_alive.metrics)
		g_list_free(group_agent_alive.metrics);
}

static void group_agent_alive_update()
{
	prom_counter_inc(cpds_agent_alive_count, (const char *[]){CPDS_AGENT_VERSION});
}
