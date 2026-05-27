#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
./scripts/build_full.sh
./build/tests/unit_tests
./build/tests/integration_tests
python3 functional_test.py
