#include "container_collector.h"
#include "bpf_stat.h"
#include "logger.h"

#include <glib.h>
#include <sys/sysinfo.h>

typedef struct _process_stat {
	int pid;
	int zombie_flag;
} process_stat_t;

typedef struct _memory_stat {
	unsigned long total;
	unsigned long usage;
	unsigned long swap_total;
	unsigned long swap_usage;
	unsigned long cached;
} memory_stat_t;

typedef struct _net_dev_stat_t {
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
} net_dev_stat_t;

typedef struct _delay_info {
	int tid;
	unsigned long long delayacct_blkio_ticks;
} delay_info_t;

typedef struct _container_info {
	char *cid;                   // container id
	int pid;                     // container main pid
	char *status;                // container status
	int exit_code;               // container exit code
	unsigned long disk_usage;    // unit: bytes
	unsigned long cpu_usage_ns;  // uint; ns
	unsigned long long disk_iodelay; // unit: ticks
	GHashTable *iodelay_map;         // map (tid, delay_info_t)
	memory_stat_t memory_stat;   // memory related stats
	perf_stat_t perf_stat;       // performance stats
	GList *net_dev_stat_list;    // list of net_dev_stat_t
	GList *process_stat_list;    // list of process_stat_t
} container_info_t;

static pthread_rwlock_t rwlock;
static pthread_t update_thread_id = 0;
static int done = 0;

// 容器信息存储在hash表中。key为容器id
static GHashTable *cmap = NULL;

// 输入xxxB/kB/MB/GB，返回以B为单位的数据
static unsigned long str_to_bytes(const char *str)
{
	guint64 bytes = 0;
	gchar *suffix = NULL; // 后缀字符串
	gdouble value = g_ascii_strtod(str, &suffix);
	if (suffix) {
		if (g_ascii_strncasecmp(suffix, "B", 1) == 0) {
			bytes = (guint64)(value);
		} else if (g_ascii_strncasecmp(suffix, "kB", 2) == 0 || g_ascii_strncasecmp(suffix, "KB", 2) == 0) {
			bytes = (guint64)(value * 1024);
		} else if (g_ascii_strncasecmp(suffix, "mB", 2) == 0 || g_ascii_strncasecmp(suffix, "MB", 2) == 0) {
			bytes = (guint64)(value * 1024 * 1024);
		} else if (g_ascii_strncasecmp(suffix, "gB", 2) == 0 || g_ascii_strncasecmp(suffix, "GB", 2) == 0) {
			bytes = (guint64)(value * 1024 * 1024 * 1024);
		}
	}
	return (unsigned long)bytes;
}

typedef struct _container_cgroup_dirs {
	char cgroup_cpu_dir[300];
	char cgroup_memory_dir[300];
	char cgroup_blkio_dir[300];
} container_cgroup_dirs;

static int get_container_cgroup_dirs(int pid, container_cgroup_dirs *ccgd)
{
	int ret = -1;

	FILE *fp = NULL;
	char pcg_file[50] = {0};
	char line[300] = {0};

	g_snprintf(pcg_file, sizeof(pcg_file), "/proc/%d/cgroup", pid);
	fp = fopen(pcg_file, "r");
	if (!fp) {
		CPDS_LOG_ERROR("Failed to read file: %s - '%s'", pcg_file, strerror(errno));
		goto out;
	}

	while (fgets(line, sizeof(line), fp)) {
		g_strstrip(line);
		char **arr = g_strsplit(line, ":", -1);
		int num = g_strv_length(arr);
		if (num < 3)
			continue;
		if (g_strcmp0("cpu,cpuacct", arr[1]) == 0) 
			g_snprintf(ccgd->cgroup_cpu_dir, sizeof(ccgd->cgroup_cpu_dir), "/sys/fs/cgroup/cpu,cpuacct%s", arr[2]);
		else if (g_strcmp0("memory", arr[1]) == 0)
			g_snprintf(ccgd->cgroup_memory_dir, sizeof(ccgd->cgroup_memory_dir), "/sys/fs/cgroup/memory%s", arr[2]);
		else if (g_strcmp0("blkio", arr[1]) == 0)
			g_snprintf(ccgd->cgroup_blkio_dir, sizeof(ccgd->cgroup_blkio_dir), "/sys/fs/cgroup/blkio%s", arr[2]);
		g_strfreev(arr);
	}

	ret = 0;
out:
	if (fp)
		fclose(fp);
	return ret;
}

