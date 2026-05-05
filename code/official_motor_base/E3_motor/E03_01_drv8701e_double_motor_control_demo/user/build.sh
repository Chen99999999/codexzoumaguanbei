#!/bin/bash
# 脚本功能：清理 out 目录无关文件 -> cmake 编译 -> make 编译 -> 上传目标文件到远程设备
export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# ===================== 核心配置区 =====================
WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${WORK_DIR}/../out"
USER_DIR="${WORK_DIR}/../user"
REMOTE_IP="192.168.43.228"
REMOTE_USER="root"
REMOTE_PATH="/home/root/"
MAKE_JOBS=$(nproc)
RESERVE_FILE="本文件夹作用.txt"

# ===================== 通用函数 =====================
error_exit() {
    echo -e "\033[31m[ERROR] $1\033[0m"
    exit 1
}

info_echo() {
    echo -e "\033[32m[INFO] $1\033[0m"
}

# ===================== 主逻辑 =====================
info_echo "准备进入目录: ${OUT_DIR}"
cd "${OUT_DIR}" || error_exit "无法进入 ${OUT_DIR} 目录，请检查目录是否存在！"

info_echo "开始清理当前目录，仅保留 ${RESERVE_FILE}"
find . -mindepth 1 ! -name "${RESERVE_FILE}" -exec rm -rf {} + || error_exit "目录清理失败，请检查目录权限！"

info_echo "执行 cmake 编译: cmake ${USER_DIR}"
cmake "${USER_DIR}" || error_exit "cmake 编译失败，请检查 CMakeLists.txt 或编译依赖！"

info_echo "cmake 执行成功，开始执行 make -j${MAKE_JOBS} 编译项目..."
make -j"${MAKE_JOBS}" || error_exit "make 编译失败，编译日志如上！"

parent_dir_name=$(basename "$(dirname "$(pwd)")")
info_echo "待上传文件: ${parent_dir_name}"

info_echo "开始上传文件到 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
scp -O "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" || error_exit "文件上传失败，请检查网络、远程权限或文件是否存在！"

info_echo "所有操作执行完成：清理目录 -> cmake 编译 -> make 编译 -> 文件上传，均成功！"
exit 0
