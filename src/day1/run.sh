#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "${BASH_SOURCE[0]%/*}" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build/day1"
SRC_DIR="${SCRIPT_DIR}"

usage() {
  cat <<'EOF'
用法: ./run.sh [value|rvo|all]
  value  编译运行 value_categories 示例
  rvo    编译运行 rvo_nrvo 示例
  all    编译运行两个示例（默认）
EOF
}

build() {
  mkdir -p "${BUILD_DIR}"
  local target="$1" src="$2"
  echo "[BUILD] ${src} -> ${target}"
  g++ -std=c++17 -O0 -g -Wall -Wextra -pedantic \
      "${SRC_DIR}/${src}" -o "${BUILD_DIR}/${target}"
}

run_value() {
  build "value_categories" "value_categories.cpp"
  echo "[RUN ] value_categories" && "${BUILD_DIR}/value_categories"
}

run_rvo() {
  build "rvo_nrvo" "rvo_nrvo.cpp"
  echo "[RUN ] rvo_nrvo" && "${BUILD_DIR}/rvo_nrvo"
}

choice=${1:-all}
case "${choice}" in
  value) run_value ;;
  rvo)   run_rvo ;;
  all)   run_value; echo; run_rvo ;;
  -h|--help) usage ;;
  *) usage; exit 1 ;;
esac