static void get_cpu_usage(const char *cgroup_cpu_dir, unsigned long *usage)
{
	if (cgroup_cpu_dir == NULL || usage == NULL)
		return;

	char cgfile[300] = {0};
	char *cg_cpu_usage = NULL;
	g_snprintf(cgfile, sizeof(cgfile), "%s/cpuacct.usage", cgroup_cpu_dir);
	if (g_file_get_contents(cgfile, &cg_cpu_usage, NULL, NULL) == TRUE) {
		g_strstrip(cg_cpu_usage);
		*usage = g_ascii_strtoull(cg_cpu_usage, NULL, 10);
		g_free(cg_cpu_usage);
	}
}

static void get_memory_stat(const char *cgroup_memory_dir, memory_stat_t *ms)
{
	if (cgroup_memory_dir == NULL || ms == NULL)
		return;

	struct sysinfo s_info = {0};
	if (sysinfo(&s_info) != 0)
		return;

	int no_limit = 0;
	char cgfile[300] = {0};

	char *cg_limit_in_bytes = NULL;
	unsigned long limit_in_bytes = 0;
	g_snprintf(cgfile, sizeof(cgfile), "%s/memory.limit_in_bytes", cgroup_memory_dir);
	if (g_file_get_contents(cgfile, &cg_limit_in_bytes, NULL, NULL) == TRUE) {
		g_strstrip(cg_limit_in_bytes);
		limit_in_bytes = g_ascii_strtoull(cg_limit_in_bytes, NULL, 10);
		g_free(cg_limit_in_bytes);
		// 容器若没做内存限制，则总内存不超过宿主机总内存
		if (limit_in_bytes > s_info.totalram) {
			no_limit = 1;
			ms->total = s_info.totalram;
		} else {
			ms->total = limit_in_bytes;
		}
	}

	char *cg_usage_in_bytes = NULL;
	g_snprintf(cgfile, sizeof(cgfile), "%s/memory.usage_in_bytes", cgroup_memory_dir);
	if (g_file_get_contents(cgfile, &cg_usage_in_bytes, NULL, NULL) == TRUE) {
		g_strstrip(cg_usage_in_bytes);
		ms->usage = g_ascii_strtoull(cg_usage_in_bytes, NULL, 10);
		g_free(cg_usage_in_bytes);
	}

	if (no_limit == 1) {
		// 容器若没做内存限制，则swap内存使用宿主机系统swap内存
		ms->swap_total = s_info.totalswap;
	} else {
		char *cg_memsw_limit_in_bytes = NULL;
		unsigned long memsw_limit_in_bytes = 0;
		g_snprintf(cgfile, sizeof(cgfile), "%s/memory.memsw.limit_in_bytes", cgroup_memory_dir);
		if (g_file_get_contents(cgfile, &cg_memsw_limit_in_bytes, NULL, NULL) == TRUE) {
			g_strstrip(cg_memsw_limit_in_bytes);
			memsw_limit_in_bytes = g_ascii_strtoull(cg_memsw_limit_in_bytes, NULL, 10);
			ms->swap_total = memsw_limit_in_bytes - limit_in_bytes;
			g_free(cg_memsw_limit_in_bytes);
		}
	}

	char *cg_memory_stat = NULL;
	g_snprintf(cgfile, sizeof(cgfile), "%s/memory.stat", cgroup_memory_dir);
	if (g_file_get_contents(cgfile, &cg_memory_stat, NULL, NULL) == TRUE) {
		g_strstrip(cg_memory_stat);
		char **stat_arr = g_strsplit(cg_memory_stat, "\n", -1);
		g_free(cg_memory_stat);
		int num = g_strv_length(stat_arr);
		const char *str_total_swap = "total_swap";
		int str_total_swap_len = strlen(str_total_swap);
		const char *str_total_cache = "total_cache";
		int str_total_cache_len = strlen(str_total_cache);
		for (int i = 0; i < num; i++) {
			if (g_ascii_strncasecmp(str_total_swap, stat_arr[i], str_total_swap_len) == 0) {
				ms->swap_usage = g_ascii_strtoull(stat_arr[i] + str_total_swap_len, NULL, 10);
			}
			if (g_ascii_strncasecmp(str_total_cache, stat_arr[i], str_total_cache_len) == 0) {
				ms->cached = g_ascii_strtoull(stat_arr[i] + str_total_cache_len, NULL, 10);
			}
		}
		g_strfreev(stat_arr);
	}
}

