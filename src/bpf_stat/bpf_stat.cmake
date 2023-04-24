execute_process(
    COMMAND uname -m
    OUTPUT_VARIABLE ARCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(BPF_STAT_BUILD_DIR "${PROJECT_BINARY_DIR}/bpf_stat")
set(BPF_STAT_INCLUDE_DIR "${BPF_STAT_BUILD_DIR}/include")
set(BPF_STAT_SKEL_HEAD "${BPF_STAT_INCLUDE_DIR}/bpf_stat.skel.h")
set(BPF_STAT_SRC_DIR "${PROJECT_SOURCE_DIR}/src/bpf_stat")
set(BPF_STAT_BPF_C "${BPF_STAT_SRC_DIR}/bpf_stat.bpf.c")
set(BPF_STAT_BPF_O "${BPF_STAT_BUILD_DIR}/bpf_stat.bpf.o")
set(BPF_VMLINUX_H "${BPF_STAT_INCLUDE_DIR}/vmlinux.h")

add_custom_target(bpf_stat_skel ALL
    DEPENDS ${BPF_STAT_SKEL_HEAD}
)

# 生成skeleton头文件，用户空间程序利用这个头文件可以编写代码加载eBPF程序
add_custom_command(OUTPUT ${BPF_STAT_SKEL_HEAD}
    COMMAND bpftool gen skeleton ${BPF_STAT_BPF_O} > ${BPF_STAT_SKEL_HEAD}
    DEPENDS ${BPF_STAT_BPF_O}
    WORKING_DIRECTORY ${BPF_STAT_SRC_DIR}
    COMMENT "building bpf skeleton..."
)

# 编译eBPF内核程序
add_custom_command(OUTPUT ${BPF_STAT_BPF_O}
    COMMAND clang -g -O2 -target bpf -D__${ARCH}__ -I ${BPF_INCLUDE_DIR} -I ${BPF_STAT_INCLUDE_DIR} -o ${BPF_STAT_BPF_O} -c ${BPF_STAT_BPF_C}
    DEPENDS bpf_lib ${BPF_VMLINUX_H} ${BPF_STAT_BPF_C}
    WORKING_DIRECTORY ${BPF_STAT_SRC_DIR}
    COMMENT "building bpf program..."
)

# 生成vmlinux.h头文件（需要BTF支持）
add_custom_command(OUTPUT ${BPF_VMLINUX_H}
    COMMAND mkdir -p ${BPF_STAT_INCLUDE_DIR}
    COMMAND bpftool btf dump file /sys/kernel/btf/vmlinux format c > ${BPF_VMLINUX_H}
    DEPENDS /sys/kernel/btf/vmlinux
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "building vmlinux.h..."
)
