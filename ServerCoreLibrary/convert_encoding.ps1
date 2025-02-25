# UTF-8에서 EUC-KR로 파일 인코딩 변환 스크립트
# 실행 경로의 모든 .cpp와 .h 파일을 변환합니다

# 변환할 파일 목록 가져오기
$files = Get-ChildItem -Path "." -Include "*.cpp", "*.h" -Recurse

foreach ($file in $files) {
    # 파일 내용을 UTF-8로 읽기
    $content = Get-Content -Path $file.FullName -Encoding UTF8 -Raw
    
    # 내용을 EUC-KR로 다시 쓰기
    [System.IO.File]::WriteAllText($file.FullName, $content, [System.Text.Encoding]::GetEncoding(51949))
    
    Write-Host "변환 완료: $($file.FullName)"
}

Write-Host "모든 파일 변환이 완료되었습니다."