static GList *fill_net_dev_stat_list(int pid, GList *plist)
{
	FILE *fp = NULL;
	char line[500];
	char *proc_file = NULL;

	// 先清空列表
	GList *iter = plist;
	while (iter != NULL) {
		g_free(iter->data);
		plist = g_list_delete_link(plist, iter);
		iter = plist;
	}
	
	if (pid <= 0)
		goto out;

	proc_file = g_strdup_printf("/proc/%d/net/dev", pid);
	fp = fopen(proc_file, "r");
	if (fp == NULL) {
		goto out;
	}

	int i = 0;
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (i++ < 2) // 跳过前两行
			continue;
		net_dev_stat_t *nds = g_malloc0(sizeof(net_dev_stat_t));
		sscanf(line, "%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", nds->ifname,
		       &nds->r_bytes, &nds->r_packets, &nds->r_errs, &nds->r_drop, &nds->r_fifo, &nds->r_frame,
		       &nds->r_compressed, &nds->r_multicast, &nds->t_bytes, &nds->t_packets, &nds->t_errs, &nds->t_drop,
		       &nds->t_fifo, &nds->t_colls, &nds->t_carrier, &nds->t_compressed);
		char *p = strtok(nds->ifname, ":");
		if (p)
			plist = g_list_append(plist, nds);
		else
			g_free(nds);
	}

out:
	if (proc_file)
		g_free(proc_file);
	if (fp)
		fclose(fp);
	return plist;
}

// 遍历进程及其子进程获取 process_stat_t 列表
static GList *traverse_fill_process_stat_list(int pid, GList *plist)
{
	char full_path[200] = {0};
	FILE *stat_fp = NULL;
	char state;
	process_stat_t *pstat = NULL;
	GDir *task_dir = NULL;
	const char *task_name = NULL;
	FILE *children_fp = NULL;
	int child_pid = 0;

	if (pid <= 0)
		goto out;

	g_snprintf(full_path, sizeof(full_path), "/proc/%d/stat", pid);
	stat_fp = fopen(full_path, "r");
	if (stat_fp == NULL)
		goto out;

	pstat = g_malloc0(sizeof(process_stat_t));
	if (pstat == NULL) {
		CPDS_LOG_ERROR("Failed to allocate memory");
		goto out;
	}
	pstat->pid = pid;
	pstat->zombie_flag = 0;

	fscanf(stat_fp, "%*s %*s %c", &state);
	if (state == 'Z')
		pstat->zombie_flag = 1;

	plist = g_list_append(plist, pstat);

	g_snprintf(full_path, sizeof(full_path), "/proc/%d/task", pid);
	task_dir = g_dir_open(full_path, 0, NULL);
	if (task_dir == NULL) {
		goto out;
	}

	// 遍历子进程
	while ((task_name = g_dir_read_name(task_dir)) != NULL) {
		g_snprintf(full_path, sizeof(full_path), "/proc/%d/task/%s/children", pid, task_name);
		children_fp = fopen(full_path, "r");
		if (children_fp == NULL)
			continue;
		while (fscanf(children_fp, "%d", &child_pid) == 1) {
			plist = traverse_fill_process_stat_list(child_pid, plist);
		}
		if (children_fp) {
			fclose(children_fp);
			children_fp = NULL;
		}
	}

out:
	if (stat_fp)
		fclose(stat_fp);
	if (task_dir)
		g_dir_close(task_dir);
	return plist;
}

