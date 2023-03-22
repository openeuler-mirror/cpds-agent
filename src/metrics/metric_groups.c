#include "metric_groups.h"

extern metric_group group_agent_alive;
extern metric_group group_node_basic;
extern metric_group group_node_network;
extern metric_group group_node_cpu;
extern metric_group group_node_memory;
extern metric_group group_node_fs;
extern metric_group group_node_disk;
extern metric_group group_node_service;

metric_group_list *init_metric_groups(metric_group_list *mgroups)
{
	mgroups = g_list_append(mgroups, &group_agent_alive);
	mgroups = g_list_append(mgroups, &group_node_basic);
	mgroups = g_list_append(mgroups, &group_node_network);
	mgroups = g_list_append(mgroups, &group_node_cpu);
	mgroups = g_list_append(mgroups, &group_node_memory);
	mgroups = g_list_append(mgroups, &group_node_fs);
	mgroups = g_list_append(mgroups, &group_node_disk);
	mgroups = g_list_append(mgroups, &group_node_service);
	return mgroups;
}
