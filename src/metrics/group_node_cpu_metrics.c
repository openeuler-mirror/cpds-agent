#include "metric_group_type.h"
#include "prom.h"
#include "logger.h"

#include <stdio.h>
#include <unistd.h>

static void group_node_cpu_init();
static void group_node_cpu_destroy();
static void group_node_cpu_update();

metric_group group_node_cpu = {.name = "node_cpu_group",
                               .update_period = 1,
                               .init = group_node_cpu_init,
                               .destroy = group_node_cpu_destroy,
                               .update = group_node_cpu_update};

static prom_counter_t *cpds_node_cpu_seconds_total;

static void group_node_cpu_init()
{
	metric_group *grp = &group_node_cpu;
	const char *labels[] = {"cpu", "mode"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_node_cpu_seconds_total = prom_counter_new("cpds_node_cpu_seconds_total", "total seconds the cpus spent in each mode.", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_cpu_seconds_total);
}

static void group_node_cpu_destroy()
{
	if (group_node_cpu.metrics)
		g_list_free(group_node_cpu.metrics);
}

static int update_node_cpu_seconds_metrics()
{
	int ret = -1;
	FILE *fp = NULL;
	char line[100];
	char cpu_name[10] = {0};
	int cpu_user, cpu_nice, cpu_system, cpu_idle, cpu_iowait, cpu_irq, cpu_softirq;

	long int hz = sysconf(_SC_CLK_TCK);
	if (hz <= 0) {
		CPDS_LOG_ERROR("get CLK_TCK error. - %s", strerror(errno));
		goto out;
	}

	const char *proc_stat_file = "/proc/stat";
	fp = fopen(proc_stat_file, "r");
	if (!fp) {
		CPDS_LOG_ERROR("can't open '/proc/stat' - %s", strerror(errno));
		goto out;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strncmp(line, "cpu", 3) != 0)
			break;
		sscanf(line, "%s %d %d %d %d %d %d %d", cpu_name, &cpu_user, &cpu_nice, &cpu_system, &cpu_idle, &cpu_iowait,&cpu_irq, &cpu_softirq);
		prom_counter_set(cpds_node_cpu_seconds_total, (double)cpu_user / hz, (const char *[]){cpu_name, "user"});
		prom_counter_set(cpds_node_cpu_seconds_total, (double)cpu_nice / hz, (const char *[]){cpu_name, "nice"});
		prom_counter_set(cpds_node_cpu_seconds_total, (double)cpu_system / hz, (const char *[]){cpu_name, "system"});
		prom_counter_set(cpds_node_cpu_seconds_total, (double)cpu_idle / hz, (const char *[]){cpu_name, "idle"});
		prom_counter_set(cpds_node_cpu_seconds_total, (double)cpu_iowait / hz, (const char *[]){cpu_name, "iowait"});
		prom_counter_set(cpds_node_cpu_seconds_total, (double)cpu_irq / hz, (const char *[]){cpu_name, "irq"});
		prom_counter_set(cpds_node_cpu_seconds_total, (double)cpu_softirq / hz, (const char *[]){cpu_name, "softirq"});
	}

	ret = 0;

out:
	// 异常时清除 metric，不上报
	if (ret != 0)
		prom_counter_clear(cpds_node_cpu_seconds_total);

	fclose(fp);
	return ret;
}

static void group_node_cpu_update()
{
	update_node_cpu_seconds_metrics();
}