static GList *fill_process_stat_list(int pid, GList *plist)
{
	// 先清空列表
	GList *iter = plist;
	while (iter != NULL) {
		g_free(iter->data);
		plist = g_list_delete_link(plist, iter);
		iter = plist;
	}

	// 遍历进程及其子进程获取 process_stat_t 列表
	plist = traverse_fill_process_stat_list(pid, plist);
	return plist;
}

static void iodelay_value_destroy(gpointer data)
{
	delay_info_t *info = (delay_info_t *)data;
	if (info) {
		g_free(info);
	}
}

static void get_disk_iodelay(const char *cgroup_blkio_dir, container_info_t *info)
{
	char full_path[260] = {0};
	FILE *fp = NULL;
	FILE *stat_fp = NULL;
	int tid = 0;
	unsigned long long delayacct_blkio_ticks = 0;
	unsigned long long previous_delayacct_blkio_ticks = 0;
	unsigned long long thread_iodelay = 0;
	unsigned long long max_iodelay = 0;

	g_snprintf(full_path, sizeof(full_path), "%s/tasks", cgroup_blkio_dir);
	fp = fopen(full_path, "r");
	if (fp == NULL)
		goto out;

	GHashTable *iodelay_map = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, iodelay_value_destroy);

	while (fscanf(fp, "%d", &tid) == 1) {
		g_snprintf(full_path, sizeof(full_path), "/proc/%d/task/%d/stat", tid, tid);
		stat_fp = fopen(full_path, "r");
		if (stat_fp == NULL)
			continue;
		// 第42个字段是delayacct_blkio_ticks
		int r = fscanf(stat_fp,
		               "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
		               "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
		               "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
		               "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
		               "%*s %llu",
		               &delayacct_blkio_ticks);
		if (r == 1) {
			delay_info_t *delay_info = g_malloc0(sizeof(delay_info_t));
			delay_info->tid = tid;
			delay_info->delayacct_blkio_ticks = delayacct_blkio_ticks;
			g_hash_table_insert(iodelay_map, &tid, delay_info);

			if (info->iodelay_map != NULL) {
				delay_info_t *previous_delay_info = g_hash_table_lookup(info->iodelay_map, &tid);
				if (previous_delay_info != NULL) {
					previous_delayacct_blkio_ticks = previous_delay_info->delayacct_blkio_ticks;
				} else {
					previous_delayacct_blkio_ticks = 0;
				}
			} else {
				previous_delayacct_blkio_ticks = 0;
			}

			thread_iodelay = delayacct_blkio_ticks - previous_delayacct_blkio_ticks;
			max_iodelay = thread_iodelay > max_iodelay ? thread_iodelay : max_iodelay;
		}
		fclose(stat_fp);
	}

	if (info->iodelay_map)
		g_hash_table_destroy(info->iodelay_map);
	info->iodelay_map = iodelay_map;
	info->disk_iodelay += max_iodelay;

out:
	if (fp)
		fclose(fp);
}

#define RESET_STRING(_old, _new)                            \
	if (_old == NULL)                                       \
		_old = g_strdup(_new);                              \
	else if (g_strcmp0(_old, _new) != 0) {                  \
		g_free(_old);                                       \
		_old = g_strdup(_new);                              \
	}

