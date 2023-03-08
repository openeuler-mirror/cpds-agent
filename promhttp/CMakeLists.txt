project(libpromhttp VERSION ${Version} LANGUAGES C)

set(build_dir ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(public_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(private_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(prom_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../prom/include)
set(public_files ${public_dir}/promhttp.h)
set(private_files ${private_dir}/promhttp.c)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../prom/build)

add_library(promhttp STATIC)

target_include_directories(
    promhttp
    PUBLIC ${public_dir} /usr/include ${prom_include_dir}
    PRIVATE ${private_dir}
)

target_sources(
    promhttp
    PUBLIC ${public_files}
    PRIVATE ${private_files}
)

find_library(prom prom HINTS ${CMAKE_CURRENT_SOURCE_DIR}/../prom/build)
find_library(microhttpd microhttpd)

target_compile_options(promhttp PRIVATE "-Werror" "-Wuninitialized" "-Wall" "-Wno-unused-label" "-std=gnu11")
target_compile_options(promhttp PUBLIC "-Wuninitialized" "-Wall" "-Wno-unused-label" "-std=gnu11")

target_link_libraries(promhttp PUBLIC pthread prom microhttpd)
