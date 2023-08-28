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

#include "logger.h"
#include "metric_group_type.h"
#include "prom.h"
#include "ping.h"
#include "context.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int ping_tag = 0;

static void group_node_network_init();
static void group_node_network_destroy();
static void group_node_network_update();

metric_group group_node_network = {.name = "node_network_group",
                                   .update_period = 3,
                                   .init = group_node_network_init,
                                   .destroy = group_node_network_destroy,
                                   .update = group_node_network_update};

static prom_gauge_t *cpds_node_network_info;
static prom_gauge_t *cpds_node_network_up;

static prom_counter_t *cpds_node_network_receive_bytes_total;
static prom_counter_t *cpds_node_network_receive_drop_total;
static prom_counter_t *cpds_node_network_receive_errors_total;
static prom_counter_t *cpds_node_network_receive_packets_total;
static prom_counter_t *cpds_node_network_transmit_bytes_total;
static prom_counter_t *cpds_node_network_transmit_drop_total;
static prom_counter_t *cpds_node_network_transmit_errors_total;
static prom_counter_t *cpds_node_network_transmit_packets_total;

static prom_counter_t *cpds_node_netstat_tcp_retrans_segs;
static prom_counter_t *cpds_node_netstat_tcp_out_segs;

static prom_counter_t *cpds_node_ping_send_count_total;
static prom_counter_t *cpds_node_ping_recv_count_total;
static prom_counter_t *cpds_node_ping_rtt_total;

static void group_node_network_init()
{
	metric_group *grp = &group_node_network;
	
	const char *labels[] = {"interface", "mac", "ip", "mask"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_node_network_info = prom_gauge_new("cpds_node_network_info", "node network information", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_info);

	const char *if_labels[] = {"interface"};
	size_t if_label_count = sizeof(if_labels) / sizeof(if_labels[0]);
	cpds_node_network_up = prom_gauge_new("cpds_node_network_up", "node network state (up/down)", if_label_count, if_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_up);

	cpds_node_network_receive_bytes_total = prom_counter_new("cpds_node_network_receive_bytes_total", "total bytes net interface received", if_label_count, if_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_receive_bytes_total);
	cpds_node_network_receive_drop_total = prom_counter_new("cpds_node_network_receive_drop_total", "total dropped packages net interface received", if_label_count, if_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_receive_drop_total);
	cpds_node_network_receive_errors_total = prom_counter_new("cpds_node_network_receive_errors_total", "total error packages net interface received", if_label_count, if_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_receive_errors_total);
	cpds_node_network_receive_packets_total = prom_counter_new("cpds_node_network_receive_packets_total", "total packages net interface received", if_label_count, if_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_receive_packets_total);
	cpds_node_network_transmit_bytes_total = prom_counter_new("cpds_node_network_transmit_bytes_total", "total bytes net interface transmitted", if_label_count, if_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_transmit_bytes_total);
	cpds_node_network_transmit_drop_total = prom_counter_new("cpds_node_network_transmit_drop_total", "total dropped packages net interface transmitted", if_label_count, if_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_transmit_drop_total);
	cpds_node_network_transmit_errors_total = prom_counter_new("cpds_node_network_transmit_errors_total", "total error packages net interface transmitted", if_label_count, if_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_transmit_errors_total);
	cpds_node_network_transmit_packets_total = prom_counter_new("cpds_node_network_transmit_packets_total", "total packages net interface transmitted", if_label_count, if_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_transmit_packets_total);

	cpds_node_netstat_tcp_retrans_segs = prom_counter_new("cpds_node_netstat_tcp_retrans_segs", "TCP Retransmitted Segments", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_netstat_tcp_retrans_segs);
	cpds_node_netstat_tcp_out_segs = prom_counter_new("cpds_node_netstat_tcp_out_segs", "TCP Outgoing Segments", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_netstat_tcp_out_segs);

	cpds_node_ping_send_count_total = prom_counter_new("cpds_node_ping_send_count_total", "totla ping sent count", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_ping_send_count_total);
	cpds_node_ping_recv_count_total = prom_counter_new("cpds_node_ping_recv_count_total", "totla ping received count", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_ping_recv_count_total);
	cpds_node_ping_rtt_total = prom_counter_new("cpds_node_ping_rtt_total", "totla ping round-trip time", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, cpds_node_ping_rtt_total);

	ping_tag = gen_ping_tag();
	register_ping_item(ping_tag, global_ctx.net_diagnostic_dest);
}

static void group_node_network_destroy()
{
	if (group_node_network.metrics)
		g_list_free(group_node_network.metrics);
	unregister_ping_item(ping_tag);
}

static void update_interface_info_metircs(const char *ifname)
{
	int fd;
	struct ifreq ifr;
	char mac[20] = {0};
	char ip[20] = {0};
	char mask[20] = {0};

	if (!ifname)
		return;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		close(fd);
		return;
	}

	strcpy(ifr.ifr_name, ifname);

	if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0) {
		// get "up"/"down" state of this interface
		if (ifr.ifr_flags & IFF_UP)
			prom_gauge_set(cpds_node_network_up, 1, (const char *[]){ifr.ifr_name});
		else
			prom_gauge_set(cpds_node_network_up, 0, (const char *[]){ifr.ifr_name});
	}

	// get the mac of this interface
	if (ioctl(fd, SIOCGIFHWADDR, (char *)(&ifr)) == 0) {
		memset(mac, 0, sizeof(mac));
		snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)ifr.ifr_hwaddr.sa_data[0],
		         (unsigned char)ifr.ifr_hwaddr.sa_data[1], (unsigned char)ifr.ifr_hwaddr.sa_data[2],
		         (unsigned char)ifr.ifr_hwaddr.sa_data[3], (unsigned char)ifr.ifr_hwaddr.sa_data[4],
		         (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
	}

	// get the IP of this interface
	if (ioctl(fd, SIOCGIFADDR, (char *)&ifr) == 0) {
		snprintf(ip, sizeof(ip), "%s", (char *)inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));
	}

	// get the subnet mask of this interface
	if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0) {
		snprintf(mask, sizeof(mask), "%s", (char *)inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_netmask))->sin_addr));
	}

	prom_gauge_set(cpds_node_network_info, 1, (const char *[]){ifr.ifr_name, mac, ip, mask});

	close(fd);
}

