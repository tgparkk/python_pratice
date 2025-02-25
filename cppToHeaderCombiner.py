import os
import glob

def combine_cpp_to_header(directory_path='.'):
    """Combine cpp file contents to their header files"""
    success = True
    cpp_files = glob.glob(os.path.join(directory_path, '*.cpp'))
    
    for cpp_file in cpp_files:
        header_file = cpp_file[:-4] + '.h'
        
        if not os.path.exists(header_file):
            print(f"Warning: {header_file} not found.")
            success = False
            continue
            
        try:
            # Try to read cpp content with different encodings
            encodings = ['utf-8', 'cp949', 'euc-kr', 'latin-1', 'ascii']
            cpp_content = None
            
            # Read cpp file
            with open(cpp_file, 'rb') as f:
                data = f.read()
                
            # Try each encoding
            for enc in encodings:
                try:
                    cpp_content = data.decode(enc)
                    print(f"Read {cpp_file} with {enc} encoding")
                    break
                except UnicodeDecodeError:
                    continue
            
            if cpp_content is None:
                print(f"Failed to decode {cpp_file} with any encoding")
                cpp_content = data.decode('utf-8', errors='replace')
            
            # Read header file with same approach
            h_encoding = 'utf-8'  # Default
            with open(header_file, 'rb') as f:
                h_data = f.read()
                
            for enc in encodings:
                try:
                    h_data.decode(enc)
                    h_encoding = enc
                    print(f"Read {header_file} with {enc} encoding")
                    break
                except UnicodeDecodeError:
                    continue
            
            # Append to header file
            with open(header_file, 'ab') as f:
                separator = f'\n\n{"="*80}\n// {os.path.basename(cpp_file)} file content\n{"="*80}\n\n'
                f.write(separator.encode(h_encoding, errors='replace'))
                f.write(cpp_content.encode(h_encoding, errors='replace'))
                
            print(f"Success: Added {cpp_file} content to {header_file}")
            
        except Exception as e:
            print(f"Error processing {cpp_file}: {str(e)}")
            success = False
            
    return success

if __name__ == '__main__':
    dir_path = r'D:\GIT\python_pratice\ServerCoreLibrary'
    combine_cpp_to_header(dir_path)