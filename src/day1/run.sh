#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "${BASH_SOURCE[0]%/*}" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build/day1"
SRC_DIR="${SCRIPT_DIR}"

usage() {
  cat <<'EOF'
用法: ./run.sh [value|rvo|fwd|rule5|all]
  value  编译运行 value_categories 示例
  rvo    编译运行 rvo_nrvo 示例
  fwd    编译运行 forwarding 示例（转发引用/引用折叠）
  rule5  编译运行 rule_of_five 示例（Rule of Five/Six）
  all    编译运行全部示例（默认）
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

run_fwd() {
  build "forwarding" "forwarding.cpp"
  echo "[RUN ] forwarding" && "${BUILD_DIR}/forwarding"
}

run_rule5() {
  build "rule_of_five" "rule_of_five.cpp"
  echo "[RUN ] rule_of_five" && "${BUILD_DIR}/rule_of_five"
}

choice=${1:-all}
case "${choice}" in
  value) run_value ;;
  rvo)   run_rvo ;;
  fwd)   run_fwd ;;
  rule5) run_rule5 ;;
  all)   run_value; echo; run_rvo; echo; run_fwd; echo; run_rule5 ;;
  -h|--help) usage ;;
  *) usage; exit 1 ;;
esac
