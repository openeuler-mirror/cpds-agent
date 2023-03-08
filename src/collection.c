#include "collection.h"
#include "logger.h"
#include "metric_groups.h"
#include "prom.h"

static metric_group_list *mgroups = NULL;
static pthread_t update_thread_id = -1;
static int done = 0;

metric_group_list *init_all_metrics()
{
	if (mgroups != NULL) {
		CPDS_LOG_WARN("metric groups already init");
		return mgroups;
	}

	mgroups = init_metric_groups(mgroups);

	GList *giter = NULL;
	for (giter = g_list_first(mgroups); giter != NULL; giter = g_list_next(giter)) {
		metric_group *group = (metric_group *)giter->data;
		if (group->init) {
			(*group->init)();
		}
	}

	return mgroups;
}

void free_all_metrics()
{
	GList *giter = NULL;
	for (giter = g_list_first(mgroups); giter != NULL; giter = g_list_next(giter)) {
		metric_group *group = (metric_group *)giter->data;
		if (group->destroy) {
			(*group->destroy)();
		}
	}
	g_list_free(mgroups);
}

static void do_update_metrcis(void *arg)
{
	pthread_testcancel();

	while (done == 0) {
		g_usleep(1000000);

		static int i = 0;
		GList *giter = NULL;
		for (giter = g_list_first(mgroups); giter != NULL; giter = g_list_next(giter)) {
			metric_group *group = (metric_group *)giter->data;
			if (group->update && (group->update_period > 0) && (i % group->update_period == 0)) {
				(*group->update)();
			}
		}
		i++;
	};
}

int start_updating_metrics()
{
	int ret = pthread_create(&update_thread_id, NULL, (void *)do_update_metrcis, NULL);
	return ret;
}

int stop_updating_metrics()
{
	void *status;
	done = 1;
	if (stop_updating_metrics > 0) {
		pthread_cancel(update_thread_id);
		pthread_join(update_thread_id, &status);
	}
	return 0;
}