static void fill_container_info(char *cid, container_info_t *info)
{
	if (cid == NULL || info == NULL) {
		CPDS_LOG_ERROR("Para error");
		return;
	}

	gchar *cmd = NULL;
	gchar *cmd_ret_str = NULL;
	gchar **cmd_ret_array = NULL;

	// 使用 docker inspect 获取信息
	cmd = g_strdup_printf("docker inspect --format \"{{.State.Pid}} {{.State.Status}} {{.State.ExitCode}}\" %s", cid);
	if (g_spawn_command_line_sync(cmd, &cmd_ret_str, NULL, NULL, NULL) == FALSE) {
		CPDS_LOG_ERROR("Failed to exe docker inspect");
		goto out;
	}
	g_strstrip(cmd_ret_str);

	cmd_ret_array = g_strsplit(cmd_ret_str, " ", -1);
	if (g_strv_length(cmd_ret_array) < 3) {
		CPDS_LOG_ERROR("Failed to parse docker inspect");
		goto out;
	}

	RESET_STRING(info->cid, cid);
	info->pid = (int)g_ascii_strtoll(cmd_ret_array[0], NULL, 10);
	RESET_STRING(info->status, cmd_ret_array[1]);
	info->exit_code = (int)g_ascii_strtoll(cmd_ret_array[2], NULL, 10);
	// 以下统计信息在容器运行起来（进程pid有效）时才有意义
	if (info->pid > 0) {
		container_cgroup_dirs ccgd = {0};
		if (get_container_cgroup_dirs(info->pid, &ccgd) == 0) {
			get_cpu_usage(ccgd.cgroup_cpu_dir, &info->cpu_usage_ns);
			get_memory_stat(ccgd.cgroup_memory_dir, &info->memory_stat);
			get_disk_iodelay(ccgd.cgroup_blkio_dir, info);
		}
		get_perf_stat(info->pid, &info->perf_stat);
		info->net_dev_stat_list = fill_net_dev_stat_list(info->pid, info->net_dev_stat_list);
		info->process_stat_list = fill_process_stat_list(info->pid, info->process_stat_list);
	}

out:
	if (cmd)
		g_free(cmd);
	if (cmd_ret_str)
		g_free(cmd_ret_str);
	if (cmd_ret_array)
		g_strfreev(cmd_ret_array);
}

/*
更新eBPF监控表，使得内核bpf程序可以匹配抓取所需信息
两个核心表（bpf maps）：
1）perf_stat_map：key是容器的主pid，bpf程序会将容器中的所有进程的性能统计信息汇总后填入
2）pid_monitor_map：key是容器内部所有pid，bpf程序依据这些pid对执行过程的特定内核函数调用进行数据抓取
*/
static void do_update_bpf_monitor_map()
{
	int cpid_arr[MAX_STAT_MAP_SIZE] = {0};
	int cpid_num = 0;
	monitor_process_info mpi_arr[MAX_STAT_MAP_SIZE] = {0};
	int mpinfo_num = 0;

	GHashTableIter iter;
	gpointer key, value;
	g_hash_table_iter_init(&iter, cmap);
	while (g_hash_table_iter_next(&iter, &key, &value) == TRUE) {
		container_info_t *cinfo = (container_info_t *)value;
		cpid_arr[cpid_num++] = cinfo->pid;
		if (cpid_num >= MAX_STAT_MAP_SIZE)
			goto oversize;
		for (GList *ls = g_list_first(cinfo->process_stat_list); ls != NULL; ls = g_list_next(ls)) {
			process_stat_t *p = (process_stat_t *)ls->data;
			mpi_arr[mpinfo_num].pid = p->pid;
			mpi_arr[mpinfo_num].container_pid = cinfo->pid;
			mpinfo_num++;
			if (mpinfo_num >= MAX_STAT_MAP_SIZE)
				goto oversize;
		}
	}

	// 将需要监控的信息设置到内核bpf map中
	set_perf_stat_pid_list(cpid_arr, cpid_num);
	set_process_monitor_list(mpi_arr, mpinfo_num);

	return;

oversize:
	CPDS_LOG_ERROR("Too many pids to monitor.");
}

void dump_container_info()
{
	GHashTableIter iter;
	gpointer key, value;
	g_hash_table_iter_init(&iter, cmap);
	CPDS_LOG_DEBUG("*************** dump container info ***************");
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		char *cid = (char *)key;
		container_info_t *cinfo = (container_info_t *)value;
		perf_stat_t *ps = &cinfo->perf_stat;
		CPDS_LOG_DEBUG("- [cpid:%d] cnt=%lu, fail=%lu, size=%llu, time=%llu", cinfo->pid, ps->total_mmap_count,
		               ps->total_mmap_fail_count, ps->total_mmap_size, ps->total_mmap_time_ns);
		CPDS_LOG_DEBUG("%s %d %s", cid, cinfo->pid, cinfo->status);
		for (GList *ls = g_list_first(cinfo->process_stat_list); ls != NULL; ls = g_list_next(ls)) {
			process_stat_t *p = (process_stat_t *)ls->data;
			CPDS_LOG_DEBUG("    [%d] Z:%d", p->pid, p->zombie_flag);
		}
	}
}

