# 构建脚本（PowerShell 版）：将 C 源码编译为 WebAssembly + 胶水 JS（由 Emscripten 自动生成）。
# 需要预先安装并激活 Emscripten SDK：https://emscripten.org/docs/getting_started/downloads.html
#   git clone https://github.com/emscripten-core/emsdk.git
#   cd emsdk; .\emsdk install latest; .\emsdk activate latest; .\emsdk_env.ps1
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
New-Item -ItemType Directory -Force -Path dist | Out-Null

# 中文字体（用于渲染所有界面文字），默认使用系统自带的微软雅黑。
# 也可以通过环境变量 CHESS_FONT_PATH 指定其他支持中文的 .ttf/.ttc 字体。
$fontSrc = if ($env:CHESS_FONT_PATH) { $env:CHESS_FONT_PATH } else { "C:/Windows/Fonts/msyh.ttc" }

if (-not (Test-Path $fontSrc)) {
    Write-Error "找不到字体文件：$fontSrc`n请通过 CHESS_FONT_PATH 环境变量指定一个支持中文的 .ttf/.ttc 字体路径。"
}

emcc `
  src/main.c src/ui.c src/sound.c src/data.c src/chess_view.c `
  projects/chess-engine/src/chess.c projects/chess-engine/src/ai.c `
  -I src -I projects/chess-engine/src `
  -sUSE_SDL=2 -sUSE_SDL_TTF=2 `
  -sALLOW_MEMORY_GROWTH=1 `
  -sASSERTIONS=1 `
  --preload-file "$fontSrc@/fonts/msyh.ttc" `
  --shell-file shell.html `
  -O2 `
  -o dist/index.html

Write-Host ""
Write-Host "构建完成：dist/index.html"
Write-Host "本地预览：cd dist; python -m http.server 8000  然后打开 http://localhost:8000"
