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

#include "ping.h"
#include "logger.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>

#define BUFSIZE 1500   // 发送缓存最大值
#define DEFAULT_LEN 56 // ping消息数据默认大小

// 数据类型别名
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

// ICMP消息头部
struct icmphdr {
	u8 type;      // 定义消息类型
	u8 code;      // 定义消息代码
	u16 checksum; // 定义校验
	union {
		struct {
			u16 id;
			u16 sequence;
		} echo;
		u32 gateway;
		struct {
			u16 unsed;
			u16 mtu;
		} frag; // pmtu实现
	} un;
	// ICMP数据占位符
	u8 data[0];
#define icmp_id un.echo.id
#define icmp_seq un.echo.sequence
};
#define ICMP_HSIZE sizeof(struct icmphdr)

// 自定义 icmp 数据区
struct icmpdata {
	int tag;
	struct timeval sendtime;
};
#define ICMP_DATA_SIZE sizeof(struct icmpdata)

// 定义一个IP消息头部结构体
struct iphdr {
	u8 hlen : 4, ver : 4; // 定义4位首部长度，和IP版本号为IPV4
	u8 tos;               // 8位服务类型TOS
	u16 tot_len;          // 16位总长度
	u16 id;               // 16位标志位
	u16 frag_off;         // 3位标志位
	u8 ttl;               // 8位生存周期
	u8 protocol;          // 8位协议
	u16 check;            // 16位IP首部校验和
	u32 saddr;            // 32位源IP地址
	u32 daddr;            // 32位目的IP地址
};

#define IP_HSIZE sizeof(struct iphdr) // 定义IP_HSIZE为ip头部长度
#define IPVERSION 4                   // 定义IPVERSION为4，指出用ipv4
#define MAX_TTL 250

static GHashTable *ping_map = NULL; // map: <tag, ping_info_t>
static int sockfd = -1;
static pthread_rwlock_t rwlock;
static pthread_t send_thread_id = 0;
static pthread_t recv_thread_id = 0;
static int done = 0;

static u16 checksum(u8 *buf, int len)
{
	u32 sum = 0;
	u16 *cbuf = (u16 *)buf;

	while (len > 1) {
		sum += *cbuf++;
		len -= 2;
	}

	if (len) {
		sum += *(u8 *)cbuf;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return ~sum;
}

static int reset_socket()
{
	if (sockfd > 0) {
		close(sockfd);
		sockfd = -1;
	}

	if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
		CPDS_LOG_ERROR("Failed to create socket - %s", strerror(errno));
		return -1;
	}

	int on = 1;
	setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

	return 0;
}

static int send_ping(int tag, int seq, const char *hostname)
{
	struct sockaddr_in dest;      // 被ping的主机IP
	struct iphdr *ip_hdr;         // iphdr为IP头部结构体
	struct icmphdr *icmp_hdr;     // icmphdr为ICMP头部结构体
	char sendbuf[BUFSIZE];        // 发送字符串数组
	int datalen = ICMP_DATA_SIZE; // ICMP消息携带的数据长度
	int len;
	int ip_len;

	struct hostent *host = gethostbyname(hostname);
	if ((host) == NULL) {
		CPDS_LOG_WARN("Failed to gethostbyname - name:%s", hostname);
		return -1;
	}

	memset(&dest, 0, sizeof dest);
	dest.sin_family = PF_INET; // PF_INET为IPV4，internet协议，在<netinet/in.h>中，地址族
	dest.sin_port = ntohs(0);  // 端口号,ntohs()返回一个以主机字节顺序表达的数。
	dest.sin_addr = *(struct in_addr *)host->h_addr_list[0]; // host->h_addr_list[0]是地址的指针.返回IP地址，初始化

	// ip头部结构体变量初始化
	ip_hdr = (struct iphdr *)sendbuf;                  // 字符串指针
	ip_hdr->hlen = sizeof(struct iphdr) >> 2;          // 头部长度
	ip_hdr->ver = IPVERSION;                           // 版本
	ip_hdr->tos = 0;                                   // 服务类型
	ip_hdr->tot_len = IP_HSIZE + ICMP_HSIZE + datalen; // 报文头部加数据的总长度
	ip_hdr->id = 0;                                    // 初始化报文标识
	ip_hdr->frag_off = 0;                              // 设置flag标记为0
	ip_hdr->protocol = IPPROTO_ICMP;                   // 运用的协议为ICMP协议
	ip_hdr->ttl = MAX_TTL;                             // 一个封包在网络上可以存活的时间
	ip_hdr->daddr = dest.sin_addr.s_addr;              // 目的地址
	ip_len = ip_hdr->hlen << 2;                        // ip数据长度

	// icmp头部结构体变量初始化
	icmp_hdr = (struct icmphdr *)(sendbuf + ip_len); // 字符串指针
	icmp_hdr->type = 8;                              // 初始化ICMP消息类型type
	icmp_hdr->code = 0;                              // 初始化消息代码code
	icmp_hdr->icmp_id = tag;                         // 把tag标识码赋值给icmp_id
	icmp_hdr->icmp_seq = seq;                        // 发送的ICMP消息序号赋值给icmp序号

	// icmp数据区赋值
	memset(icmp_hdr->data, 0xff, datalen);
	struct icmpdata *data = (struct icmpdata *)icmp_hdr->data;
	data->tag = tag;
	gettimeofday(&data->sendtime, NULL); // 获取当前时间

	len = ip_hdr->tot_len;                              // 报文总长度赋值给len变量
	icmp_hdr->checksum = 0;                             // 初始化
	icmp_hdr->checksum = checksum((u8 *)icmp_hdr, len); // 计算校验和

	sendto(sockfd, sendbuf, len, 0, (struct sockaddr *)&dest, sizeof(dest));

	// CPDS_LOG_DEBUG(">>> ping send %s: tag:%d, seq:%d", hostname, data->tag, seq);

	return 0;
}