static void do_update_info()
{
	if (pthread_rwlock_wrlock(&rwlock) != 0)
		goto out;

	// docker.service 服务不在线则不更新容器信息
	gchar *docker_active = NULL;
	if (g_spawn_command_line_sync("systemctl is-active docker.service", &docker_active, NULL, NULL, NULL) == FALSE) {
		goto out;
	}
	if (docker_active == NULL)
		goto out;
	g_strstrip(docker_active);
	if (g_strcmp0(docker_active, "active") != 0) {
		g_free(docker_active);
		goto out;
	}
	g_free(docker_active);

	// 获取当前容器id列表
	gchar *id_list_str = NULL;
	const char *cmd = "docker ps -a --no-trunc --format \"{{.ID}} {{.Size}}\"";
	if (g_spawn_command_line_sync(cmd, &id_list_str, NULL, NULL, NULL) == FALSE) {
		CPDS_LOG_ERROR("Failed to exe docker ps to get container cid");
		goto out;
	}
	g_strstrip(id_list_str);
	char **cid_array = g_strsplit(id_list_str, "\n", -1);
	g_free(id_list_str);

	/*
	遍历已保存的容器信息hash表，
	1) 匹配到当前容器id则更新，没有则从表中移除；
	2) 标记当前容器id列表中已匹配到的id (未标记的将被插入到hash表中)；
	*/
	GHashTableIter iter;
	gpointer key, value;
	int cnum = g_strv_length(cid_array);
	GArray *id_found_flags = g_array_sized_new(FALSE, TRUE, sizeof(int), cnum); // 标记已匹配到的容器id
	g_array_set_size(id_found_flags, cnum);
	g_hash_table_iter_init(&iter, cmap);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		char *cid = (char *)key;
		int i = 0;
		for (i = 0; i < cnum; i++) {
			if (g_ascii_strncasecmp(cid, cid_array[i], strlen(cid)) == 0) {
				g_array_index(id_found_flags, int, i) = 1;
				container_info_t *info = (container_info_t *)value;
				char *str_disk_usage = g_strrstr(cid_array[i], " ");
				if (str_disk_usage)
					info->disk_usage = str_to_bytes(str_disk_usage);
				else
					info->disk_usage = 0;
				fill_container_info(cid, info);
				break;
			}
		}
		if (i >= cnum) {
			g_hash_table_iter_remove(&iter);
		}
	}

	// hash表中插入未标记的容器信息
	for (int i = 0; i < id_found_flags->len; i++) {
		if (g_array_index(id_found_flags, int, i) != 1) {
			container_info_t *info = g_malloc0(sizeof(container_info_t));
			char *str_disk_usage = g_strrstr(cid_array[i], " ");
			if (str_disk_usage)
				info->disk_usage = str_to_bytes(str_disk_usage);
			else
				info->disk_usage = 0;
			char cid[100];
			if (sscanf(cid_array[i], "%s ", cid) == 1) {
				fill_container_info(cid, info);
				g_hash_table_insert(cmap, g_strdup(cid), info);
			}
		}
	}

	g_strfreev(cid_array);
	g_array_free(id_found_flags, TRUE);

	// 更新eBPF监控表
	do_update_bpf_monitor_map();

	// dump_container_info();

out:
	pthread_rwlock_unlock(&rwlock);
}

static void update_thread(void *arg)
{
	pthread_testcancel();

	while (done == 0) {
		g_usleep(1000000);
		do_update_info();
	};
}

static void cmap_key_destroy(gpointer data)
{
	g_free(data);
}

