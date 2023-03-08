#include "metric_groups.h"

extern metric_group group_agent_alive;
extern metric_group group_test;

metric_group_list *init_metric_groups(metric_group_list *mgroups)
{
	mgroups = g_list_append(mgroups, &group_agent_alive);
	mgroups = g_list_append(mgroups, &group_test);
	return mgroups;
}
