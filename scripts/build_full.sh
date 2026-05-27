#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "[1/3] Configuring DB project"
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

echo "[2/3] Building C++ targets"
cmake --build build --config Release --parallel "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
mkdir -p publish/linux-x64
cp build/customdb_server publish/linux-x64/customdb_server
if [ -f build/libdb_native.so ]; then cp build/libdb_native.so publish/linux-x64/; fi

echo "[3/3] Docker image (optional)"
if command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1; then
  docker build --no-cache -t databaseoficial-customdb-server .
else
  echo "Docker is not available; skipping image build."
fi

echo "Build completed: build/customdb_server"
