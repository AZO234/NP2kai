@echo off

cd /d "%~dp0"

echo ============================================
echo   Neko Project II シームレスマウスドライバ
echo   アンインストーラ
echo ============================================
echo.

REM ---- OSバージョン確認（Windows 2000 = 5.0）----
ver | find " 5.00." >nul
if errorlevel 1 (
    echo.
    echo このOSは恐らくWindows 2000ではありません。
    echo アンインストールを中止します。
    echo.
    echo 終了するには何かキーを押して下さい。
    pause > NUL
    goto :eof
)

REM ---- ユーザー確認 ----
set /p CONFIRM=アンインストールを実行しますか？ (Y/N) :

if /I not "%CONFIRM%"=="Y" (
    echo.
    echo アンインストールをキャンセルしました。
    echo.
    echo 終了するには何かキーを押して下さい。
    pause > NUL
    goto :eof
)

echo.
echo アンインストールを実行しています...
echo.

REM フィルタを消す
cscript //nologo //E:VBScript "%~dp0rmmsz.dat" "HKLM\System\CurrentControlSet\Control\Class\{4D36E96F-E325-11CE-BFC1-08002BE10318}" "UpperFilters" "npmouse"
if errorlevel 1 goto :ERROR

REM テンポラリを借りる
copy /Y uninst.inx %windir%\Temp\npmouse_uninst.inf > NUL

rundll32 setupapi,InstallHinfSection DefaultUninstall 132 %windir%\Temp\npmouse_uninst.inf

REM テンポラリを消す
del /F /Q %windir%\Temp\npmouse_uninst.inf > NUL

echo.
echo 完了しました。
echo 再起動が必要です。
echo.
echo 終了するには何かキーを押して下さい。
pause > NUL
goto :eof

:ERROR
echo.
echo フィルタの登録解除に失敗しました。中止します。
echo.
echo 終了するには何かキーを押して下さい。
pause > NUL
