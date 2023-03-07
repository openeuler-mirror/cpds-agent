#ifndef _COLLECTION_H_
#define _COLLECTION_H_

#include "metric_group_type.h"

metric_group_list *init_all_metrics();

void free_all_metrics();

int start_updating_metrics();

int stop_updating_metrics();

#endif
