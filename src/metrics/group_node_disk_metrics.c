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

#include "metric_group_type.h"
#include "prom.h"
#include "logger.h"
#include "json.h"

static void group_node_disk_init();
static void group_node_disk_destroy();
static void group_node_disk_update();

metric_group group_node_disk = {.name = "node_disk_group",
                              .update_period = 3,
                              .init = group_node_disk_init,
                              .destroy = group_node_disk_destroy,
                              .update = group_node_disk_update};

static prom_gauge_t *cpds_node_blk_total_bytes;
static prom_counter_t *cpds_node_disk_reads_completed_total;
static prom_counter_t *cpds_node_disk_reads_merged_total;
static prom_counter_t *cpds_node_disk_reads_sectors_total;
static prom_counter_t *cpds_node_disk_read_time_seconds_total;
static prom_counter_t *cpds_node_disk_writes_completed_total;
static prom_counter_t *cpds_node_disk_writes_merged_total;
static prom_counter_t *cpds_node_disk_writes_sectors_total;
static prom_counter_t *cpds_node_disk_write_time_seconds_total;
static prom_gauge_t *cpds_node_disk_io_now;
static prom_counter_t *cpds_node_disk_io_time_seconds_total;
static prom_counter_t *cpds_node_disk_io_time_weighted_seconds_total;
static prom_counter_t *cpds_node_disk_read_bytes_total;
static prom_counter_t *cpds_node_disk_written_bytes_total;
static prom_gauge_t *cpds_node_lvm_state;

