#include "container_collector.h"
#include "metric_group_type.h"
#include "prom.h"

#include <stdio.h>
#include <unistd.h>

static void group_container_resource_init();
static void group_container_resource_destroy();
static void group_container_resource_update();

metric_group group_container_resource = {.name = "container_resource_group",
                                         .update_period = 1,
                                         .init = group_container_resource_init,
                                         .destroy = group_container_resource_destroy,
                                         .update = group_container_resource_update};

// memory related metrics
static prom_gauge_t *cpds_container_memory_total_bytes;
static prom_gauge_t *cpds_container_memory_usage_bytes;
static prom_gauge_t *cpds_container_memory_swap_total_bytes;
static prom_gauge_t *cpds_container_memory_swap_usage_bytes;
static prom_gauge_t *cpds_container_memory_cache_bytes;

// cpu related metrics
static prom_gauge_t *cpds_container_cpu_usage_seconds_total;

// disk related metrics
static prom_gauge_t *cpds_container_disk_usage_bytes;
static prom_gauge_t *cpds_container_disk_iodelay_total;

// network related metrics
static prom_gauge_t *cpds_container_icmp_out_type8_total;
static prom_gauge_t *cpds_container_icmp_in_type0_total;
static prom_counter_t *cpds_container_network_receive_bytes_total;
static prom_counter_t *cpds_container_network_receive_drop_total;
static prom_counter_t *cpds_container_network_receive_errors_total;
static prom_counter_t *cpds_container_network_receive_packets_total;
static prom_counter_t *cpds_container_network_transmit_bytes_total;
static prom_counter_t *cpds_container_network_transmit_drop_total;
static prom_counter_t *cpds_container_network_transmit_errors_total;
static prom_counter_t *cpds_container_network_transmit_packets_total;

static prom_counter_t *cpds_container_ping_send_count_total;
static prom_counter_t *cpds_container_ping_recv_count_total;
static prom_counter_t *cpds_container_ping_rtt_total;

