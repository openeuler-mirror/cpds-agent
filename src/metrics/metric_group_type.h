#ifndef _METRIC_GROUP_TYPE_H_
#define _METRIC_GROUP_TYPE_H_

#include <glib.h>

typedef GList metric_list;

// 一个metric group包含多个同类的metrics
struct _metric_group {
	char *name;
	metric_list *metrics;
	int update_period; // 更新周期，单位s
	void (*init)();
	void (*destroy)();
	void (*update)(); // 注意：update函数内部不能长时间阻塞！
};
typedef struct _metric_group metric_group;

typedef GList metric_group_list;

#endif