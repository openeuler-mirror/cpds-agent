#include "logger.h"
#include "metric_group_type.h"
#include "prom.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static void group_node_network_init();
static void group_node_network_destroy();
static void group_node_network_update();

metric_group group_node_network = {.name = "node_network_group",
                                   .update_period = 4,
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
}

static void group_node_network_destroy()
{
	if (group_node_network.metrics)
		g_list_free(group_node_network.metrics);
}

static int update_net_info_metrics(void)
{
	int fd;
	int if_num = 0;
	struct ifreq ifr[200] = {0};
	struct ifconf ifc;
	char mac[20] = {0};
	char ip[20] = {0};
	char mask[20] = {0};

	// 每次更新清理一下，避免残留已删除的指标项
	prom_gauge_clear(cpds_node_network_info);
	prom_gauge_clear(cpds_node_network_up);

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		close(fd);
		return -1;
	}

	ifc.ifc_len = sizeof(ifr);
	ifc.ifc_buf = (caddr_t)ifr;
	if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)) {
		if_num = ifc.ifc_len / sizeof(struct ifreq);
		while (if_num-- > 0) {
			if (ioctl(fd, SIOCGIFFLAGS, &ifr[if_num]) < 0) {
				CPDS_LOG_ERROR("ioctl: %s [%s:%d]", strerror(errno), __FILE__, __LINE__);
				continue;
			}

			// get "up"/"down" state of this interface
			if (ifr[if_num].ifr_flags & IFF_UP)
				prom_gauge_set(cpds_node_network_up, 1, (const char *[]){ifr[if_num].ifr_name});
			else 
				prom_gauge_set(cpds_node_network_up, 0, (const char *[]){ifr[if_num].ifr_name});

			// get the mac of this interface
			if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&ifr[if_num]))) {
				memset(mac, 0, sizeof(mac));
				snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
				         (unsigned char)ifr[if_num].ifr_hwaddr.sa_data[0],
				         (unsigned char)ifr[if_num].ifr_hwaddr.sa_data[1],
				         (unsigned char)ifr[if_num].ifr_hwaddr.sa_data[2],
				         (unsigned char)ifr[if_num].ifr_hwaddr.sa_data[3],
				         (unsigned char)ifr[if_num].ifr_hwaddr.sa_data[4],
				         (unsigned char)ifr[if_num].ifr_hwaddr.sa_data[5]);
			} else {
				CPDS_LOG_ERROR("ioctl: %s [%s:%d]", strerror(errno), __FILE__, __LINE__);
				continue;
			}

			// get the IP of this interface
			if (!ioctl(fd, SIOCGIFADDR, (char *)&ifr[if_num])) {
				snprintf(ip, sizeof(ip), "%s",
				         (char *)inet_ntoa(((struct sockaddr_in *)&(ifr[if_num].ifr_addr))->sin_addr));
			} else {
				CPDS_LOG_ERROR("ioctl: %s [%s:%d]", strerror(errno), __FILE__, __LINE__);
				continue;
			}

			// get the subnet mask of this interface
			if (!ioctl(fd, SIOCGIFNETMASK, &ifr[if_num])) {
				snprintf(mask, sizeof(mask), "%s",
				         (char *)inet_ntoa(((struct sockaddr_in *)&(ifr[if_num].ifr_netmask))->sin_addr));
			} else {
				CPDS_LOG_ERROR("ioctl: %s [%s:%d]", strerror(errno), __FILE__, __LINE__);
				continue;
			}

			prom_gauge_set(cpds_node_network_info, 1, (const char *[]){ifr[if_num].ifr_name, mac, ip, mask});
		}
	} else {
		CPDS_LOG_ERROR("ioctl: %s [%s:%d]", strerror(errno), __FILE__, __LINE__);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
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

static void group_node_network_update()
{
	update_net_info_metrics();
	update_net_dev_metrics();
	update_netstat_tcp_metrics();
}