static void group_container_resource_init()
{
	metric_group *grp = &group_container_resource;
	const char *labels[] = {"container"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_container_memory_total_bytes = prom_gauge_new("cpds_container_memory_total_bytes", "container total memory in bytes", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_memory_total_bytes);
	cpds_container_memory_usage_bytes = prom_gauge_new("cpds_container_memory_usage_bytes", "container memory usage in bytes", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_memory_usage_bytes);
	cpds_container_memory_swap_total_bytes = prom_gauge_new("cpds_container_memory_swap_total_bytes", "container total swap memory in bytes", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_memory_swap_total_bytes);
	cpds_container_memory_swap_usage_bytes = prom_gauge_new("cpds_container_memory_swap_usage_bytes", "container swap memory usage in bytes", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_memory_swap_usage_bytes);
	cpds_container_memory_cache_bytes = prom_gauge_new("cpds_container_memory_cache_bytes", "container cache memory in bytes", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_memory_cache_bytes);
	
	cpds_container_cpu_usage_seconds_total = prom_gauge_new("cpds_container_cpu_usage_seconds_total", "total seconds container cpu usage", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_cpu_usage_seconds_total);
	
	cpds_container_disk_usage_bytes = prom_gauge_new("cpds_container_disk_usage_bytes", "container disk usage in bytes", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_disk_usage_bytes);
	cpds_container_disk_iodelay_total = prom_gauge_new("cpds_container_disk_iodelay_total", "container disk iodelay in ticks", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_disk_iodelay_total);
	
	cpds_container_icmp_out_type8_total = prom_gauge_new("cpds_container_icmp_out_type8_total", "container icmp OutType8 total", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_icmp_out_type8_total);
	cpds_container_icmp_in_type0_total = prom_gauge_new("cpds_container_icmp_in_type0_total", "container icmp InType0 total", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_icmp_in_type0_total);

	const char *ping_labels[] = {"container", "ip"};
	size_t ping_label_count = sizeof(ping_labels) / sizeof(ping_labels[0]);
	cpds_container_ping_send_count_total = prom_counter_new("cpds_container_ping_send_count_total", "totla ping sent count", ping_label_count, ping_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_ping_send_count_total);
	cpds_container_ping_recv_count_total = prom_counter_new("cpds_container_ping_recv_count_total", "totla ping received count", ping_label_count, ping_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_ping_recv_count_total);
	cpds_container_ping_rtt_total = prom_counter_new("cpds_container_ping_rtt_total", "totla ping round-trip time", ping_label_count, ping_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_ping_rtt_total);

	const char *net_labels[] = {"container", "interface", "network_mode"};
	size_t net_label_count = sizeof(net_labels) / sizeof(net_labels[0]);
	cpds_container_network_receive_bytes_total = prom_counter_new("cpds_container_network_receive_bytes_total", "total bytes containter net interface received", net_label_count, net_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_network_receive_bytes_total);
	cpds_container_network_receive_drop_total = prom_counter_new("cpds_container_network_receive_drop_total", "total dropped packages containter net interface received", net_label_count, net_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_network_receive_drop_total);
	cpds_container_network_receive_errors_total = prom_counter_new("cpds_container_network_receive_errors_total", "total error packages containter net interface received", net_label_count, net_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_network_receive_errors_total);
	cpds_container_network_receive_packets_total = prom_counter_new("cpds_container_network_receive_packets_total", "total packages containter net interface received", net_label_count, net_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_network_receive_packets_total);
	cpds_container_network_transmit_bytes_total = prom_counter_new("cpds_container_network_transmit_bytes_total", "total bytes containter net interface transmitted", net_label_count, net_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_network_transmit_bytes_total);
	cpds_container_network_transmit_drop_total = prom_counter_new("cpds_container_network_transmit_drop_total", "total dropped packages containter net interface transmitted", net_label_count, net_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_network_transmit_drop_total);
	cpds_container_network_transmit_errors_total = prom_counter_new("cpds_container_network_transmit_errors_total", "total error packages containter net interface transmitted", net_label_count, net_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_network_transmit_errors_total);
	cpds_container_network_transmit_packets_total = prom_counter_new("cpds_container_network_transmit_packets_total", "total packages containter net interface transmitted", net_label_count, net_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_container_network_transmit_packets_total);
}

static void group_container_resource_destroy()
{
	if (group_container_resource.metrics)
		g_list_free(group_container_resource.metrics);
}

static void update_container_resource_info(GList *plist)
{
	prom_gauge_clear(cpds_container_memory_total_bytes);
	prom_gauge_clear(cpds_container_memory_usage_bytes);
	prom_gauge_clear(cpds_container_memory_swap_total_bytes);
	prom_gauge_clear(cpds_container_memory_swap_usage_bytes);
	prom_gauge_clear(cpds_container_memory_cache_bytes);
	prom_gauge_clear(cpds_container_cpu_usage_seconds_total);
	prom_gauge_clear(cpds_container_disk_usage_bytes);
	prom_gauge_clear(cpds_container_disk_iodelay_total);
	prom_gauge_clear(cpds_container_icmp_out_type8_total);
	prom_gauge_clear(cpds_container_icmp_in_type0_total);
	prom_counter_clear(cpds_container_network_receive_bytes_total);
	prom_counter_clear(cpds_container_network_receive_drop_total);
	prom_counter_clear(cpds_container_network_receive_errors_total);
	prom_counter_clear(cpds_container_network_receive_packets_total);
	prom_counter_clear(cpds_container_network_transmit_bytes_total);
	prom_counter_clear(cpds_container_network_transmit_drop_total);
	prom_counter_clear(cpds_container_network_transmit_errors_total);
	prom_counter_clear(cpds_container_network_transmit_packets_total);
	prom_counter_clear(cpds_container_ping_send_count_total);
	prom_counter_clear(cpds_container_ping_recv_count_total);
	prom_counter_clear(cpds_container_ping_rtt_total);

	GList *iter = plist;
	while (iter != NULL) {
		ctn_resource_metric *crm = iter->data;
		prom_gauge_set(cpds_container_memory_total_bytes, crm->memory_total_bytes, (const char *[]){crm->cid});
		prom_gauge_set(cpds_container_memory_usage_bytes, crm->memory_usage_bytes, (const char *[]){crm->cid});
		prom_gauge_set(cpds_container_memory_swap_total_bytes, crm->memory_swap_total_bytes, (const char *[]){crm->cid});
		prom_gauge_set(cpds_container_memory_swap_usage_bytes, crm->memory_swap_usage_bytes, (const char *[]){crm->cid});
		prom_gauge_set(cpds_container_memory_cache_bytes, crm->memory_cached_bytes, (const char *[]){crm->cid});
		prom_gauge_set(cpds_container_cpu_usage_seconds_total, crm->cpu_usage_seconds, (const char *[]){crm->cid});
		prom_gauge_set(cpds_container_disk_usage_bytes, crm->disk_usage_bytes, (const char *[]){crm->cid});
		prom_gauge_set(cpds_container_disk_iodelay_total, crm->disk_iodelay, (const char *[]){crm->cid});

		prom_gauge_set(cpds_container_icmp_out_type8_total, crm->ctn_net_snmp_stat.network_icmp_out_type8_total, (const char *[]){crm->cid});
		prom_gauge_set(cpds_container_icmp_in_type0_total, crm->ctn_net_snmp_stat.network_icmp_in_type0_total, (const char *[]){crm->cid});

		if (crm->ctn_ping_stat.send_cnt > 0) {
			prom_counter_set(cpds_container_ping_send_count_total, crm->ctn_ping_stat.send_cnt, (const char *[]){crm->cid, crm->ip_addr});
			prom_counter_set(cpds_container_ping_recv_count_total, crm->ctn_ping_stat.recv_cnt, (const char *[]){crm->cid, crm->ip_addr});
			prom_counter_set(cpds_container_ping_rtt_total, crm->ctn_ping_stat.rtt, (const char *[]){crm->cid, crm->ip_addr});
		}

		GList *sub_iter = crm->ctn_net_dev_stat_list;
		while (sub_iter != NULL) {
			ctn_net_dev_stat_metric *cndsm = sub_iter->data;
			prom_counter_set(cpds_container_network_receive_bytes_total, cndsm->network_receive_bytes_total,
			                 (const char *[]){crm->cid, cndsm->ifname, crm->network_mode});
			prom_counter_set(cpds_container_network_receive_drop_total, cndsm->network_receive_drop_total,
			                 (const char *[]){crm->cid, cndsm->ifname, crm->network_mode});
			prom_counter_set(cpds_container_network_receive_errors_total, cndsm->network_receive_errors_total,
			                 (const char *[]){crm->cid, cndsm->ifname, crm->network_mode});
			prom_counter_set(cpds_container_network_receive_packets_total, cndsm->network_receive_packets_total,
			                 (const char *[]){crm->cid, cndsm->ifname, crm->network_mode});
			prom_counter_set(cpds_container_network_transmit_bytes_total, cndsm->network_transmit_bytes_total,
			                 (const char *[]){crm->cid, cndsm->ifname, crm->network_mode});
			prom_counter_set(cpds_container_network_transmit_drop_total, cndsm->network_transmit_drop_total,
			                 (const char *[]){crm->cid, cndsm->ifname, crm->network_mode});
			prom_counter_set(cpds_container_network_transmit_errors_total, cndsm->network_transmit_errors_total,
			                 (const char *[]){crm->cid, cndsm->ifname, crm->network_mode});
			prom_counter_set(cpds_container_network_transmit_packets_total, cndsm->network_transmit_packets_total,
			                 (const char *[]){crm->cid, cndsm->ifname, crm->network_mode});
			sub_iter = sub_iter->next;
		}
		iter = iter->next;
	}
}

static void group_container_resource_update()
{
	get_ctn_resource_metric(update_container_resource_info);
}
