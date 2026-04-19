@echo off

cd /d "%~dp0"

echo ============================================
echo   Neko Project II シームレスマウスドライバ
echo   インストーラ
echo ============================================
echo.

REM ---- OSバージョン確認（Windows 2000 = 5.0）----
ver | find " 5.00." >nul
if errorlevel 1 (
    echo.
    echo このOSは恐らくWindows 2000ではありません。
    echo インストールを中止します。
    echo.
    echo 終了するには何かキーを押して下さい。
    pause > NUL
    goto :eof
)

REM ---- ユーザー確認 ----
set /p CONFIRM=インストールを実行しますか？ (Y/N) :

if /I not "%CONFIRM%"=="Y" (
    echo.
    echo インストールをキャンセルしました。
    echo.
    echo 終了するには何かキーを押して下さい。
    pause > NUL
    goto :eof
)

echo.
echo インストールを実行しています...
echo.

REM テンポラリを借りる
copy /Y npmouse.sys %windir%\Temp\npmouse.sys > NUL
copy /Y npmouse.inx %windir%\Temp\npmouse.inf > NUL

rundll32 setupapi,InstallHinfSection DefaultInstall 132 %windir%\Temp\npmouse.inf

REM テンポラリを消す
del /F /Q %windir%\Temp\npmouse.sys > NUL
del /F /Q %windir%\Temp\npmouse.inf > NUL

echo.
echo 完了しました。
echo 再起動が必要です。
echo.
echo 終了するには何かキーを押して下さい。
pause > NUL