import subprocess
import re
import os
import sys
from collections import defaultdict

def generate_report():
    print("Running lint_files.sh...")
    try:
        # Run the lint script and capture output
        result = subprocess.run(['./lint_files.sh'], capture_output=True, text=True)
        output = result.stdout + result.stderr
    except Exception as e:
        print(f"Failed to run lint script: {e}")
        return

    print("Parsing output...")
    
    # Categories
    clang_tidy_checks = defaultdict(list)
    cppcheck_checks = defaultdict(list)
    other_errors = []

    # Regex for clang-tidy: file:line:col: severity: message [check-name]
    # Example: /path/file.c:148:36: warning: parameter name 'id' is too short... [readability-identifier-length]
    clang_regex = re.compile(r'^(.+):(\d+):\d+: (\w+): (.+) \[(.+)\]$')

    # Regex for cppcheck: [file:line]: (severity) message [id]
    # Example: [Core/Src/main.c:100]: (warning) Found something [someId]
    cppcheck_regex = re.compile(r'^\[(.+):(\d+)\]: \((.+)\) (.+) \[(.+)\]$')

    lines = output.splitlines()
    for line in lines:
        line = line.strip()
        if not line:
            continue

        # Try clang-tidy
        match = clang_regex.match(line)
        if match:
            file_path, line_num, severity, message, check_name = match.groups()
            # Make path relative to Cwd if possible
            if file_path.startswith(os.getcwd()):
                file_path = os.path.relpath(file_path, os.getcwd())
            
            clang_tidy_checks[check_name].append({
                'file': file_path,
                'line': line_num,
                'severity': severity,
                'message': message
            })
            continue

        # Try cppcheck
        match = cppcheck_regex.match(line)
        if match:
            file_path, line_num, severity, message, check_id = match.groups()
            cppcheck_checks[check_id].append({
                'file': file_path,
                'line': line_num,
                'severity': severity,
                'message': message
            })
            continue
            
        # Capture other potential errors (compiler errors etc)
        # We want to avoid capturing the "Processing file..." lines or summary lines
        if "warning:" in line or "error:" in line:
             # Filter out the summary lines like "1 warning generated." or "Suppressed..."
             if not line.endswith("generated.") and not line.startswith("Suppressed"):
                 # Also avoid the "note:" lines which often follow warnings but don't have the [check-name] format handled above
                 # Actually, notes are useful context, but might clutter "Uncategorized". 
                 # Let's keep them if they look like file:line: note:
                 other_errors.append(line)

    # Generate Markdown
    with open('lint_report.md', 'w') as f:
        f.write('# Lint Report\n\n')
        
        f.write(f'Total Clang-Tidy Categories: {len(clang_tidy_checks)}\n')
        f.write(f'Total Cppcheck Categories: {len(cppcheck_checks)}\n\n')

        if clang_tidy_checks:
            f.write('## Clang-Tidy Issues\n\n')
            for check, items in sorted(clang_tidy_checks.items(), key=lambda x: len(x[1]), reverse=True):
                f.write(f'### {check} ({len(items)})\n')
                for item in items:
                    f.write(f"- **[{item['file']}]({item['file']}#L{item['line']})**: {item['message']}\n")
                f.write('\n')

        if cppcheck_checks:
            f.write('## Cppcheck Issues\n\n')
            for check, items in sorted(cppcheck_checks.items(), key=lambda x: len(x[1]), reverse=True):
                f.write(f'### {check} ({len(items)})\n')
                for item in items:
                    f.write(f"- **[{item['file']}]({item['file']}#L{item['line']})**: {item['message']}\n")
                f.write('\n')
                
        if other_errors:
            f.write('## Uncategorized/Other Errors\n\n')
            f.write('*(Includes notes and unparsed warnings)*\n\n')
            for err in other_errors:
                f.write(f"- `{err}`\n")

    print("Report generated: lint_report.md")

    # Count total issues
    count_clang = sum(len(v) for v in clang_tidy_checks.values())
    count_cpp = sum(len(v) for v in cppcheck_checks.values())
    total_issues = count_clang + count_cpp + len(other_errors)

    if total_issues > 0:
        print(f"FAILURE: Found {total_issues} lint issues.")
        sys.exit(1)
    else:
        print("SUCCESS: No lint issues found.")
        sys.exit(0)

if __name__ == '__main__':
    generate_report()
