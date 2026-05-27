#!/usr/bin/env python3
import json
import os
import shutil
import signal
import socket
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PORT = 5432
HOST = "127.0.0.1"

def run(cmd, **kwargs):
    return subprocess.run(cmd, shell=True, text=True, capture_output=True, cwd=ROOT, **kwargs)

def request(sql):
    payload = sql.encode("utf-8")
    with socket.create_connection((HOST, PORT), timeout=5) as s:
        s.sendall(struct.pack(">I", len(payload)) + payload)
        _status = s.recv(4)
        size_raw = s.recv(4)
        if len(size_raw) != 4:
            raise RuntimeError("bad response length")
        size = struct.unpack(">I", size_raw)[0]
        data = b""
        while len(data) < size:
            chunk = s.recv(size - len(data))
            if not chunk:
                break
            data += chunk
        return json.loads(data.decode("utf-8"))

def port_ready(timeout=10):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection((HOST, PORT), timeout=0.5):
                return True
        except OSError:
            time.sleep(0.2)
    return False

def stop_port_process():
    if sys.platform.startswith("linux") or sys.platform == "darwin":
        subprocess.run(f"fuser -k {PORT}/tcp", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def main():
    shutil.rmtree(ROOT / "data", ignore_errors=True)
    stop_port_process()

    print("[1/4] build")
    script = ROOT / "scripts" / ("build_full.bat" if os.name == "nt" else "build_full.sh")
    result = run(str(script))
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr)
        return result.returncode

    server = ROOT / "build" / ("Release/customdb_server.exe" if os.name == "nt" else "customdb_server")
    print("[2/4] server")
    proc = subprocess.Popen([str(server)], cwd=server.parent, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        if not port_ready():
            print("server did not start")
            return 1

        print("[3/4] sql protocol")
        assert request("CREATE DATABASE testdb;")["type"] == "ddl"
        assert request("USE testdb;")["type"] == "ddl"
        assert request("CREATE TABLE users (id INT AUTO_INCREMENT, name TEXT UNIQUE, age INT);")["type"] == "ddl"
        assert request("INSERT INTO users (name, age) VALUES ('Alice', 25);")["type"] == "dml"
        assert request("INSERT INTO users (name, age) VALUES ('Bob', 30);")["affected_rows"] == 1
        selected = request("SELECT name, age FROM users WHERE age >= 30;")
        assert selected["type"] == "select" and selected["rows"] == [["Bob", "30"]]
        assert request("UPDATE users SET age = 31 WHERE name = 'Bob';")["affected_rows"] == 1
        assert request("DELETE FROM users WHERE name = 'Alice';")["affected_rows"] == 1
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
        stop_port_process()

    print("[4/4] persistence")
    proc = subprocess.Popen([str(server)], cwd=server.parent, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        if not port_ready():
            return 1
        persisted = request("USE testdb; SELECT * FROM users;")
        assert persisted["type"] == "select" and persisted["rows"][0][1] == "Bob"
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
        stop_port_process()

    print("functional tests passed")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