static void cmap_value_destroy(gpointer data)
{
	container_info_t *info = (container_info_t *)data;
	if (info) {
		if (info->cid) {
			g_free(info->cid);
			info->cid = NULL;
		}
		if (info->status) {
			g_free(info->status);
			info->status = NULL;
		}
		if (info->net_dev_stat_list) {
			for (GList *ls = g_list_first(info->net_dev_stat_list); ls != NULL; ls = g_list_next(ls)) {
				g_free(ls->data);
				ls->data = NULL;
			}
			g_list_free(info->net_dev_stat_list);
			info->net_dev_stat_list = NULL;
		}
		if (info->process_stat_list) {
			for (GList *ls = g_list_first(info->process_stat_list); ls != NULL; ls = g_list_next(ls)) {
				g_free(ls->data);
				ls->data = NULL;
			}
			g_list_free(info->process_stat_list);
			info->process_stat_list = NULL;
		}
		if (info->iodelay_map) {
			g_hash_table_destroy(info->iodelay_map);
			info->iodelay_map = NULL;
		}
		g_free(info);
	}
}

int start_updating_container_info()
{
	if (update_thread_id > 0) {
		CPDS_LOG_ERROR("Container info thread is already running");
		return -1;
	}

	if (pthread_rwlock_init(&rwlock, NULL) !=0 ) {
		CPDS_LOG_ERROR("Failed to init pthread rwlock");
		return -1;
	}

	if (start_bpf_stat_monitor() != 0) {
		CPDS_LOG_ERROR("Failed to start stat monitor");
		return -1;
	}

	cmap = g_hash_table_new_full(g_str_hash, g_str_equal, cmap_key_destroy, cmap_value_destroy);
	if (cmap == NULL) {
		CPDS_LOG_ERROR("Failed to create container info hash table");
		return -1;
	}

	int ret = pthread_create(&update_thread_id, NULL, (void *)update_thread, NULL);
	return ret;
}

int stop_updating_container_info()
{
	void *status;

	done = 1;

	if (update_thread_id > 0) {
		pthread_cancel(update_thread_id);
		pthread_join(update_thread_id, &status);
	}

	if (cmap != NULL) {
		g_hash_table_destroy(cmap);
		cmap = NULL;
	}

	destory_bpf_stat_monitor();

	pthread_rwlock_destroy(&rwlock);

	return 0;
}

GList *get_container_basic_info_list()
{
	GList *plist = NULL;
	GHashTableIter iter;
	gpointer key, value;

	if (pthread_rwlock_wrlock(&rwlock) != 0)
		goto out;

	if (cmap == NULL)
		goto out;

	g_hash_table_iter_init(&iter, cmap);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		container_info_t *cinfo = (container_info_t *)value;
		ctn_basic_metric *cbm = g_malloc0(sizeof(ctn_basic_metric));
		if (cbm == NULL) {
			CPDS_LOG_ERROR("Failed to allocate memory");
			goto out;
		}
		cbm->cid = g_strdup(cinfo->cid);
		cbm->pid = cinfo->pid;
		cbm->status = g_strdup(cinfo->status);
		cbm->exit_code = cinfo->exit_code;
		plist = g_list_append(plist, cbm);
	}

out:
	pthread_rwlock_unlock(&rwlock);
	return plist;
}

GList *get_container_perf_info_list()
{
	GList *plist = NULL;
	GHashTableIter iter;
	gpointer key, value;

	if (pthread_rwlock_wrlock(&rwlock) != 0)
		goto out;

	if (cmap == NULL)
		goto out;

	g_hash_table_iter_init(&iter, cmap);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		container_info_t *cinfo = (container_info_t *)value;
		perf_stat_t *ps = &cinfo->perf_stat;
		ctn_perf_metric *cpm = g_malloc0(sizeof(ctn_perf_metric));
		if (cpm == NULL) {
			CPDS_LOG_ERROR("Failed to allocate memory");
			goto out;
		}
		cpm->cid = g_strdup(cinfo->cid);
		cpm->total_create_process_fail_cnt = ps->total_create_process_fail_cnt;
		cpm->total_create_thread_fail_cnt = ps->total_create_thread_fail_cnt;
		cpm->total_mmap_count = ps->total_mmap_count;
		cpm->total_mmap_fail_count = ps->total_mmap_fail_count;
		cpm->total_mmap_size = ps->total_mmap_size;
		cpm->total_mmap_time_seconds = (double)ps->total_mmap_time_ns / 1000000000;
		plist = g_list_append(plist, cpm);
	}

out:
	pthread_rwlock_unlock(&rwlock);
	return plist;
}

