@echo off
setlocal enabledelayedexpansion

:: iconv.exe가 필요합니다 - GNU iconv 유틸리티 윈도우 버전
:: https://gnuwin32.sourceforge.net/packages/libiconv.htm 에서 다운로드 가능

:: 현재 폴더의 모든 .cpp 및 .h 파일 처리
for /r %%f in (*.cpp *.h) do (
    echo 처리 중: %%f
    iconv -f UTF-8 -t EUC-KR "%%f" > "%%f.tmp"
    move /y "%%f.tmp" "%%f"
)

echo 모든 파일 변환이 완료되었습니다.
pause