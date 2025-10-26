@echo off
echo Compiling Bloom shaders...

REM fxc.exeのパスを探す（環境に応じて調整してください）
set FXC="C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\fxc.exe"

REM fxc.exeが見つからない場合は、別のパスを試す
if not exist %FXC% (
    set FXC="C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe"
)

if not exist %FXC% (
    echo Error: fxc.exe not found!
    echo Please update the FXC path in this batch file.
    pause
    exit /b 1
)

echo Using FXC: %FXC%
echo.

REM shadersディレクトリに移動
cd shaders

REM Bloom.hlsl から複数のエントリーポイントをコンパイル
echo Compiling Bloom_VSMain.cso...
%FXC% /T vs_5_0 /E VSMain /Fo Bloom_VSMain.cso Bloom.hlsl
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile Bloom_VSMain
    cd ..
    pause
    exit /b 1
)

echo Compiling Bloom_PSBrightPass.cso...
%FXC% /T ps_5_0 /E PSBrightPass /Fo Bloom_PSBrightPass.cso Bloom.hlsl
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile Bloom_PSBrightPass
    cd ..
    pause
    exit /b 1
)

echo Compiling Bloom_PSComposite.cso...
%FXC% /T ps_5_0 /E PSComposite /Fo Bloom_PSComposite.cso Bloom.hlsl
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile Bloom_PSComposite
    cd ..
    pause
    exit /b 1
)

REM GaussianBlur.hlsl
echo Compiling GaussianBlur_PSMain.cso...
%FXC% /T ps_5_0 /E PSMain /Fo GaussianBlur_PSMain.cso GaussianBlur.hlsl
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile GaussianBlur_PSMain
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo All shaders compiled successfully!
echo ========================================
echo.
echo Generated files in shaders/:
dir /b shaders\*.cso

pause