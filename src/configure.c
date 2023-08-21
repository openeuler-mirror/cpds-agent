#include "context.h"
#include "json.h"
#include "logger.h"

#include <glib.h>

int load_config(agent_context *ctx, const char *cfg_file)
{
	if (ctx == NULL || cfg_file == NULL) {
		CPDS_PRINT("load_config para error");
		return -1;
	}

	int ret = -1;
	GError *error = NULL;
	gchar *file_content = NULL;
	cJSON *cfg_json = NULL;

	if (g_file_get_contents(cfg_file, &file_content, NULL, &error) == FALSE) {
		CPDS_PRINT("read config file fail. - %s", error->message);
		goto out;
	}

	cfg_json = cJSON_Parse(file_content);
	if (cfg_json == NULL) {
		CPDS_PRINT("parse config content fail.");
		goto out;
	}

	// 如果参数没指定，从配置加载日志配置文件路径
	if (ctx->log_cfg_file == NULL) {
		char *temp_str = cJSON_GetStringValue(cJSON_GetObjectItem(cfg_json, "log_cfg_file"));
		if (temp_str != NULL)
			ctx->log_cfg_file = g_strdup(temp_str);
		else
			ctx->log_cfg_file = g_strdup(DEFAULT_LOG_CFG_FILE);
	}

	// 如果参数没指定，从配置加载metrics暴露端口
	if (ctx->expose_port <= 0) {
		cJSON *temp = cJSON_GetObjectItem(cfg_json, "expose_port");
		if (temp && cJSON_IsNumber(temp))
			ctx->expose_port = temp->valueint;
		else
			ctx->expose_port = DEFAULT_EXPOSE_PORT;
	}

	char *temp_str = cJSON_GetStringValue(cJSON_GetObjectItem(cfg_json, "net_diagnostic_dest"));
	if (temp_str != NULL) {
		ctx->net_diagnostic_dest = g_strdup(temp_str);
	} else {
		ctx->net_diagnostic_dest = g_strdup(DEFAULT_NET_DIAGNOSTIC_DEST);
		CPDS_LOG_INFO("Use DEFAULT_NET_DIAGNOSTIC_DEST %s", ctx->net_diagnostic_dest);
	}

	ret = 0;

out:
	if (error)
		g_error_free(error);
	if (file_content)
		g_free(file_content);
	if (cfg_json)
		cJSON_Delete(cfg_json);

	return ret;
}