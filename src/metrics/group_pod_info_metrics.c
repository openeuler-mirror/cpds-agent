#include "json.h"
#include "logger.h"
#include "metric_group_type.h"
#include "prom.h"

#include <curl/curl.h>

#define MAX_PODS_BUFF_SIZE 10 * 1024 * 1024
#define HTTP_PODS_URL "http://localhost:10255/pods"

typedef struct _pod_info {
	char *name;
	char *phase;
	char *reason;
	int value; // 1: running; 0: other state
} pod_info_t;

static GList *pod_info_list = NULL;

typedef struct _pods_buff {
	int size;
	char buff[MAX_PODS_BUFF_SIZE];
} pods_buff_t;

static CURL *curl = NULL;
static pthread_rwlock_t rwlock;
static pthread_t thread_id = 0;
static int done = 0;

static void collect_pod_info_thead(void *arg);

static void group_pod_info_init();
static void group_pod_info_destroy();
static void group_pod_info_update();

metric_group group_pod_info = {.name = "pod_info_group",
                               .update_period = 3,
                               .init = group_pod_info_init,
                               .destroy = group_pod_info_destroy,
                               .update = group_pod_info_update};

static prom_gauge_t *cpds_pod_state;

static void group_pod_info_init()
{
	metric_group *grp = &group_pod_info;
	const char *labels[] = {"name", "phase", "reason"};
	size_t label_count = sizeof(labels) / sizeof(labels[0]);
	cpds_pod_state = prom_gauge_new("cpds_pod_state", "pod status", label_count, labels);
	grp->metrics = g_list_append(grp->metrics, cpds_pod_state);

	// 初始化 curl
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl == NULL) {
		CPDS_LOG_ERROR("Init curl fail");
		return;
	}

	if (pthread_rwlock_init(&rwlock, NULL) != 0) {
		CPDS_LOG_ERROR("Failed to init pthread rwlock");
		return;
	}

	// 启动信息采集线程
	int ret = pthread_create(&thread_id, NULL, (void *)collect_pod_info_thead, NULL);
	if (ret != 0) {
		CPDS_LOG_ERROR("Failed to create updating pod info thread");
	}
}

static void group_pod_info_destroy()
{
	void *status;
	done = 1;
	if (thread_id > 0) {
		pthread_cancel(thread_id);
		pthread_join(thread_id, &status);
	}

	if (group_pod_info.metrics)
		g_list_free(group_pod_info.metrics);

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	pthread_rwlock_destroy(&rwlock);
}

static void group_pod_info_update()
{
	if (pthread_rwlock_wrlock(&rwlock) != 0)
		return;

	prom_gauge_clear(cpds_pod_state);

	GList *iter = pod_info_list;
	while (iter != NULL) {
		pod_info_t *pod_info = iter->data;
		prom_gauge_set(cpds_pod_state, pod_info->value,
		               (const char *[]){pod_info->name, pod_info->phase, pod_info->reason});
		iter = iter->next;
	}

	pthread_rwlock_unlock(&rwlock);
}

// Callback function to handle the server response
static size_t http_res_handler(void *contents, size_t size, size_t count, void *para)
{
	int total_size = (int)(size * count);

	pods_buff_t *pods_buff = para;
	if (!pods_buff)
		return total_size;

	if (total_size + pods_buff->size > MAX_PODS_BUFF_SIZE) {
		CPDS_LOG_ERROR("http response size too long");
		return total_size;
	}

	memcpy(&pods_buff->buff[pods_buff->size], contents, total_size);
	pods_buff->size += total_size;

	return total_size;
}

static void clear_pod_info_list()
{
	GList *iter = pod_info_list;
	while (iter != NULL) {
		pod_info_t *pinfo = iter->data;
		if (pinfo) {
			if (pinfo->name)
				g_free(pinfo->name);
			if (pinfo->phase)
				g_free(pinfo->phase);
			if (pinfo->reason)
				g_free(pinfo->reason);
			g_free(pinfo);
		}
		pod_info_list = g_list_delete_link(pod_info_list, iter);
		iter = pod_info_list;
	}
}

