#!/bin/bash
# Run tests and generate verification report workflow

set -e

echo "======================================================================="
echo "SlowFrame Test Verification Workflow"
echo "======================================================================="
echo ""

# 1. Run all tests with output preservation
echo "Step 1: Running all tests..."
../../venv/bin/python3 run_master_tests.py

# 2. Generate verification report
echo ""
echo "Step 2: Generating verification report..."
../../venv/bin/python3 generate_verification_report.py --output ../verification_reports/verification_$(date +%Y%m%d_%H%M%S).html

# 3. Open report in browser (macOS)
if [[ "$OSTYPE" == "darwin"* ]]; then
    latest_report=$(ls -t ../verification_reports/verification_*.html 2>/dev/null | head -n1)
    if [ -n "$latest_report" ]; then
        echo ""
        echo "Step 3: Opening verification report in browser..."
        open "$latest_report"
    fi
fi

echo ""
echo "======================================================================="
echo "Workflow complete!"
echo "Review the HTML report and document your findings."
echo "Use the 'Save Verification' button for each test."
echo "Export results when complete using the 'Export Verification Results' button."
echo "======================================================================="
