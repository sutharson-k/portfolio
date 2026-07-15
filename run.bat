@echo off
rem 一键构建并本地运行作品集网站：加载 Emscripten 环境 -> 构建 -> 启动本地
rem 服务器 -> 打开浏览器。双击本文件，或在 cmd 中运行即可。
setlocal

rem === 如果你的 Emscripten SDK 装在别的位置，改这一行 ===
set "EMSDK_DIR=D:\emsdk_parent\emsdk"

cd /d "%~dp0"

if not exist "%EMSDK_DIR%\emsdk_env.bat" (
  echo [错误] 找不到 Emscripten SDK：%EMSDK_DIR%
  echo 请把本文件顶部的 EMSDK_DIR 改成你的 emsdk 安装路径。
  echo 还没安装的话，先参照 README.md 安装 Emscripten SDK。
  pause
  exit /b 1
)

echo [1/3] 加载 Emscripten 环境...
call "%EMSDK_DIR%\emsdk_env.bat" >nul

echo [2/3] 构建网站（首次构建较慢，之后会用缓存加速）...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1"
if errorlevel 1 (
  echo.
  echo 构建失败，请查看上方报错信息。
  pause
  exit /b 1
)

echo [3/3] 启动本地服务器并打开浏览器...
cd /d "%~dp0dist"
start "作品集本地服务器（关闭此窗口以停止）" cmd /k python -m http.server 8000
timeout /t 2 /nobreak >nul
start "" http://localhost:8000/index.html

echo.
echo 已在新窗口启动本地服务器：http://localhost:8000
echo 关闭那个窗口即可停止服务器。
pause
