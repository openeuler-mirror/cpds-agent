#ifndef _REGISTRATION_H_
#define _REGISTRATION_H_

#include "collection.h"

int init_default_prometheus_registry();

int register_metircs_to_default_registry(metric_group_list *mgroups);

int destroy_default_prometheus_registry();

#endif