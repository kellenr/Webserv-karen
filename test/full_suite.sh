#!/bin/bash
# Comprehensive test runner for the Webserv project
# This script builds the server, runs valgrind, verifies each configuration
# file, and performs a series of HTTP requests to ensure all basic methods
# and CGI functionality work as expected.

set -euo pipefail

# ensure we run from repo root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR/.."

# cleanup handler to stop server if the script exits unexpectedly
cleanup() {
    if [ -n "${SERVER_PID:-}" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill -INT "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT
PORT=8081
SERVER=./webserv
CONF_DIR=conf
TEST_FILE=test/test.txt
COOKIE_FILE=www/cgi-bin/cookie.py

GREEN="\033[1;32m"
RED="\033[1;31m"
CYAN="\033[1;36m"
RESET="\033[0m"

function print_ok() {
    echo -e "${GREEN}✔ $1${RESET}"
}

function print_fail() {
    echo -e "${RED}✖ $1${RESET}"
}

function build_server() {
    echo -e "${CYAN}Building project...${RESET}"
    if make -j4 > /tmp/webserv_build.log 2>&1; then
        print_ok "Build succeeded"
    else
        print_fail "Build failed"
        cat /tmp/webserv_build.log
        exit 1
    fi
}

function run_valgrind() {
    echo -e "${CYAN}Running valgrind...${RESET}"
    if ! command -v valgrind > /dev/null; then
        echo "Valgrind not installed - skipping" > /tmp/valgrind.log
        print_fail "Valgrind unavailable"
        return
    fi
    set +e
    valgrind --leak-check=full --error-exitcode=1 \
        $SERVER $CONF_DIR/default.conf > /tmp/valgrind.log 2>&1 &
    local pid=$!
    sleep 2
    kill -INT $pid 2>/dev/null || true
    wait $pid
    local status=$?
    set -e
    if [ $status -eq 0 ]; then
        print_ok "Valgrind clean"
    else
        print_fail "Valgrind reported issues"
        tail -n 20 /tmp/valgrind.log
    fi
}

function check_configs() {
    echo -e "${CYAN}Checking configuration files...${RESET}"
    for cfg in $CONF_DIR/*.conf; do
        set +e
        $SERVER "$cfg" > /tmp/server.log 2>&1 &
        local pid=$!
        sleep 1
        if kill -0 $pid 2>/dev/null; then
            kill -INT $pid 2>/dev/null || true
        fi
        wait $pid
        local status=$?
        set -e
        if [ $status -eq 0 ]; then
            print_ok "$cfg"
        else
            print_fail "$cfg"
            tail -n 2 /tmp/server.log
        fi
    done
}

function start_server() {
    set +e
    $SERVER $CONF_DIR/default.conf > /tmp/server.log 2>&1 &
    SERVER_PID=$!
    set -e
    sleep 1
}

function stop_server() {
    if [ -n "${SERVER_PID:-}" ] && kill -0 $SERVER_PID 2>/dev/null; then
        kill -INT $SERVER_PID 2>/dev/null || true
    fi
    wait $SERVER_PID 2>/dev/null || true
}

function http_test() {
    local desc=$1
    local expected=$2
    shift 2
    local code
    code=$(curl -s -o /tmp/out -w "%{http_code}" "$@")
    if [ "$code" = "$expected" ]; then
        print_ok "$desc"
    else
        print_fail "$desc (got $code)"
        head -n 5 /tmp/out
    fi
}

build_server
run_valgrind
check_configs

echo -e "${CYAN}Starting server for HTTP tests...${RESET}"
start_server

# Basic HTTP method tests
http_test "GET /" 200 http://127.0.0.1:$PORT/
http_test "HEAD /" 200 -I http://127.0.0.1:$PORT/
http_test "POST upload" 201 -F "file=@$TEST_FILE" http://127.0.0.1:$PORT/upload/
http_test "PUT upload" 201 -T $TEST_FILE http://127.0.0.1:$PORT/upload/put_test.txt
http_test "DELETE file" 204 -X DELETE http://127.0.0.1:$PORT/test_delete.txt

# CGI and cookie tests
http_test "CGI hello" 200 http://127.0.0.1:$PORT/cgi-bin/hello.py
curl -i -s http://127.0.0.1:$PORT/cgi-bin/cookie.py > /tmp/cookie.out
if grep -q "Set-Cookie" /tmp/cookie.out; then
    print_ok "CGI cookie"
else
    print_fail "CGI cookie"
fi

stop_server

echo -e "${CYAN}Test suite complete.${RESET}"