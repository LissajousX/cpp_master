# cpp_master

练习 C++11/14/17 基础与示例。包含值类别、RVO/NRVO 示例与学习笔记。

## 目录
- `plan.md`：两周复习计划。
- `day1.md`：Day1 知识要点与练习。
- `docs/`：学习笔记（如 `docs/day1_learn.md`）。
- `src/day1/`：示例代码与运行脚本。
  - `value_categories.cpp`：值类别、完美转发、重载命中示例。
  - `rvo_nrvo.cpp`：RVO/NRVO 对比示例。
  - `run.sh`：编译运行脚本（`./run.sh [value|rvo|all]`）。

## 快速开始
```bash
cd src/day1
chmod +x run.sh
./run.sh          # 编译运行全部示例
./run.sh value    # 仅值类别示例
./run.sh rvo      # 仅 RVO/NRVO 示例
```

## 环境
- C++17 编译器（脚本使用 `g++ -std=c++17`）。
- Linux/macOS 下直接运行脚本，Windows 可用 WSL 或手动编译对应源文件。
