#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "${BASH_SOURCE[0]%/*}" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build/day2"
SRC_DIR="${SCRIPT_DIR}"

usage() {
  cat <<'EOF'
用法: ./run.sh [res|all]
  res  编译运行 resource_strategies 示例（独占/共享/可拷可移资源）
  all  同上（默认）
EOF
}

build() {
  mkdir -p "${BUILD_DIR}"
  local target="$1" src="$2"
  echo "[BUILD] ${src} -> ${target}"
  g++ -std=c++17 -O0 -g -Wall -Wextra -pedantic \
      "${SRC_DIR}/${src}" -o "${BUILD_DIR}/${target}"
}

run_res() {
  build "resource_strategies" "resource_strategies.cpp"
  echo "[RUN ] resource_strategies" && "${BUILD_DIR}/resource_strategies"
}

choice=${1:-all}
case "${choice}" in
  res|all) run_res ;;
  -h|--help) usage ;;
  *) usage; exit 1 ;;
esac
