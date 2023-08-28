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