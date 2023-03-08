project(libpromhttp VERSION ${Version} LANGUAGES C)

set(build_dir ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(public_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(private_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(
    public_files
    ${public_dir}/prom_alloc.h
    ${public_dir}/prom_collector.h
    ${public_dir}/prom_collector_registry.h
    ${public_dir}/prom_counter.h
    ${public_dir}/prom_gauge.h
    ${public_dir}/prom_histogram.h
    ${public_dir}/prom_histogram_buckets.h
    ${public_dir}/prom_linked_list.h
    ${public_dir}/prom_map.h
    ${public_dir}/prom_metric.h
    ${public_dir}/prom_metric_sample.h
    ${public_dir}/prom_metric_sample_histogram.h
    ${public_dir}/prom.h
)

set(
    private_files
    ${private_dir}/prom_assert.h
    ${private_dir}/prom_collector.c
    ${private_dir}/prom_collector_registry.c
    ${private_dir}/prom_collector_registry_i.h
    ${private_dir}/prom_collector_registry_t.h
    ${private_dir}/prom_collector_t.h
    ${private_dir}/prom_counter.c
    ${private_dir}/prom_gauge.c
    ${private_dir}/prom_histogram.c
    ${private_dir}/prom_histogram_buckets.c
    ${private_dir}/prom_linked_list.c
    ${private_dir}/prom_linked_list_i.h
    ${private_dir}/prom_linked_list_t.h
    ${private_dir}/prom_log.h
    ${private_dir}/prom_map.c
    ${private_dir}/prom_map_i.h
    ${private_dir}/prom_map_t.h
    ${private_dir}/prom_metric.c
    ${private_dir}/prom_metric_formatter.c
    ${private_dir}/prom_metric_formatter_i.h
    ${private_dir}/prom_metric_formatter_t.h
    ${private_dir}/prom_metric_i.h
    ${private_dir}/prom_metric_sample.c
    ${private_dir}/prom_metric_sample_histogram.c
    ${private_dir}/prom_metric_sample_histogram_i.h
    ${private_dir}/prom_metric_sample_histogram_t.h
    ${private_dir}/prom_metric_sample_i.h
    ${private_dir}/prom_metric_sample_t.h
    ${private_dir}/prom_metric_t.h
    ${private_dir}/prom_process_fds.c
    ${private_dir}/prom_process_fds_i.h
    ${private_dir}/prom_process_fds_t.h
    ${private_dir}/prom_process_limits.c
    ${private_dir}/prom_process_limits_i.h
    ${private_dir}/prom_process_limits_t.h
    ${private_dir}/prom_process_stat.c
    ${private_dir}/prom_process_stat_i.h
    ${private_dir}/prom_process_stat_t.h
    ${private_dir}/prom_procfs_i.h
    ${private_dir}/prom_procfs_t.h
    ${private_dir}/prom_procfs.c
    ${private_dir}/prom_string_builder.c
    ${private_dir}/prom_string_builder_i.h
    ${private_dir}/prom_string_builder_t.h
)

add_library(prom STATIC)

target_compile_options(prom PRIVATE "-Werror" "-Wuninitialized" "-Wall" "-Wno-unused-label" "-std=gnu11")
target_compile_options(prom PUBLIC "-Wuninitialized" "-Wall" "-Wno-unused-label" "-std=gnu11")

target_include_directories(
    prom
    PUBLIC ${public_dir}
    PRIVATE ${private_dir}
)

target_sources(
    prom
    PUBLIC ${public_files}
    PRIVATE ${private_files}
)

target_link_libraries(prom PUBLIC pthread)
