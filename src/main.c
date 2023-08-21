#include "collection.h"
#include "commandline.h"
#include "configure.h"
#include "context.h"
#include "logger.h"
#include "registration.h"
#include "web_service.h"
#include "ping.h"

#include <signal.h>
#include <glib.h>

static int done = 0;

static void int_handler(int signal)
{
	CPDS_LOG_INFO_PRINT("shutting down...");
	done = 1;
}

static int do_cpds_agent_service(agent_context *ctx)
{
	int ret = -1;

	CPDS_LOG_INFO("cpds-agent service started. port=%d", ctx->expose_port);
	
	ret = init_ping_svc();
	if (ret != 0) {
		CPDS_LOG_ERROR("Failed to init ping svc");
		goto out;
	}

	if (init_default_prometheus_registry() != 0) {
		CPDS_LOG_ERROR_PRINT("init premetheus registry error");
		goto out;
	}

	metric_group_list *mgroups = init_all_metrics();
	if (register_metircs_to_default_registry(mgroups) != 0) {
		CPDS_LOG_ERROR_PRINT("register metrics error");
		goto out;
	}

	if (start_http_service(ctx->expose_port) != 0) {
		CPDS_LOG_ERROR_PRINT("start http service error");
		goto out;
	}

	if (start_updating_metrics() != 0) {
		CPDS_LOG_ERROR_PRINT("start updating metrics error");
		goto out;
	}

	signal(SIGINT, int_handler);

	while (done == 0) {
		g_usleep(100000);
	}

	ret = 0;

out:
	stop_updating_metrics();
	stop_http_service();
	destroy_default_prometheus_registry();
	free_all_metrics();
	destroy_ping_svc();

	CPDS_LOG_INFO("cpds-agent service stopped");
	
	return ret;
}

int main(int argc, char **argv)
{
	int ret = -1;
	agent_context *ctx = &global_ctx;

	if (parse_commandline(ctx, argc, argv) != 0) {
		CPDS_PRINT("parse command line error");
		goto out;
	}

	// 显示版本信息并退出
	if (ctx->show_version == TRUE) {
		CPDS_PRINT("version: %s", CPDS_AGENT_VERSION);
		CPDS_PRINT("commit id: %s", GIT_COMMIT_ID);
		ret = 0;
		goto out;
	}

	if (ctx->config_file == NULL) {
		ctx->config_file = g_strdup(DEFAULT_CFG_FILE);
	}
	if (load_config(ctx, ctx->config_file) != 0) {
		CPDS_PRINT("load config error. file: '%s'", ctx->config_file);
		goto out;
	}

	if (log_init(ctx->log_cfg_file) != 0) {
		CPDS_PRINT("log init error");
		goto out;
	}

	ret = do_cpds_agent_service(ctx);

out:
	free_global_context(ctx);
	log_fini();

	return ret;
}
