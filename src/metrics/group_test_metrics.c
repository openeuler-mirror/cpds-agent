#include "metric_group_type.h"
#include "prom.h"

static void group_test_init();
static void group_test_destroy();
static void group_test_update();

metric_group group_test = {.name = "group_test",
                                   .update_period = 1,
                                   .init = group_test_init,
                                   .destroy = group_test_destroy,
                                   .update = group_test_update};

static prom_counter_t *test_counter;
static prom_gauge_t *test_gauge;

static void group_test_init()
{
	metric_group *grp = &group_test;
	
	test_counter = prom_counter_new("test_counter", "counter for test", 0, NULL);
	grp->metrics = g_list_append(grp->metrics, test_counter);
	
	static const char *label_keys[] = {"lable_1", "lable_2"};
	test_gauge = prom_gauge_new("test_gauge", "gauge for test", 2, label_keys);
	grp->metrics = g_list_append(grp->metrics, test_gauge);
}

static void group_test_destroy()
{
	if (group_test.metrics)
		g_list_free(group_test.metrics);
}

static void group_test_update()
{
	static int i = 0;
	
	if (i % 10 == 0)
		prom_counter_set(test_counter, 0, NULL);
	else
		prom_counter_inc(test_counter, NULL);
		
	if ((i / 10) % 2 == 0) {
		prom_gauge_add(test_gauge, 2, (const char *[]){"animal", "cat"});
		prom_gauge_add(test_gauge, 1, (const char *[]){"animal", "dog"});
	} else {
		prom_gauge_sub(test_gauge, 2, (const char *[]){"animal", "cat"});
		prom_gauge_sub(test_gauge, 1, (const char *[]){"animal", "dog"});
	}
	
	i++;
}
