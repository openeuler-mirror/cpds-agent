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
                                   .update_period = 30,
                                   .init = group_node_network_init,
                                   .destroy = group_node_network_destroy,
                                   .update = group_node_network_update};

static prom_gauge_t *cpds_node_network_info;

static void group_node_network_init()
{
	metric_group *grp = &group_node_network;
	const char *labels[] = {"interface", "mac", "ip", "mask"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_node_network_info = prom_gauge_new("cpds_node_network_info", "node network information", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_network_info);
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
	struct ifreq ifrcopy;
	char mac[20] = {0};
	char ip[20] = {0};
	char mask[20] = {0};

	// 每次更新清理一下，避免残留已删除的指标项
	prom_gauge_clear(cpds_node_network_info);

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
			// ignore the interface that not up or not running
			ifrcopy = ifr[if_num];
			if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy)) {
				CPDS_LOG_ERROR("ioctl: %s [%s:%d]", strerror(errno), __FILE__, __LINE__);
				continue;
			}

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

static void group_node_network_update()
{
	update_net_info_metrics();
}
