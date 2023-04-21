set(BPF_SRC_DIR "${PROJECT_SOURCE_DIR}/libbpf")
set(BPF_BUILD_DIR "${PROJECT_BINARY_DIR}/bpf_lib")
set(BPF_INCLUDE_DIR "${BPF_BUILD_DIR}/include")
set(BPF_LIB "${BPF_BUILD_DIR}/lib/libbpf.a")

add_custom_target(bpf_lib ALL
    DEPENDS ${BPF_LIB}
)

add_custom_command(OUTPUT ${BPF_LIB}
    COMMAND BUILD_STATIC_ONLY=y OBJDIR=${BPF_BUILD_DIR} LIBDIR=${BPF_BUILD_DIR}/lib INCLUDEDIR=${BPF_BUILD_DIR}/include make install
    WORKING_DIRECTORY "${BPF_SRC_DIR}/src"
    COMMENT "building bpf lib..."
)