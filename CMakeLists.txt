cmake_minimum_required(VERSION 3.14.5)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

execute_process(
    COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/VERSION
    OUTPUT_VARIABLE Version
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/VERSION
    COMMAND sed --regexp-extended "s/([0-9]+)\.([0-9])+\.([0-9]+)/\\1/g"
    OUTPUT_VARIABLE MajorVersion
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/VERSION
    COMMAND sed --regexp-extended "s/([0-9]+)\.([0-9])+\.([0-9]+)/\\2/"
    OUTPUT_VARIABLE MinorVersion
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/VERSION
    COMMAND sed --regexp-extended "s/([0-9]+)\.([0-9])+\.([0-9]+)/\\3/"
    OUTPUT_VARIABLE PatchVersion
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

project(cpds-agent VERSION ${Version})
message(STATUS "${PROJECT_NAME} version: ${Version}")

# 获取当前 git commit id
execute_process(COMMAND git rev-parse HEAD
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                TIMEOUT 1
                RESULT_VARIABLE result_var
                OUTPUT_VARIABLE commit_id
                ERROR_VARIABLE error_var
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE)
message(STATUS "commit_id: ${commit_id}")
add_definitions(-DGIT_COMMIT_ID="${commit_id}")

# glib
find_package(PkgConfig)
pkg_search_module(GLIB REQUIRED glib-2.0)
MESSAGE(STATUS "glib-2.0 dirs:" ${GLIB_INCLUDE_DIRS})
MESSAGE(STATUS "glib-2.0 lib:" ${GLIB_LIBRARIES})

# 目标文件
set(TARGET_BIN ${PROJECT_NAME})
add_executable(${TARGET_BIN})

# 子模块：prometheus client lib
add_subdirectory(prom)
add_subdirectory(promhttp)

# 子模块：zlog lib
set(ZLOG_PROJ_DIR "${PROJECT_SOURCE_DIR}/zlog")
set(ZLOG_BUILD_DIR "${PROJECT_BINARY_DIR}/zlog_lib")
set(ZLOG_LIB "${ZLOG_BUILD_DIR}/libzlog.a")
add_subdirectory(zlog)
add_dependencies(${TARGET_BIN} zlog_lib)

# 编译选项
target_compile_options(${TARGET_BIN} 
    PRIVATE
    -DCPDS_AGENT_VERSION="${Version}"
    -DGIT_COMMIT_ID="${commit_id}"
    "-Wall"
)

# 源文件
aux_source_directory("${PROJECT_SOURCE_DIR}/src" MAIN_SRCS)
aux_source_directory("${PROJECT_SOURCE_DIR}/src/metrics" MAIN_SRCS)
aux_source_directory("${PROJECT_SOURCE_DIR}/json" JSON_SRCS)
target_sources(${TARGET_BIN}
    PRIVATE
    ${MAIN_SRCS}
    ${JSON_SRCS}
) 

# 头文件目录
target_include_directories(${TARGET_BIN}
    PRIVATE 
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src/metrics"
    "${PROJECT_SOURCE_DIR}/prom/include"
    "${PROJECT_SOURCE_DIR}/promhttp/include"
    "${PROJECT_SOURCE_DIR}/json"
    "${ZLOG_PROJ_DIR}"
    "${GLIB_INCLUDE_DIRS}"
)

# 链接库
target_link_libraries(${TARGET_BIN}
    PRIVATE 
    ${GLIB_LIBRARIES}
    ${ZLOG_LIB}
    prom
    promhttp
    pthread
)

install(TARGETS ${TARGET_BIN} RUNTIME DESTINATION "/usr/bin")
install(DIRECTORY config/ DESTINATION "/etc/cpds/agent")