static void send_thread(void *arg)
{
	pthread_testcancel();
	while (done == 0) {

		sleep(2);

		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init(&iter, ping_map);
		while (1) {
			if (pthread_rwlock_wrlock(&rwlock) != 0)
				return;
			if (g_hash_table_iter_next(&iter, &key, &value) == FALSE) {
				pthread_rwlock_unlock(&rwlock);
				break;
			}
			if (value == NULL) {
				pthread_rwlock_unlock(&rwlock);
				continue;
			}
			
			ping_info_t *info = (ping_info_t *)value;
			if (info->dest == NULL) {
				pthread_rwlock_unlock(&rwlock);
				continue;
			}
			char *hostname = g_strdup(info->dest);
			int seq = info->send_cnt;
			info->send_cnt = info->send_cnt + 2;
			int tag = GPOINTER_TO_INT(key);
			pthread_rwlock_unlock(&rwlock);

			// 发送ping包
			send_ping(tag, seq + 1, hostname);
			send_ping(tag, seq + 2, hostname);

			g_free(hostname);

			g_usleep(2000);
		}
	}
}

static int recv_ping(char *recv_buf, int recv_len, struct sockaddr_in *from)
{
	struct iphdr *ip;
	struct icmphdr *icmp;
	struct icmpdata *data;
	int ip_hlen;
	u16 ip_datalen;
	int tag;

	struct timeval recvtime;
	gettimeofday(&recvtime, NULL); // 记录收到应答的时间

	if (recv_len < IP_HSIZE + ICMP_HSIZE + ICMP_DATA_SIZE) {
		CPDS_LOG_INFO("Receive size too short");
		return -1;
	}

	ip = (struct iphdr *)recv_buf;
	ip_hlen = ip->hlen << 2;
	ip_datalen = ntohs(ip->tot_len) - ip_hlen;
	icmp = (struct icmphdr *)(recv_buf + ip_hlen);

	if (checksum((u8 *)icmp, ip_datalen)) {
		CPDS_LOG_INFO("checksum fail");
		return -1;
	}

	if (ip->ttl >= MAX_TTL) {
		return -1;
	}

	data = (struct icmpdata *)icmp->data;
	tag = data->tag;

	if (pthread_rwlock_wrlock(&rwlock) != 0)
		return -1;

	// 查表，更新接收数据
	ping_info_t *pinfo = g_hash_table_lookup(ping_map, GINT_TO_POINTER(tag));
	if (pinfo == NULL) {
		pthread_rwlock_unlock(&rwlock);
		return -1;
	}
	double rtt = (recvtime.tv_sec - data->sendtime.tv_sec) + (recvtime.tv_usec - data->sendtime.tv_usec) / 1000000.0;
	pinfo->rtt += rtt;
	pinfo->recv_cnt++;

	// CPDS_LOG_DEBUG("<<< %d bytes from %s:icmp_seq=%u ttl=%d rtt=%.3f ms", ip_datalen, // IP数据长度
	//                inet_ntoa(from->sin_addr),                                         // 目的ip地址
	//                icmp->icmp_seq,                                                    // icmp报文序列号
	//                ip->ttl,                                                           // 生存时间
	//                rtt * 1000);                                                       // 往返时间

	pthread_rwlock_unlock(&rwlock);

	return 0;
}