GList *get_container_resource_info_list()
{
	GList *plist = NULL;
	GHashTableIter iter;
	gpointer key, value;

	if (pthread_rwlock_wrlock(&rwlock) != 0)
		goto out;

	if (cmap == NULL)
		goto out;

	g_hash_table_iter_init(&iter, cmap);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		container_info_t *cinfo = (container_info_t *)value;
		ctn_resource_metric *crm = g_malloc0(sizeof(ctn_resource_metric));
		if (crm == NULL) {
			CPDS_LOG_ERROR("Failed to allocate memory");
			goto out;
		}
		crm->cid = g_strdup(cinfo->cid);
		crm->pid = cinfo->pid;
		crm->cpu_usage_seconds = (double)cinfo->cpu_usage_ns / 1000000000;
		crm->disk_usage_bytes = cinfo->disk_usage;
		crm->disk_iodelay = cinfo->disk_iodelay;
		crm->memory_total_bytes = cinfo->memory_stat.total;
		crm->memory_usage_bytes = cinfo->memory_stat.usage;
		crm->memory_swap_total_bytes = cinfo->memory_stat.swap_total;
		crm->memory_swap_usage_bytes = cinfo->memory_stat.swap_usage;
		crm->memory_cached_bytes = cinfo->memory_stat.cached;

		crm->ctn_net_dev_stat_list = NULL;
		for (GList *ls = g_list_first(cinfo->net_dev_stat_list); ls != NULL; ls = g_list_next(ls)) {
			net_dev_stat_t *nds = ls->data;
			ctn_net_dev_stat_metric *cndsm = g_malloc0(sizeof(ctn_net_dev_stat_metric));
			if (cndsm == NULL) {
				CPDS_LOG_ERROR("Failed to allocate memory");
				goto out;
			}
			cndsm->ifname = g_strdup(nds->ifname);
			cndsm->network_receive_bytes_total = nds->r_bytes;
			cndsm->network_receive_drop_total = nds->r_drop;
			cndsm->network_receive_errors_total = nds->r_errs;
			cndsm->network_receive_packets_total = nds->r_packets;
			cndsm->network_transmit_bytes_total = nds->t_bytes;
			cndsm->network_transmit_drop_total = nds->t_drop;
			cndsm->network_transmit_errors_total = nds->t_errs;
			cndsm->network_transmit_packets_total = nds->t_packets;
			crm->ctn_net_dev_stat_list = g_list_append(crm->ctn_net_dev_stat_list, cndsm);
		}

		plist = g_list_append(plist, crm);
	}

out:
	pthread_rwlock_unlock(&rwlock);
	return plist;
}

GList *get_container_process_info_list()
{
	GList *plist = NULL;
	GHashTableIter iter;
	gpointer key, value;

	if (pthread_rwlock_wrlock(&rwlock) != 0)
		goto out;

	if (cmap == NULL)
		goto out;

	g_hash_table_iter_init(&iter, cmap);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		container_info_t *cinfo = (container_info_t *)value;
		ctn_process_metric *cpm = g_malloc0(sizeof(ctn_process_metric));
		if (cpm == NULL) {
			CPDS_LOG_ERROR("Failed to allocate memory");
			goto out;
		}
		cpm->cid = g_strdup(cinfo->cid);

		cpm->ctn_sub_process_stat_list = NULL;
		for (GList *ls = g_list_first(cinfo->process_stat_list); ls != NULL; ls = g_list_next(ls)) {
			process_stat_t *ps = ls->data;
			ctn_sub_process_stat_metric *cspsm = g_malloc0(sizeof(ctn_sub_process_stat_metric));
			if (cspsm == NULL) {
				CPDS_LOG_ERROR("Failed to allocate memory");
				goto out;
			}
			cspsm->pid = ps->pid;
			cspsm->zombie_flag = ps->zombie_flag;
			cpm->ctn_sub_process_stat_list = g_list_append(cpm->ctn_sub_process_stat_list, cspsm);
		}

		plist = g_list_append(plist, cpm);
	}

out:
	pthread_rwlock_unlock(&rwlock);
	return plist;
}
