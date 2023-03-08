#include "logger.h"

#include <glib.h>

int log_init(const char *cfg_file)
{
	if (cfg_file == NULL) {
		CPDS_PRINT("log_init para error!!");
		return -1;
	}
	// 指定配置文件路径及类型名 初始化zlog
	int rc = dzlog_init(cfg_file, "agent");
	if (rc != 0) {
		// 配置文件加载失败（可能因人为修改导致），则加载预置默认配置，保证log可以工作。
		// 绝大部分情况不应该走这个分支
		char *predef_content = g_strdup(
		    "[formats]"
		    "\nfmt = \"%d.%us [%V][%p][%f][%U][%L] %m%n\""
		    "\n[rules]"
		    "\nagent.* \"/var/log/cpds/agent/cpds-agent.log\",20M*10~\"/var/log/cpds/agent/cpds-agent.log.#r\"; fmt");
		const char *tmp_cfg_file = "/run/predef_cpds_agent_log.conf";
		g_file_set_contents(tmp_cfg_file, predef_content, -1, NULL);
		g_free(predef_content);
		rc = dzlog_init(tmp_cfg_file, "agent");
		if (rc != 0) {
			// 正常不会到这里，到这里说明 predef_content 预配置加载都有问题
			CPDS_PRINT("?????? init log failed!! load predefined log config error!!");
			return -1;
		}
		// 输出下警告
		CPDS_LOG_WARN("Loading '%s' failed! Use predefined log config instead.", cfg_file);
	}
	return 0;
}

void log_fini()
{
	zlog_fini();
}