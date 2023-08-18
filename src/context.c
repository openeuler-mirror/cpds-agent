#include "context.h"

agent_context global_ctx = {
	.show_version = FALSE,
	.config_file = NULL,
	.log_cfg_file = NULL,
	.net_diagnostic_dest = NULL,
	.expose_port = 0
};

void free_global_context()
{
	agent_context *ctx = &global_ctx;
	if (ctx->config_file) {
		g_free(ctx->config_file);
		ctx->config_file = NULL;
	}
	if (ctx->log_cfg_file) {
		g_free(ctx->log_cfg_file);
		ctx->log_cfg_file = NULL;
	}
	if (ctx->net_diagnostic_dest) {
		g_free(ctx->net_diagnostic_dest);
		ctx->net_diagnostic_dest = NULL;
	}
}
