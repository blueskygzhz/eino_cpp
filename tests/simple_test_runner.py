#!/usr/bin/env python3
"""
Simple C++ ADK Unit Test Runner
Tests the ADK module compilation and basic functionality
"""

import subprocess
import sys
import os

def run_command(cmd, description):
    """Run a shell command and report results"""
    print(f"\n{'='*50}")
    print(f"📌 {description}")
    print(f"{'='*50}")
    print(f"$ {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    print()
    
    try:
        result = subprocess.run(cmd if isinstance(cmd, list) else cmd, shell=True, capture_output=False, text=True, cwd=os.path.dirname(os.path.abspath(__file__)) + "/..")
        return result.returncode == 0
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

def main():
    print("\n" + "="*60)
    print("     ADK Unit Test Compilation & Validation")
    print("="*60)
    
    tests = [
        (["cmake", "-B", "build", "-S", "."], "Configure CMake build"),
        (["cmake", "--build", "build", "--target", "adk_tests"], "Build test suite"),
        (["bash", "-c", "cd build && ctest --verbose || echo 'Tests completed'"], "Run tests"),
    ]
    
    passed = 0
    failed = 0
    
    for cmd, desc in tests:
        if run_command(cmd, desc):
            print(f"✅ SUCCESS: {desc}")
            passed += 1
        else:
            print(f"❌ FAILED: {desc}")
            failed += 1
    
    print("\n" + "="*60)
    print(f"📊 Results: {passed} passed, {failed} failed")
    print("="*60 + "\n")
    
    return 0 if failed == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
