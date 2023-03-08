#include "context.h"
#include "logger.h"

#include <glib.h>

int parse_commandline(agent_context *ctx, int argc, char **argv)
{
	GError *error = NULL;
	
	GOptionEntry entries[] = {
	    {"version", 'v', 0, G_OPTION_ARG_NONE, &ctx->show_version, "show version", NULL},
	    {"config-file", 'c', 0, G_OPTION_ARG_STRING, &ctx->config_file, "specify the config file name (default: /etc/cpds/agent/config.json)", "PATH"},
	    {"log-config-file", 'l', 0, G_OPTION_ARG_STRING, &ctx->log_cfg_file, "specify the log config file name (default: /etc/cpds/agent/log.conf)", "PATH"},
	    {"expose-port", 'p', 0, G_OPTION_ARG_INT, &ctx->expose_port, "specify the port that expose metrics to (default: 20001)", "PORT"},
	    {NULL}
	};

	GOptionContext *context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, entries, NULL);
	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE) {
		CPDS_PRINT("option parsing failed: %s", error->message);
		g_error_free(error);
		return -1;
	}

	return 0;
}
