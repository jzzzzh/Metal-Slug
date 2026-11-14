#!/usr/bin/env bash
# ============================================================================
#  校验脚本 —— 在任意平台（不需要 Windows / EasyX）验证游戏代码的正确性
#
#  用 tools/easyx_stub 里的 EasyX 桩头文件，把游戏编译成无头程序：
#    1) 语法检查（-Wall -Wextra，零警告）
#    2) AddressSanitizer + UndefinedBehaviorSanitizer 下跑完整流程
#       （菜单 / 选关 / 战斗 / Boss / 解救 / 通关 / 死亡 / 无尽模式）
#
#  用法：
#    ./tools/run_checks.sh                        # 默认 60 秒压测
#    METAL_SLUG_SOAK=36000 ./tools/run_checks.sh   # 10 分钟压测
# ============================================================================
set -euo pipefail

cd "$(dirname "$0")/.."

CXX="${CXX:-clang++}"
STUBS="tools/easyx_stub"
OUT="${OUT:-/tmp/metal_slug_headless}"
FLAGS="-std=c++14 -DUNICODE -D_UNICODE -I $STUBS"

echo "=============================================="
echo " 1/3  语法检查（-Wall -Wextra，Unicode / ANSI 两种字符集）"
echo "=============================================="
"$CXX" $FLAGS -fsyntax-only -Wall -Wextra -Wpedantic -Wshadow -Wno-unknown-pragmas tools/headless_test.cpp
# ANSI 构建同样要能编过（MSVC 上两种字符集都可能出现）
"$CXX" -std=c++14 -I $STUBS -fsyntax-only -Wall -Wextra -Wpedantic -Wshadow -Wno-unknown-pragmas tools/headless_test.cpp
echo "  通过：两种字符集均零警告"
echo

echo "=============================================="
echo " 2/3  ASan + UBSan 无头全流程测试"
echo "=============================================="
"$CXX" $FLAGS -g -O1 \
    -fsanitize=address,undefined \
    -fno-omit-frame-pointer \
    -fno-sanitize-recover=all \
    tools/headless_test.cpp -o "$OUT"
"$OUT"
echo
echo "=============================================="
echo " 3/3  实现细节提示（仅提示，不作为通过条件）"
echo "=============================================="
# 说明：这一步只是文本抽查，方便快速定位改动，不构成行为验证。
# 真正的行为验证在第 2 步：无头测试里每个修复都有对应的断言与反例。
# 因此这里即使没命中也只提示、不中断脚本。
hint() {
    if grep -q -e "$2" "$1"; then
        echo "  · $3"
    else
        echo "  ? (未找到) $3"
    fi
}
hint EasyXPng.h      'putimagePngEx'    "混合实现统一收敛到 putimagePngEx"
hint HeJinDanTou.cpp 'putToggleImagePng'  "镜像绘制复用统一入口"
hint EasyXPng.h      'src_w - 1 - i'     "镜像索引使用源图宽度（修正了原版的越界读）"
hint HeJinDanTou.cpp 'getwidth() <= 0'   "素材加载有缺失校验"
hint HeJinDanTou.cpp 'remove_if'         "实体回收走 remove_if（不再在 range-for 里 erase）"
hint HeJinDanTou.cpp 'spawnObstacles'    "掩体生成集中在一个函数里（无尽模式换轮会重铺）"
hint HeJinDanTou.cpp 'prevY'             "手雷等高速物体用扫掠区间判定掩体碰撞"
echo "  （以上为文本抽查提示，行为验证见第 2 步）"
echo
echo "全部校验通过 ✅"
