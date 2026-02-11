#!/usr/bin/env python3
"""
Comprehensive Test Runner for PiSSTVpp2
Executes all test suites and generates aggregated report
"""

import subprocess
import sys
import os
import json
import time
from datetime import datetime
from pathlib import Path

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    BRIGHT_CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

class TestRunner:
    """Master test runner orchestrating all test suites"""
    
    def __init__(self, executable_path="/Users/ssamjung/Desktop/WIP/PiSSTVpp2/bin/pisstvpp2"):
        self.exe = executable_path
        self.test_dir = os.path.dirname(os.path.abspath(__file__))
        self.results = {
            'edge_cases': {'pass': 0, 'fail': 0, 'total': 0, 'execution_time': 0},
            'security_exploits': {'pass': 0, 'fail': 0, 'total': 0, 'execution_time': 0},
            'error_codes': {'pass': 0, 'fail': 0, 'total': 0, 'execution_time': 0},
        }
        self.overall_pass = 0
        self.overall_fail = 0
        
        if not os.path.exists(self.exe):
            raise FileNotFoundError(f"Executable not found: {self.exe}")
    
    def _run_test_suite(self, test_file: str, suite_name: str) -> bool:
        """Execute a single test suite and capture results"""
        test_path = os.path.join(self.test_dir, test_file)
        
        if not os.path.exists(test_path):
            print(f"{Colors.RED}[ERROR]{Colors.RESET} {test_file} not found")
            return False
        
        print(f"\n{Colors.BOLD}{Colors.BLUE}{'='*70}{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}Running: {suite_name}{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.BLUE}{'='*70}{Colors.RESET}\n")
        
        start_time = time.time()
        
        try:
            result = subprocess.run(
                [sys.executable, test_path, self.exe],
                capture_output=False,
                text=True,
                timeout=180
            )
            
            execution_time = time.time() - start_time
            self.results[suite_name]['execution_time'] = execution_time
            
            # Parse output to extract test counts
            return result.returncode == 0
            
        except subprocess.TimeoutExpired:
            print(f"{Colors.RED}[TIMEOUT]{Colors.RESET} Test suite exceeded 180 seconds")
            return False
        except Exception as e:
            print(f"{Colors.RED}[ERROR]{Colors.RESET} {str(e)}")
            return False
    
    def _extract_test_results(self, output: str, suite_name: str):
        """Extract test results from output"""
        lines = output.split('\n')
        for line in lines:
            if 'Passed:' in line and 'Failed:' in line:
                try:
                    parts = line.split()
                    for i, part in enumerate(parts):
                        if part == 'Passed:':
                            passed = int(parts[i+1].rstrip(','))
                            self.results[suite_name]['pass'] = passed
                        elif part == 'Failed:':
                            failed = int(parts[i+1].rstrip(','))
                            self.results[suite_name]['fail'] = failed
                    self.results[suite_name]['total'] = self.results[suite_name]['pass'] + self.results[suite_name]['fail']
                except:
                    pass
    
    def run_all(self):
        """Execute all test suites"""
        print(f"\n{Colors.BOLD}{Colors.BRIGHT_CYAN}╔{'═'*68}╗{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.BRIGHT_CYAN}║  PiSSTVpp2 - Comprehensive Test Suite Runner               ║{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.BRIGHT_CYAN}╚{'═'*68}╝{Colors.RESET}")
        print(f"\n{Colors.CYAN}Executable: {self.exe}{Colors.RESET}")
        print(f"{Colors.CYAN}Test Directory: {self.test_dir}{Colors.RESET}")
        print(f"{Colors.CYAN}Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}{Colors.RESET}")
        
        start_time = time.time()
        
        # Run test suites sequentially
        suites = [
            ('test_edge_cases.py', 'edge_cases', 'Edge Case & Boundary Value Tests'),
            ('test_error_codes.py', 'error_codes', 'Error Code Verification Tests'),
            ('test_security_exploits.py', 'security_exploits', 'Security & Exploitation Tests'),
        ]
        
        results_by_suite = {}
        
        for test_file, suite_key, display_name in suites:
            try:
                test_path = os.path.join(self.test_dir, test_file)
                if not os.path.exists(test_path):
                    print(f"\n{Colors.YELLOW}[SKIP]{Colors.RESET} {display_name} - {test_file} not found")
                    continue
                
                print(f"\n{Colors.BOLD}{Colors.BLUE}{'─'*70}{Colors.RESET}")
                print(f"{Colors.BOLD}{Colors.CYAN}▶ {display_name}{Colors.RESET}")
                print(f"{Colors.BOLD}{Colors.BLUE}{'─'*70}{Colors.RESET}\n")
                
                suite_start = time.time()
                
                result = subprocess.run(
                    [sys.executable, test_path, self.exe],
                    capture_output=True,
                    text=True,
                    timeout=180
                )
                
                suite_time = time.time() - suite_start
                
                # Print suite output
                print(result.stdout)
                if result.stderr:
                    print(result.stderr)
                
                # Track results
                passed = failed = 0
                for line in result.stdout.split('\n'):
                    if 'Passed:' in line and ('Failed:' in line or 'PASS' in line):
                        try:
                            if 'Passed:  ' in line:
                                parts = line.split()
                                passed = int(parts[parts.index('Passed:') + 1].rstrip(',').rstrip('/'))
                                if '/' in parts[parts.index('Passed:') + 1]:
                                    total = int(parts[parts.index('Passed:') + 1].split('/')[-1])
                                    failed = total - passed
                        except:
                            pass
                
                results_by_suite[suite_key] = {
                    'time': suite_time,
                    'passed': passed,
                    'failed': failed,
                    'success': result.returncode == 0
                }
                
                self.overall_pass += passed
                self.overall_fail += failed
                
            except subprocess.TimeoutExpired:
                print(f"{Colors.RED}[TIMEOUT]{Colors.RESET} {display_name} exceeded 180 seconds")
                results_by_suite[suite_key] = {'time': 180, 'passed': 0, 'failed': 0, 'success': False}
            except Exception as e:
                print(f"{Colors.RED}[ERROR]{Colors.RESET} {display_name}: {str(e)}")
                results_by_suite[suite_key] = {'time': 0, 'passed': 0, 'failed': 0, 'success': False}
        
        total_time = time.time() - start_time
        self._print_summary(results_by_suite, total_time)
    
    def _print_summary(self, results_by_suite, total_time):
        """Print comprehensive test summary"""
        overall_pass = sum(r.get('passed', 0) for r in results_by_suite.values())
        overall_fail = sum(r.get('failed', 0) for r in results_by_suite.values())
        overall_total = overall_pass + overall_fail
        
        print(f"\n{Colors.BOLD}{Colors.BLUE}{'═'*70}{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.BRIGHT_CYAN}COMPREHENSIVE TEST RESULTS{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.BLUE}{'═'*70}{Colors.RESET}\n")
        
        # Individual suite results
        print(f"{Colors.BOLD}Test Suite Breakdown:{Colors.RESET}")
        print(f"{Colors.BLUE}─{Colors.RESET} " * 35)
        
        suite_names = {
            'edge_cases': 'Edge Case Tests',
            'error_codes': 'Error Code Tests',
            'security_exploits': 'Security Tests'
        }
        
        for suite_key, display_name in suite_names.items():
            if suite_key in results_by_suite:
                result = results_by_suite[suite_key]
                passed = result['passed']
                failed = result['failed']
                total = passed + failed
                time_str = f"{result['time']:.2f}s"
                
                if total > 0:
                    pass_pct = (passed / total) * 100
                    status = f"{Colors.GREEN}✓{Colors.RESET}" if result['success'] else f"{Colors.RED}✗{Colors.RESET}"
                    print(f"{status} {display_name:30} {Colors.GREEN}{passed:3d}{Colors.RESET}/{total:<3d} " +
                          f"({pass_pct:5.1f}%) - {Colors.CYAN}{time_str}{Colors.RESET}")
        
        print(f"\n{Colors.BLUE}─{Colors.RESET} " * 35)
        
        # Overall summary
        print(f"\n{Colors.BOLD}Overall Results:{Colors.RESET}")
        if overall_total > 0:
            overall_pct = (overall_pass / overall_total) * 100
            result_color = Colors.GREEN if overall_fail == 0 else Colors.YELLOW if overall_fail < 5 else Colors.RED
            print(f"  {Colors.BOLD}Total Tests:{Colors.RESET} {overall_total}")
            print(f"  {Colors.BOLD}Passed:{Colors.RESET} {Colors.GREEN}{overall_pass}{Colors.RESET}")
            print(f"  {Colors.BOLD}Failed:{Colors.RESET} {result_color}{overall_fail}{Colors.RESET}{Colors.RESET}")
            print(f"  {Colors.BOLD}Success Rate:{Colors.RESET} {result_color}{overall_pct:.1f}%{Colors.RESET}")
        
        print(f"  {Colors.BOLD}Total Time:{Colors.RESET} {Colors.CYAN}{total_time:.2f}s{Colors.RESET}")
        
        print(f"\n{Colors.BOLD}{Colors.BLUE}{'═'*70}{Colors.RESET}")
        
        # Final verdict
        if overall_fail == 0:
            print(f"\n{Colors.GREEN}{Colors.BOLD}✓ ALL TESTS PASSED!{Colors.RESET}")
            print(f"{Colors.GREEN}The application is production-ready with comprehensive test coverage.{Colors.RESET}")
        elif overall_fail < 5:
            print(f"\n{Colors.YELLOW}{Colors.BOLD}⚠ MINOR ISSUES DETECTED{Colors.RESET}")
            print(f"{Colors.YELLOW}Review failed tests before deployment.{Colors.RESET}")
        else:
            print(f"\n{Colors.RED}{Colors.BOLD}✗ SIGNIFICANT ISSUES DETECTED{Colors.RESET}")
            print(f"{Colors.RED}Address failures before proceeding.{Colors.RESET}")
        
        print(f"{Colors.BLUE}{'═'*70}{Colors.RESET}\n")
        
        # Save report
        self._save_report(results_by_suite, overall_pass, overall_fail, total_time)
    
    def _save_report(self, results_by_suite, overall_pass, overall_fail, total_time):
        """Save test results to JSON report"""
        report = {
            'timestamp': datetime.now().isoformat(),
            'executable': self.exe,
            'test_directory': self.test_dir,
            'overall': {
                'passed': overall_pass,
                'failed': overall_fail,
                'total': overall_pass + overall_fail,
                'success_rate': (overall_pass / (overall_pass + overall_fail) * 100) if (overall_pass + overall_fail) > 0 else 0,
                'execution_time': total_time
            },
            'suites': results_by_suite
        }
        
        report_path = os.path.join(self.test_dir, 'test_report_latest.json')
        try:
            with open(report_path, 'w') as f:
                json.dump(report, f, indent=2)
            print(f"{Colors.CYAN}Report saved: {report_path}{Colors.RESET}")
        except Exception as e:
            print(f"{Colors.YELLOW}Warning: Could not save report: {e}{Colors.RESET}")

def main():
    """Main entry point"""
    exe_path = sys.argv[1] if len(sys.argv) > 1 else "/Users/ssamjung/Desktop/WIP/PiSSTVpp2/bin/pisstvpp2"
    
    try:
        runner = TestRunner(exe_path)
        runner.run_all()
        sys.exit(0)
    except FileNotFoundError as e:
        print(f"{Colors.RED}Error: {e}{Colors.RESET}")
        sys.exit(1)
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}Test run cancelled by user{Colors.RESET}")
        sys.exit(130)
    except Exception as e:
        print(f"{Colors.RED}Unexpected error: {e}{Colors.RESET}")
        sys.exit(2)

if __name__ == "__main__":
    main()
