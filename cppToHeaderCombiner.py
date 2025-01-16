import os
import glob
import tempfile
import shutil

def combine_cpp_to_header(directory_path='.'):
    """
    주어진 디렉토리에서 모든 .cpp 파일의 내용을 찾아서
    같은 이름의 .h 파일 끝에 추가합니다.
    
    Args:
        directory_path (str): 처리할 디렉토리 경로 (기본값: 현재 디렉토리)
    
    Returns:
        bool: 모든 파일이 성공적으로 처리되었는지 여부
    """
    success = True
    # .cpp 파일 목록 가져오기
    cpp_files = glob.glob(os.path.join(directory_path, '*.cpp'))
    
    for cpp_file in cpp_files:
        # .h 파일 경로 생성 (.cpp를 .h로 변경)
        header_file = cpp_file[:-4] + '.h'
        
        # .h 파일이 존재하는지 확인
        if not os.path.exists(header_file):
            print(f"경고: {header_file} 파일을 찾을 수 없습니다.")
            success = False
            continue
            
        try:
            # .cpp 파일 내용 읽기
            with open(cpp_file, 'r', encoding='utf-8') as f:
                cpp_content = f.read()
            
            # .h 파일에 구분선과 함께 .cpp 내용 추가
            with open(header_file, 'a', encoding='utf-8') as f:
                f.write('\n\n' + '='*80 + '\n')
                f.write(f'// {os.path.basename(cpp_file)} 파일 내용\n')
                f.write('='*80 + '\n\n')
                f.write(cpp_content)
                
            print(f"성공: {cpp_file} 내용을 {header_file}에 추가했습니다.")
            
        except Exception as e:
            print(f"오류 발생: {cpp_file} 처리 중 - {str(e)}")
            success = False
            
    return success

def run_tests():
    """테스트를 실행하고 결과를 출력합니다."""
    
    # 임시 디렉토리 생성
    with tempfile.TemporaryDirectory() as temp_dir:
        print("\n=== 테스트 시작 ===")
        
        # 테스트 1: 정상적인 경우
        print("\n테스트 1: 정상적인 경우")
        # 테스트 파일 생성
        cpp_content = "void test() { cout << \"Hello\"; }"
        h_content = "void test();"
        
        with open(os.path.join(temp_dir, "test.cpp"), "w") as f:
            f.write(cpp_content)
        with open(os.path.join(temp_dir, "test.h"), "w") as f:
            f.write(h_content)
            
        result = combine_cpp_to_header(temp_dir)
        
        # 결과 확인
        with open(os.path.join(temp_dir, "test.h"), "r") as f:
            final_content = f.read()
        print(f"테스트 1 결과: {'성공' if cpp_content in final_content else '실패'}")
        
        # 테스트 2: .h 파일이 없는 경우
        print("\n테스트 2: .h 파일이 없는 경우")
        # 디렉토리 초기화
        shutil.rmtree(temp_dir)
        os.makedirs(temp_dir)
        
        with open(os.path.join(temp_dir, "test2.cpp"), "w") as f:
            f.write(cpp_content)
            
        result = combine_cpp_to_header(temp_dir)
        print(f"테스트 2 결과: {'성공' if not result else '실패'}")
        
        # 테스트 3: 여러 파일 처리
        print("\n테스트 3: 여러 파일 처리")
        # 디렉토리 초기화
        shutil.rmtree(temp_dir)
        os.makedirs(temp_dir)
        
        # 여러 파일 생성
        for i in range(3):
            with open(os.path.join(temp_dir, f"test{i}.cpp"), "w") as f:
                f.write(f"void test{i}() {{}}")
            with open(os.path.join(temp_dir, f"test{i}.h"), "w") as f:
                f.write(f"void test{i}();")
                
        result = combine_cpp_to_header(temp_dir)
        print(f"테스트 3 결과: {'성공' if result else '실패'}")
        
        print("\n=== 테스트 완료 ===")

if __name__ == '__main__':
    # 테스트 실행
    #run_tests()
    combine_cpp_to_header('D:\GIT\python_pratice\ServerCoreLibrary')