static void parse_pod_info(char *buff)
{
	if (buff == NULL)
		return;

	cJSON *j_root = NULL;
	j_root = cJSON_Parse(buff);
	if (j_root == NULL) {
		CPDS_LOG_ERROR("Failed to parse pods buff");
		goto out;
	}

	cJSON *j_item_arr = cJSON_GetObjectItem(j_root, "items");
	if (j_item_arr == NULL || !cJSON_IsArray(j_item_arr)) {
		CPDS_LOG_ERROR("get pod list items fail");
		goto out;
	}

	GList *tmp_pod_info_list = NULL;

	cJSON *j_item = NULL;
	cJSON_ArrayForEach(j_item, j_item_arr)
	{
		char *name = NULL;
		char *phase = NULL;
		char *reason = NULL;
		int value = 1;

		cJSON *j_meta = cJSON_GetObjectItem(j_item, "metadata");
		name = cJSON_GetStringValue(cJSON_GetObjectItem(j_meta, "name"));
		// CPDS_LOG_DEBUG("  name: %s", name);
		cJSON *j_status = cJSON_GetObjectItem(j_item, "status");
		phase = cJSON_GetStringValue(cJSON_GetObjectItem(j_status, "phase"));
		// CPDS_LOG_DEBUG("  phase: %s", phase);
		if (g_strcmp0(phase, "Running") != 0)
			value = 0;
		cJSON *j_ctn_status_arr = cJSON_GetObjectItem(j_status, "containerStatuses");
		cJSON *j_main_ctn_status = (j_ctn_status_arr != NULL) ? j_ctn_status_arr->child : NULL;
		cJSON *j_main_ctn_state_obj = cJSON_GetObjectItem(j_main_ctn_status, "state");
		cJSON *j_main_ctn_state = (j_main_ctn_state_obj != NULL) ? j_main_ctn_state_obj->child : NULL;
		if (j_main_ctn_state != NULL) {
			// CPDS_LOG_DEBUG("    container state: %s", j_main_ctn_state->string);
			if (g_strcmp0(j_main_ctn_state->string, "running") != 0) {
				reason = cJSON_GetStringValue(cJSON_GetObjectItem(j_main_ctn_state, "reason"));
				// CPDS_LOG_DEBUG("    reason: %s", reason);
				value = 0;
			}
		}

		pod_info_t *pod_info = g_malloc0(sizeof(pod_info_t));
		pod_info->name = (name != NULL) ? g_strdup(name) : NULL;
		pod_info->phase = (phase != NULL) ? g_strdup(phase) : NULL;
		pod_info->reason = (reason != NULL) ? g_strdup(reason) : NULL;
		pod_info->value = value;

		tmp_pod_info_list = g_list_append(tmp_pod_info_list, pod_info);
	}

	if (pthread_rwlock_wrlock(&rwlock) != 0)
		goto out;

	clear_pod_info_list();
	pod_info_list = tmp_pod_info_list;

	pthread_rwlock_unlock(&rwlock);

out:
	if (j_root)
		cJSON_Delete(j_root);
}

static void collect_pod_info_thead(void *arg)
{
	if (curl == NULL)
		return;

	pods_buff_t *pods_buff = g_malloc0(sizeof(pods_buff_t));
	if (!pods_buff) {
		CPDS_LOG_ERROR("Failed to allocate memory");
		return;
	}

	curl_easy_setopt(curl, CURLOPT_URL, HTTP_PODS_URL);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_res_handler);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, pods_buff);

	while (done == 0) {
		g_usleep(3000000);

		memset(pods_buff, 0, sizeof(pods_buff_t));

		// Perform the HTTP GET request
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
		}

		parse_pod_info(pods_buff->buff);
	}

	if (pods_buff)
		g_free(pods_buff);
}
