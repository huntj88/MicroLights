import glob
import re
import sys
import os

def update_test_file(file_path):
    """
    Scans a test file for test functions and updates/creates the main function
    to run all of them.
    """
    with open(file_path, 'r') as f:
        content = f.read()

    # Find all test functions
    # Matches: void test_Name(void)
    # We allow some flexibility in whitespace
    test_func_pattern = re.compile(r'^void\s+(test_\w+)\s*\(\s*void\s*\)', re.MULTILINE)
    found_tests = test_func_pattern.findall(content)
    
    if not found_tests:
        print(f"Warning: No tests found in {os.path.basename(file_path)}")
        return False

    # Sort tests for consistent output
    found_tests.sort()

    # Generate the new main function body
    new_main_lines = ["int main(void) {", "    UNITY_BEGIN();"]
    for test in found_tests:
        new_main_lines.append(f"    RUN_TEST({test});")
    new_main_lines.append("    return UNITY_END();")
    new_main_lines.append("}")
    new_main_block = "\n".join(new_main_lines)

    # Regex to find existing main function
    # Matches int main(void) { ... } including newlines
    # We use a non-greedy match for the body, but balancing braces is hard with regex.
    # However, assuming standard formatting where main is at the end or clearly defined.
    # A safer approach for C files is often to look for "int main(void)" and replace until the end of file 
    # if we assume main is last. Or we can just look for the start and try to match braces.
    
    # Given the project structure, main seems to be at the end.
    # Let's try to find "int main(void)" and replace the whole block.
    
    main_pattern = re.compile(r'int\s+main\s*\(\s*void\s*\)\s*\{', re.MULTILINE)
    match = main_pattern.search(content)
    
    new_content = ""
    
    if match:
        # Main exists. We need to replace it.
        # Since regex replacement of a block is tricky without a parser, 
        # and we want to be safe, let's assume main goes to the end of the file 
        # OR we can just replace the content from "int main(void) {" down to the last "}"
        
        start_idx = match.start()
        
        # Simple brace counting to find the end of main
        open_braces = 0
        end_idx = -1
        in_main = False
        
        for i in range(start_idx, len(content)):
            if content[i] == '{':
                open_braces += 1
                in_main = True
            elif content[i] == '}':
                open_braces -= 1
            
            if in_main and open_braces == 0:
                end_idx = i + 1
                break
        
        if end_idx != -1:
            # Replace the old main with the new main
            new_content = content[:start_idx] + new_main_block + content[end_idx:]
        else:
            print(f"Error: Could not parse main function in {file_path}")
            return False
    else:
        # Main does not exist, append it
        if not content.endswith('\n'):
            content += '\n'
        new_content = content + "\n" + new_main_block + "\n"

    # Only write if changed
    if new_content != content:
        print(f"Updating {os.path.basename(file_path)}...")
        with open(file_path, 'w') as f:
            f.write(new_content)
        return True
    else:
        return False

def check_test_inclusion(run_tests_path, test_files):
    """
    Checks if all test files are referenced in the run_tests.sh script.
    """
    if not os.path.exists(run_tests_path):
        print(f"Error: {run_tests_path} not found.")
        return False

    with open(run_tests_path, 'r') as f:
        run_tests_content = f.read()

    missing_files = []
    for test_file in test_files:
        filename = os.path.basename(test_file)
        if filename not in run_tests_content:
            missing_files.append(filename)

    if missing_files:
        print("\nError: The following test files are present in Tests/ but not included in run_tests.sh:")
        for f in missing_files:
            print(f"  - {f}")
        print("\nPlease add compilation and execution steps for these files in run_tests.sh.")
        return False
    
    return True

def main():
    project_root = os.getcwd()
    tests_dir = os.path.join(project_root, 'Tests')
    run_tests_path = os.path.join(project_root, 'run_tests.sh')
    
    # Find all test files
    test_files = glob.glob(os.path.join(tests_dir, 'test_*.c'))
    
    if not test_files:
        print("No test files found in Tests/ directory.")
        sys.exit(1)
        
    print(f"Scanning and updating {len(test_files)} test files...")
    
    updated_count = 0
    for f in test_files:
        if update_test_file(f):
            updated_count += 1
            
    if updated_count > 0:
        print(f"Updated {updated_count} files.")
    else:
        print("All files are up to date.")

    if not check_test_inclusion(run_tests_path, test_files):
        sys.exit(1)

if __name__ == "__main__":
    main()