static void group_node_disk_init()
{
	metric_group *grp = &group_node_disk;
	
	const char *blk_labels[] = {"name", "type", "mount"};
	size_t label_count = sizeof(blk_labels) / sizeof(blk_labels[0]);
	cpds_node_blk_total_bytes = prom_gauge_new("cpds_node_blk_total_bytes", "node block device total size in bytes", label_count, blk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_blk_total_bytes);

	const char *disk_labels[] = {"device"};
	label_count = sizeof(disk_labels) / sizeof(disk_labels[0]);
	cpds_node_disk_reads_completed_total = prom_counter_new("cpds_node_disk_reads_completed_total", "The total number of reads completed successfully.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_reads_completed_total);
	cpds_node_disk_reads_merged_total = prom_counter_new("cpds_node_disk_reads_merged_total", "The total number of reads merged.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_reads_merged_total);
	cpds_node_disk_reads_sectors_total = prom_counter_new("cpds_node_disk_reads_sectors_total", "The total number of sectors read successfully.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_reads_sectors_total);
	cpds_node_disk_read_time_seconds_total = prom_counter_new("cpds_node_disk_read_time_seconds_total", "This is the total number of seconds spent by all reads.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_read_time_seconds_total);
	cpds_node_disk_writes_completed_total = prom_counter_new("cpds_node_disk_writes_completed_total", "The total number of writes completed successfully.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_writes_completed_total);
	cpds_node_disk_writes_merged_total = prom_counter_new("cpds_node_disk_writes_merged_total", "The total number of writes merged.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_writes_merged_total);
	cpds_node_disk_writes_sectors_total = prom_counter_new("cpds_node_disk_writes_sectors_total", "The total number of sectors written successfully.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_writes_sectors_total);
	cpds_node_disk_write_time_seconds_total = prom_counter_new("cpds_node_disk_write_time_seconds_total", "This is the total number of seconds spent by all writes.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_write_time_seconds_total);
	cpds_node_disk_io_now = prom_gauge_new("cpds_node_disk_io_now", "The number of I/Os currently in progress.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_io_now);
	cpds_node_disk_io_time_seconds_total = prom_counter_new("cpds_node_disk_io_time_seconds_total", "Total seconds spent doing I/Os.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_io_time_seconds_total);
	cpds_node_disk_io_time_weighted_seconds_total = prom_counter_new("cpds_node_disk_io_time_weighted_seconds_total", "The weighted time of seconds spent doing I/Os.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_io_time_weighted_seconds_total);
	cpds_node_disk_read_bytes_total = prom_counter_new("cpds_node_disk_read_bytes_total", "The total bytes read successfully.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_read_bytes_total);
	cpds_node_disk_written_bytes_total = prom_counter_new("cpds_node_disk_written_bytes_total", "The total bytes written successfully.", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_disk_written_bytes_total);

	cpds_node_lvm_state = prom_gauge_new("cpds_node_lvm_state", "LVM state (1:active, 0:inactive)", label_count, disk_labels);
	grp->metrics = g_list_append(grp->metrics, cpds_node_lvm_state);
}

static void group_node_disk_destroy()
{
	if (group_node_disk.metrics)
		g_list_free(group_node_disk.metrics);
}

static int update_node_blk_metrics()
{
	int ret = -1;
	gchar *lsblk_output = NULL;
	cJSON *j_root = NULL;

	// 每次更新清理一下，避免残留已删除的指标项
	prom_gauge_clear(cpds_node_blk_total_bytes);

	const gchar *lsblk_cmd = "lsblk -lbJ -o NAME,TYPE,SIZE,MOUNTPOINT";
	if (g_spawn_command_line_sync(lsblk_cmd, &lsblk_output, NULL, NULL, NULL) == FALSE) {
		CPDS_LOG_ERROR("execute lsblk fail");
		goto out;
	}

	j_root = cJSON_Parse(lsblk_output);
	if (j_root == NULL) {
		CPDS_LOG_ERROR("parse lsblk output fail.");
		goto out;
	}

	cJSON *j_dev_arr = cJSON_GetObjectItem(j_root, "blockdevices");
	if (j_dev_arr == NULL || !cJSON_IsArray(j_dev_arr)) {
		CPDS_LOG_ERROR("get blockdevices fail");
		goto out;
	}

	int num = cJSON_GetArraySize(j_dev_arr);
	for (int i = 0; i < num; i++) {
		cJSON *j_dev = cJSON_GetArrayItem(j_dev_arr, i);
		char *name = cJSON_GetStringValue(cJSON_GetObjectItem(j_dev, "name"));
		char *type = cJSON_GetStringValue(cJSON_GetObjectItem(j_dev, "type"));
		char *mountpoint = cJSON_GetStringValue(cJSON_GetObjectItem(j_dev, "mountpoint"));
		double val = -1;
		cJSON *j_val = cJSON_GetObjectItem(j_dev, "size");
		if (j_val == NULL || !cJSON_IsNumber(j_val)) {
			CPDS_LOG_ERROR("get '%s' size fail", name);
			continue;
		}
		val = j_val->valuedouble;
		prom_gauge_set(cpds_node_blk_total_bytes, val, (const char *[]){name, type, mountpoint});
	}

	ret = 0;

out:
	if (lsblk_output)
		g_free(lsblk_output);
	if (j_root)
		cJSON_Delete(j_root);
	return ret;
}

int update_node_disk_metrics()
{
	char line[200] = {0};
	char device[10] = {0};
	int major, minor;
	long reads_completed_total;
	long reads_merged_total;
	long reads_sectors_total;
	long read_time_seconds_total;
	long writes_completed_total;
	long writes_merged_total;
	long writes_sectors_total;
	long write_time_seconds_total;
	long io_now;
	long io_time_seconds_total;
	long io_time_weighted_seconds_total;

	FILE *fp = fopen("/proc/diskstats", "r");
	if (!fp) {
		CPDS_LOG_ERROR("can't open '/proc/diskstats' - %s", strerror(errno));
		prom_counter_clear(cpds_node_disk_reads_completed_total);
		prom_counter_clear(cpds_node_disk_reads_completed_total);
		prom_counter_clear(cpds_node_disk_reads_merged_total);
		prom_counter_clear(cpds_node_disk_reads_sectors_total);
		prom_counter_clear(cpds_node_disk_read_time_seconds_total);
		prom_counter_clear(cpds_node_disk_writes_completed_total);
		prom_counter_clear(cpds_node_disk_writes_merged_total);
		prom_counter_clear(cpds_node_disk_writes_sectors_total);
		prom_counter_clear(cpds_node_disk_write_time_seconds_total);
		prom_gauge_clear(cpds_node_disk_io_now);
		prom_counter_clear(cpds_node_disk_io_time_seconds_total);
		prom_counter_clear(cpds_node_disk_io_time_weighted_seconds_total);
		prom_counter_clear(cpds_node_disk_read_bytes_total);
		prom_counter_clear(cpds_node_disk_written_bytes_total);
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		sscanf(line, "%d %d %s %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", &major, &minor, device,
		       &reads_completed_total, &reads_merged_total, &reads_sectors_total, &read_time_seconds_total,
		       &writes_completed_total, &writes_merged_total, &writes_sectors_total, &write_time_seconds_total, &io_now,
		       &io_time_seconds_total, &io_time_weighted_seconds_total);

		prom_counter_set(cpds_node_disk_reads_completed_total, reads_completed_total, (const char *[]){device});
		prom_counter_set(cpds_node_disk_reads_completed_total, reads_completed_total, (const char *[]){device});
		prom_counter_set(cpds_node_disk_reads_merged_total, reads_merged_total, (const char *[]){device});
		prom_counter_set(cpds_node_disk_reads_sectors_total, reads_sectors_total, (const char *[]){device});
		prom_counter_set(cpds_node_disk_read_time_seconds_total, read_time_seconds_total, (const char *[]){device});
		prom_counter_set(cpds_node_disk_writes_completed_total, writes_completed_total, (const char *[]){device});
		prom_counter_set(cpds_node_disk_writes_merged_total, writes_merged_total, (const char *[]){device});
		prom_counter_set(cpds_node_disk_writes_sectors_total, writes_sectors_total, (const char *[]){device});
		prom_counter_set(cpds_node_disk_write_time_seconds_total, write_time_seconds_total, (const char *[]){device});
		prom_gauge_set(cpds_node_disk_io_now, io_now, (const char *[]){device});
		prom_counter_set(cpds_node_disk_io_time_seconds_total, io_time_seconds_total, (const char *[]){device});
		prom_counter_set(cpds_node_disk_io_time_weighted_seconds_total, io_time_weighted_seconds_total, (const char *[]){device});
		prom_counter_set(cpds_node_disk_read_bytes_total, reads_sectors_total * 512, (const char *[]){device});
		prom_counter_set(cpds_node_disk_written_bytes_total, writes_sectors_total * 512, (const char *[]){device});
	}
	fclose(fp);
	return 0;
}

static void strip_quot(char str[], int len)
{
	int i, j;
	for (i = 0; i < len; i++) {
		if (str[i] == '\'') {
			for (j = i; j < len; j++) {
				str[j] = str[j + 1];
			}
			len--;
		}
	}
}

static void update_node_lvm_state_metrics()
{
	gchar *cmd_out = NULL;
	gchar *cmd_err = NULL;
	GError *gerr = NULL;
	char **line_arr = NULL;

	prom_gauge_clear(cpds_node_lvm_state);

	if (g_spawn_command_line_sync("lvscan", &cmd_out, &cmd_err, NULL, &gerr) == FALSE) {
		CPDS_LOG_WARN("Failed to exe lvscan. - %s", gerr->message);
		goto out;
	}

	line_arr = g_strsplit(cmd_out, "\n", -1);
	if (line_arr == NULL)
		goto out;

	int i = 0;
	while (line_arr[i] != NULL) {
		char state[20];
		char device[200];
		sscanf(line_arr[i], "%s %s", state, device);
		strip_quot(device, strlen(device));
		if (g_strcmp0(state, "ACTIVE") == 0)
			prom_gauge_set(cpds_node_lvm_state, 1, (const char *[]){device});
		else
			prom_gauge_set(cpds_node_lvm_state, 0, (const char *[]){device});
		i++;
	}

out:
	if (line_arr)
		g_strfreev(line_arr);
	if (cmd_out)
		g_free(cmd_out);
	if (gerr)
		g_error_free(gerr);
}

static void group_node_disk_update()
{
	update_node_blk_metrics();
	update_node_disk_metrics();
	update_node_lvm_state_metrics();
}