static void recv_thread(void *arg)
{
	pthread_testcancel();

	int recv_len = 0;
	socklen_t addr_len;
	struct sockaddr_in from;
	char recv_buf[BUFSIZE] = {0};
	int inner_fail_count = 0;
	int outer_fail_count = 0;

	addr_len = sizeof(from);
	while (done == 0) {
		if ((recv_len = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&from, &addr_len)) < 0) {
			if (errno == EINTR) // EINTR表示信号中断
				continue;
			CPDS_LOG_ERROR("Receive ping response error - %s", strerror(errno));
			if (++inner_fail_count >= 3) {
				// 尝试重置socket
				CPDS_LOG_ERROR("Try to reset socket");
				reset_socket();
				inner_fail_count = 0;
				if (++outer_fail_count >= 3) {
					CPDS_LOG_ERROR("Failed to fix socket error");
					break;
				}
				continue;
			} else {
				continue;
			}
		}

		recv_ping(recv_buf, recv_len, &from);
	}
}

static void ping_info_destroy(gpointer data)
{
	ping_info_t *info = (ping_info_t *)data;
	if (info) {
		if (info->dest) {
			g_free(info->dest);
			info->dest = NULL;
		}
		g_free(info);
	}
}

int gen_ping_tag()
{
	static int tag = 0;
	return ++tag;
}

int init_ping_svc()
{
	int ret = -1;

	if (pthread_rwlock_init(&rwlock, NULL) != 0) {
		CPDS_LOG_ERROR("Failed to init pthread rwlock");
		return -1;
	}

	if (reset_socket() != 0) {
		return -1;
	}

	ping_map = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, ping_info_destroy);
	
	// 启动ping发送线程
	ret = pthread_create(&send_thread_id, NULL, (void *)send_thread, NULL);
	if (ret != 0) {
		CPDS_LOG_ERROR("Failed to create ping send thread - %s", strerror(errno));
		return -1;
	}

	// 启动ping接收线程
	ret = pthread_create(&recv_thread_id, NULL, (void *)recv_thread, NULL);
	if (ret != 0) {
		CPDS_LOG_ERROR("Failed to create ping recv thread - %s", strerror(errno));
		return -1;
	}

	return 0;
}

void destroy_ping_svc()
{
	void *status;

	if (sockfd > 0) {
		close(sockfd);
		sockfd = -1;
	}

	done = 1;

	if (send_thread_id > 0) {
		pthread_cancel(send_thread_id);
		pthread_join(send_thread_id, &status);
	}

	if (recv_thread_id > 0) {
		pthread_cancel(recv_thread_id);
		pthread_join(recv_thread_id, &status);
	}

	if (ping_map != NULL) {
		pthread_rwlock_wrlock(&rwlock);
		g_hash_table_destroy(ping_map);
		ping_map = NULL;
		pthread_rwlock_unlock(&rwlock);
	}

	pthread_rwlock_destroy(&rwlock);
}

void register_ping_item(int tag, const char *dest)
{
	if (ping_map == NULL) {
		CPDS_LOG_ERROR("register_ping_item ping_map NULL!!");
		return;
	}

	if (pthread_rwlock_wrlock(&rwlock) != 0)
		return;

	ping_info_t *pinfo = g_hash_table_lookup(ping_map, GINT_TO_POINTER(tag));
	if (pinfo != NULL) {
		CPDS_LOG_INFO("tag:%d already registered", tag);
		goto out;
	}
	ping_info_t *info = g_malloc0(sizeof(ping_info_t));
	info->dest = g_strdup(dest);
	g_hash_table_insert(ping_map, GINT_TO_POINTER(tag), info);
	CPDS_LOG_DEBUG("register ping tag: %d, host: %s", tag, info->dest);

out:
	pthread_rwlock_unlock(&rwlock);
}

void unregister_ping_item(int tag)
{
	if (ping_map == NULL) {
		CPDS_LOG_ERROR("unregister_ping_item ping_map NULL!!");
		return;
	}

	if (pthread_rwlock_wrlock(&rwlock) != 0)
		return;

	if (g_hash_table_remove(ping_map, GINT_TO_POINTER(tag)) == TRUE) {
		CPDS_LOG_DEBUG("unregister ping tag:%d", tag);
	}

	pthread_rwlock_unlock(&rwlock);
}

int get_ping_info(int tag, ping_info_t *info)
{
	if (info == NULL)
		return -1;

	int ret = -1;

	if (pthread_rwlock_wrlock(&rwlock) != 0)
		return -1;

	ping_info_t *pinfo = g_hash_table_lookup(ping_map, GINT_TO_POINTER(tag));
	if (pinfo != NULL) {
		info->send_cnt = pinfo->send_cnt;
		info->recv_cnt = pinfo->recv_cnt;
		info->rtt = pinfo->rtt;
	} else {
		goto out;
	}

	ret = 0;

out:
	pthread_rwlock_unlock(&rwlock);
	return ret;
}