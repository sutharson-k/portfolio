# Sutharson · 个人作品集网站

一个完全用 **C 语言**编写、编译为 **WebAssembly** 运行的个人作品集网站。
所有界面文字均为中文。渲染基于 SDL2（Emscripten 内置移植），文字渲染基于
SDL2_ttf（使用系统自带中文字体，默认微软雅黑）。

## ⚠️ 重要提示：本项目尚未编译验证

当前开发环境中没有安装任何 C 编译器（无 gcc / clang / emcc），因此这些源码
**只经过人工审读，未经实际编译和运行验证**。请按下方步骤自行构建后，
把编译器报出的任何错误反馈回来，我会据此修正代码。

## 目录结构

```
src/                    应用外壳（导航、首页、项目、技能、联系）
  main.c                入口、窗口/渲染器初始化、主循环、页面路由
  ui.c / ui.h            SDL2 绘制辅助：文字、矩形、圆形、点击命中系统
  data.c / data.h        项目与技能数据（取材自 github.com/sutharson-k）
  chess_view.c / .h       国际象棋试玩页面（接入 chess-engine 引擎）
  sound.c / sound.h       音效（通过 Emscripten 桥接 Web Audio API）
projects/chess-engine/src/
  chess.c / chess.h       国际象棋规则引擎（移植自原 chess-engine 仓库的 chess.js）
  ai.c / ai.h             极小极大搜索 AI（移植自原仓库的 ai.js）
shell.html                Emscripten 页面外壳模板（仅加载态提示，无业务逻辑）
build.sh / build.ps1       构建脚本
```

## 构建步骤

1. 安装 Emscripten SDK（约 1GB，只需一次）：

   ```
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh      # Windows PowerShell 用 .\emsdk_env.ps1
   ```

2. 在本项目目录下执行构建脚本：

   - Git Bash / WSL / macOS / Linux：`./build.sh`
   - PowerShell：`./build.ps1`

   构建产物在 `dist/`（`index.html` + `index.js` + `index.wasm` +
   `index.data`，后者是打包进虚拟文件系统的中文字体）。

3. 本地预览（不能直接双击打开 `index.html`，需通过 HTTP 服务器）：

   ```
   cd dist
   python -m http.server 8000
   ```

   然后浏览器打开 `http://localhost:8000`。

## 已知的简化与限制

- 固定画布分辨率 1200×820，未做响应式适配（移动端体验会较差）。
- 棋子用字母徽标（K/Q/R/B/N/P + 黑白配色圆片）表示，没有使用国际象棋
  Unicode 符号——因为无法确认目标字体是否包含这些符号字形，用字母更稳妥。
- 棋盘未绘制行列坐标标签（a-h / 1-8）。
- 因为是 canvas 整体渲染，没有可选中文本、无障碍支持、也不利于 SEO。
- 国际象棋规则细节均照搬原 JS 版本的实现（包括其中两个非标准之处，为保持
  行为一致而未"修正"）：半步计数器从不重置（影响 50 步和棋判定的计时）、
  易位走法不附加"将军/将死"记谱后缀。
- 悔棋按钮最多回退到本局第一步。

## 唯一必须使用 JavaScript 的两处

WebAssembly 本身无法直接访问浏览器 API，这两处通过 Emscripten 的 `EM_ASM` /
`EM_JS` 桥接（桥接代码本身不含任何页面逻辑，只是浏览器 API 的直接转发）：

1. `platform_open_url`（`src/ui.c`）——点击项目卡片 / 联系方式时用
   `window.open()` 打开新标签页。
2. `sound.c` 中的音效——用 Web Audio API 合成走子/吃子/将军等音效。

## 项目数据来源

`src/data.c` 中的 15 个项目取自 `github.com/sutharson-k` 的公开仓库，
已剔除空仓库、fork 仓库，以及作者自己标注"未运行成功"的仓库。
