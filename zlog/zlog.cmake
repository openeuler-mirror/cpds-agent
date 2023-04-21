set(ZLOG_PROJ_DIR "${PROJECT_SOURCE_DIR}/zlog")
set(ZLOG_BUILD_DIR "${PROJECT_BINARY_DIR}/zlog_lib")
set(ZLOG_LIB "${ZLOG_BUILD_DIR}/libzlog.a")

add_custom_target(zlog_lib ALL
    DEPENDS ${ZLOG_LIB}
)

add_custom_command(OUTPUT ${ZLOG_LIB}
    COMMAND unzip -o "${ZLOG_PROJ_DIR}/zlog.zip" -d ${ZLOG_BUILD_DIR}
    COMMAND cd "${ZLOG_BUILD_DIR}/src" && make
    COMMAND cp -f "${ZLOG_BUILD_DIR}/src/zlog.h" ${ZLOG_PROJ_DIR}
    COMMAND cp -f "${ZLOG_BUILD_DIR}/src/libzlog.a" ${ZLOG_LIB}
    DEPENDS "${ZLOG_PROJ_DIR}/zlog.zip"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "building zlog..."
)