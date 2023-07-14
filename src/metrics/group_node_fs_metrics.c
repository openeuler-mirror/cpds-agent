#include "metric_group_type.h"
#include "prom.h"
#include "logger.h"
#include "json.h"

#include <mntent.h>
#include <stdio.h>
#include <string.h>
#include <sys/vfs.h>

static void group_node_fs_init();
static void group_node_fs_destroy();
static void group_node_fs_update();

metric_group group_node_fs = {.name = "node_fs_group",
                              .update_period = 3,
                              .init = group_node_fs_init,
                              .destroy = group_node_fs_destroy,
                              .update = group_node_fs_update};

static prom_gauge_t *cpds_node_fs_total_bytes;
static prom_gauge_t *cpds_node_fs_usage_bytes;

static void group_node_fs_init()
{
	metric_group *grp = &group_node_fs;

	const char *labels[] = {"fs", "mount"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_node_fs_total_bytes = prom_gauge_new("cpds_node_fs_total_bytes", "node filesystem total size in bytes", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_fs_total_bytes);
	cpds_node_fs_usage_bytes = prom_gauge_new("cpds_node_fs_usage_bytes", "node filesystem used size in bytes", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_fs_usage_bytes);
}

static void group_node_fs_destroy()
{
	if (group_node_fs.metrics)
		g_list_free(group_node_fs.metrics);
}

int update_node_fs_metrics()
{
	FILE *mount_table;
	struct mntent *mount_entry;
	struct statfs s;

	// 每次更新清理一下，避免残留已删除的指标项
	prom_gauge_clear(cpds_node_fs_total_bytes);
	prom_gauge_clear(cpds_node_fs_usage_bytes);

	mount_table = setmntent("/etc/mtab", "r");
	if (!mount_table) {
		CPDS_LOG_ERROR("set mount entry error. '%s'", strerror(errno));
		return -1;
	}

	while (1) {
		const char *device;
		const char *mount_point;
		if (mount_table) {
			mount_entry = getmntent(mount_table);
			if (!mount_entry) {
				endmntent(mount_table);
				break;
			}
		} else
			continue;
		device = mount_entry->mnt_fsname;
		mount_point = mount_entry->mnt_dir;
		if (statfs(mount_point, &s) != 0) {
			CPDS_LOG_WARN("statfs failed! - '%s'", strerror(errno));
			continue;
		}
		if ((s.f_blocks > 0) || !mount_table) {
			double total = s.f_blocks * s.f_bsize;
			double usage = (s.f_blocks - s.f_bfree) * s.f_bsize;
			prom_gauge_set(cpds_node_fs_total_bytes, total, (const char *[]){device, mount_point});
			prom_gauge_set(cpds_node_fs_usage_bytes, usage, (const char *[]){device, mount_point});
		}
	}
	return 0;
}

static void group_node_fs_update()
{
	update_node_fs_metrics();
}
