@echo off
REM 位置纠偏系统构建脚本（Windows平台）

REM 设置构建目录
set BUILD_DIR=build

REM 创建构建目录（如果不存在）
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM 进入构建目录
cd %BUILD_DIR%

REM 运行CMake配置
cmake ..

REM 检查CMake是否成功
if %ERRORLEVEL% neq 0 (
    echo CMake配置失败，请检查错误信息。
    cd ..
    pause
    exit /b 1
)

REM 构建项目（使用Visual Studio）
msbuild location_correction.sln /p:Configuration=Release /m

REM 检查构建是否成功
if %ERRORLEVEL% neq 0 (
    echo 项目构建失败，请检查错误信息。
    cd ..
    pause
    exit /b 1
)

REM 返回上级目录
cd ..

echo 项目构建成功！可执行文件位于 %BUILD_DIR%\bin\Release 目录下。
pause