static void update_net_dev_metrics()
{
	FILE *fp = NULL;
	char line[500];
	char ifname[100];

	// Receive
	unsigned long long r_bytes;
	unsigned long long r_packets;
	unsigned long long r_errs;
	unsigned long long r_drop;
	unsigned long long r_fifo;
	unsigned long long r_frame;
	unsigned long long r_compressed;
	unsigned long long r_multicast;

	// Transmit
	unsigned long long t_bytes;
	unsigned long long t_packets;
	unsigned long long t_errs;
	unsigned long long t_drop;
	unsigned long long t_fifo;
	unsigned long long t_colls;
	unsigned long long t_carrier;
	unsigned long long t_compressed;

	prom_gauge_clear(cpds_node_network_info);
	prom_gauge_clear(cpds_node_network_up);
	prom_counter_clear(cpds_node_network_receive_bytes_total);
	prom_counter_clear(cpds_node_network_receive_drop_total);
	prom_counter_clear(cpds_node_network_receive_errors_total);
	prom_counter_clear(cpds_node_network_receive_packets_total);
	prom_counter_clear(cpds_node_network_transmit_bytes_total);
	prom_counter_clear(cpds_node_network_transmit_drop_total);
	prom_counter_clear(cpds_node_network_transmit_errors_total);
	prom_counter_clear(cpds_node_network_transmit_packets_total);

	fp = fopen("/proc/net/dev", "r");
	if (fp == NULL) {
		goto out;
	}

	int i = 0;
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (i++ < 2) // 跳过前两行
			continue;
		sscanf(line, "%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", ifname,
		       &r_bytes, &r_packets, &r_errs, &r_drop, &r_fifo, &r_frame, &r_compressed, &r_multicast, &t_bytes,
		       &t_packets, &t_errs, &t_drop, &t_fifo, &t_colls, &t_carrier, &t_compressed);
		char *p = strtok(ifname, ":");
		if (p) {
			update_interface_info_metircs(ifname);
			prom_counter_set(cpds_node_network_receive_bytes_total, r_bytes, (const char *[]){ifname});
			prom_counter_set(cpds_node_network_receive_drop_total, r_drop, (const char *[]){ifname});
			prom_counter_set(cpds_node_network_receive_errors_total, r_errs, (const char *[]){ifname});
			prom_counter_set(cpds_node_network_receive_packets_total, r_packets, (const char *[]){ifname});
			prom_counter_set(cpds_node_network_transmit_bytes_total, t_bytes, (const char *[]){ifname});
			prom_counter_set(cpds_node_network_transmit_drop_total, t_drop, (const char *[]){ifname});
			prom_counter_set(cpds_node_network_transmit_errors_total, t_errs, (const char *[]){ifname});
			prom_counter_set(cpds_node_network_transmit_packets_total, t_packets, (const char *[]){ifname});
		}
	}

out:
	if (fp)
		fclose(fp);
}

static void update_netstat_tcp_metrics()
{
	FILE *fp = NULL;
	char line[500];

	fp = fopen("/proc/net/snmp", "r");
	if (fp == NULL) {
		goto out;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		// 扫描“Tcp”行
		if (strncmp("Tcp", line, 3) != 0)
			continue;
		int i = 0;
		int num = 0;
		int idx_OutSegs = -1, idx_RetransSegs = -1;
		char **header_arr = g_strsplit(line, " ", -1);
		num = g_strv_length(header_arr);
		for (i = 0; i < num; i++) {
			if (strncmp("OutSegs", header_arr[i], strlen("OutSegs")) == 0)
				idx_OutSegs = i;
			else if (strncmp("RetransSegs", header_arr[i], strlen("RetransSegs")) == 0)
				idx_RetransSegs = i;
		}
		g_strfreev(header_arr);
		if (idx_OutSegs < 0 || idx_RetransSegs < 0)
			break;
		// 值在下一行对应位置
		if (fgets(line, sizeof(line), fp) == NULL)
			break;
		char **value_arr = g_strsplit(line, " ", -1);
		num = g_strv_length(value_arr);
		if (idx_OutSegs < num)
			prom_counter_set(cpds_node_netstat_tcp_out_segs, g_ascii_strtod(value_arr[idx_OutSegs], NULL), NULL);
		if (idx_RetransSegs < num)
			prom_counter_set(cpds_node_netstat_tcp_retrans_segs, g_ascii_strtod(value_arr[idx_RetransSegs], NULL), NULL);
		g_strfreev(value_arr);
	}

out:
	if (fp)
		fclose(fp);
}

static void update_ping_metrics()
{
	ping_info_t info = {0};
	int ret = get_ping_info(ping_tag, &info);
	if (ret == 0) {
		prom_counter_set(cpds_node_ping_send_count_total, info.send_cnt, NULL);
		prom_counter_set(cpds_node_ping_recv_count_total, info.recv_cnt, NULL);
		prom_counter_set(cpds_node_ping_rtt_total, info.rtt, NULL);
	}
}

static void group_node_network_update()
{
	update_net_dev_metrics();
	update_netstat_tcp_metrics();
	update_ping_metrics();
}
