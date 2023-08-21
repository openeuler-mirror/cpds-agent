#ifndef _PING_H_
#define _PING_H_

typedef struct _ping_info {
	char *dest;   // ping 主机名
	int send_cnt; // 发送次数
	int recv_cnt; // 接收次数
	double rtt;   // 往返时间(s)
} ping_info_t;

int gen_ping_tag();
int init_ping_svc();
void destroy_ping_svc();
void register_ping_item(int tag, const char *dest);
void unregister_ping_item(int tag);
int get_ping_info(int tag, ping_info_t *info);

#endif