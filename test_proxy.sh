#!/bin/bash
# =================================================================
#   caching-proxy — Integration Test Suite
#   Edit the CONFIG block below to test any origin, port, or paths.
#   Run with:  ./test.sh
#   Skip build: BUILD=false ./test.sh
# =================================================================

# ── CONFIG ────────────────────────────────────────────────────────
PORT=3000
ORIGIN="http://dummyjson.com"
BINARY="./build/caching-proxy"
BUILD_DIR="build"
BUILD=${BUILD:-true}           # override: BUILD=false ./test.sh

# Each path below runs as a MISS → HIT pair automatically.
# Add or remove paths freely.
TEST_PATHS=(
    "/products"
    "/products/1"
    "/users"
    "/posts"
)

# Concurrency / coalescer stress test.
# Keep this path OUT of TEST_PATHS above (must be uncached when test runs).
COALESCE_PATH="/products/2"
COALESCE_CONCURRENCY=5
# ─────────────────────────────────────────────────────────────────

# ── COLORS ────────────────────────────────────────────────────────
RED='\033[0;31m'   GREEN='\033[0;32m'  YELLOW='\033[1;33m'
BLUE='\033[0;34m'  CYAN='\033[0;36m'  BOLD='\033[1m'
DIM='\033[2m'      NC='\033[0m'

# ── GLOBALS ───────────────────────────────────────────────────────
PASS_COUNT=0
FAIL_COUNT=0
PROXY_PID=""
LOG_FILE="proxy_test.log"

# ── FORMATTING ────────────────────────────────────────────────────
banner() {
    local L="━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo -e "\n${BOLD}${BLUE}${L}${NC}"
    echo -e "${BOLD}${BLUE}  $*${NC}"
    echo -e "${BOLD}${BLUE}${L}${NC}"
}

section() { echo -e "\n${YELLOW}  ┌─ $*${NC}"; }

pass_line() {
    # $1=id  $2=description  $3=detail
    PASS_COUNT=$((PASS_COUNT + 1))
    printf "  ${GREEN}✅ PASS${NC}  ${BOLD}[%-5s]${NC}  %-42s${DIM}%s${NC}\n" "$1" "$2" "$3"
}

fail_line() {
    FAIL_COUNT=$((FAIL_COUNT + 1))
    printf "  ${RED}❌ FAIL${NC}  ${BOLD}[%-5s]${NC}  %-42s${DIM}%s${NC}\n" "$1" "$2" "$3"
}

# ── CLEANUP ───────────────────────────────────────────────────────
cleanup() {
    if [[ -n "$PROXY_PID" ]]; then
        echo -e "\n${BLUE}  Stopping proxy (PID: ${PROXY_PID})...${NC}"
        kill "$PROXY_PID" 2>/dev/null || true
    fi
    rm -f "$LOG_FILE" /tmp/coalesce_*.txt /tmp/proxy_build.log
}
trap cleanup EXIT

# ── ASSERT: GET path, verify HTTP 200 + expected X-Cache value ────
check_xcache() {
    local id="$1" desc="$2" path="$3" expected="$4"
    local headers status xcache detail

    headers=$(curl -s -D - -o /dev/null --max-time 10 \
        "http://localhost:${PORT}${path}" 2>/dev/null)

    status=$(echo "$headers" | grep -E "^HTTP/"    | head -1 | tr -d '\r')
    xcache=$(echo "$headers" | grep -i "^X-Cache:" | head -1 | tr -d '\r')
    detail="${status}  |  ${xcache}"

    if echo "$status" | grep -q "200" && echo "$xcache" | grep -qi "$expected"; then
        pass_line "$id" "$desc" "$detail"
    else
        fail_line "$id" "$desc" "got: ${detail:-no response}  |  want X-Cache: ${expected}"
    fi
}

# =================================================================
# 1. BUILD
# =================================================================
banner "🔨  Build"

if [[ "$BUILD" == "true" ]]; then
    if cmake --build "$BUILD_DIR" > /tmp/proxy_build.log 2>&1; then
        echo -e "  ${GREEN}✅ Build successful${NC}"
    else
        echo -e "  ${RED}❌ Build failed — aborting${NC}\n"
        cat /tmp/proxy_build.log
        exit 1
    fi
else
    echo -e "  ${DIM}  Skipped  (BUILD=false)${NC}"
fi

# =================================================================
# 2. START PROXY
# =================================================================
banner "🚀  Proxy Startup"
echo -e "  ${DIM}port: ${PORT}   origin: ${ORIGIN}${NC}"

"$BINARY" --port "$PORT" --origin "$ORIGIN" > "$LOG_FILE" 2>&1 &
PROXY_PID=$!

# Poll until the TCP port is open, up to ~5 seconds
READY=false
for i in $(seq 1 25); do
    if ! kill -0 "$PROXY_PID" 2>/dev/null; then
        echo -e "\n  ${RED}❌ Proxy process died immediately. Log:${NC}"
        cat "$LOG_FILE"
        exit 1
    fi
    if bash -c "echo >/dev/tcp/localhost/${PORT}" 2>/dev/null; then
        READY=true
        break
    fi
    sleep 0.2
done

if $READY; then
    echo -e "  ${GREEN}✅ Listening on port ${PORT}  (PID: ${PROXY_PID})${NC}"
else
    echo -e "  ${RED}❌ Proxy did not bind within 5 s. Log:${NC}"
    cat "$LOG_FILE"
    exit 1
fi

# =================================================================
# 3. MISS → HIT TESTS FOR EACH PATH
# =================================================================
banner "🧪  Cache Tests"

GROUP=0
for path in "${TEST_PATHS[@]}"; do
    GROUP=$((GROUP + 1))
    section "Group ${GROUP}  —  ${path}"
    check_xcache "${GROUP}.1" "First request  → expect MISS" "$path" "MISS"
    check_xcache "${GROUP}.2" "Second request → expect HIT " "$path" "HIT"
done

# =================================================================
# 4. COALESCER / THUNDERING HERD TEST
# =================================================================
GROUP=$((GROUP + 1))
section "Group ${GROUP}  —  Coalescer  (${COALESCE_CONCURRENCY}× concurrent on ${COALESCE_PATH})"
echo -e "  ${DIM}    Firing ${COALESCE_CONCURRENCY} simultaneous requests...${NC}"

CURL_PIDS=()
for i in $(seq 1 "$COALESCE_CONCURRENCY"); do
    curl -s -D - -o /dev/null --connect-timeout 5 --max-time 15 \
        "http://localhost:${PORT}${COALESCE_PATH}" > "/tmp/coalesce_${i}.txt" 2>/dev/null &
    CURL_PIDS+=($!)
done
wait "${CURL_PIDS[@]}"  # block until all background curls finish

OK_COUNT=0; MISS_C=0; HIT_C=0
for i in $(seq 1 "$COALESCE_CONCURRENCY"); do
    f="/tmp/coalesce_${i}.txt"
    st=$(grep -E "^HTTP/"    "$f" 2>/dev/null | head -1 | tr -d '\r')
    xc=$(grep -i "^X-Cache:" "$f" 2>/dev/null | head -1 | tr -d '\r')
    echo "$st" | grep -q  "200"  && OK_COUNT=$((OK_COUNT + 1))
    echo "$xc" | grep -qi "MISS" && MISS_C=$((MISS_C + 1))
    echo "$xc" | grep -qi "HIT"  && HIT_C=$((HIT_C + 1))
done

detail="HTTP 200: ${OK_COUNT}/${COALESCE_CONCURRENCY}  |  MISS: ${MISS_C}  HIT: ${HIT_C}"

if [[ "$OK_COUNT" -eq "$COALESCE_CONCURRENCY" ]]; then
    pass_line "${GROUP}.1" "All ${COALESCE_CONCURRENCY} concurrent requests returned 200 " "$detail"
else
    fail_line "${GROUP}.1" "Some requests did not return 200      " "$detail"
fi

if grep -q "Coalesced" "$LOG_FILE" 2>/dev/null; then
    pass_line "${GROUP}.2" "Coalescer activated                   " "'Coalesced' entry found in proxy log"
else
    fail_line "${GROUP}.2" "No coalescing detected                " "'Coalesced' not found — requests may not have overlapped"
fi

# =================================================================
# 5. PROXY LOG  (color-coded)
# =================================================================
banner "📋  Proxy Log"

while IFS= read -r line; do
    if   echo "$line" | grep -q "Coalesced";              then echo -e "  ${CYAN}${line}${NC}"
    elif echo "$line" | grep -q "\[CACHE\] HIT";          then echo -e "  ${GREEN}${line}${NC}"
    elif echo "$line" | grep -q "\[CACHE\] MISS";         then echo -e "  ${YELLOW}${line}${NC}"
    elif echo "$line" | grep -qE "\[ERROR\]|\[FATAL\]|\[WARNING\]"; \
                                                          then echo -e "  ${RED}${line}${NC}"
    else                                                       echo -e "  ${DIM}${line}${NC}"
    fi
done < "$LOG_FILE"

# =================================================================
# 6. SUMMARY
# =================================================================
TOTAL=$((PASS_COUNT + FAIL_COUNT))
banner "📊  Summary"

printf "  %-14s %s\n"  "Total tests"  "${BOLD}${TOTAL}${NC}"
printf "  ${GREEN}%-14s %s${NC}\n"  "Passed"  "${BOLD}${PASS_COUNT}${NC}"
printf "  ${RED}%-14s %s${NC}\n"    "Failed"  "${BOLD}${FAIL_COUNT}${NC}"
echo ""

if [[ "$FAIL_COUNT" -eq 0 ]]; then
    echo -e "  ${GREEN}${BOLD}🎉  All ${TOTAL} tests passed!${NC}\n"
    exit 0
else
    echo -e "  ${RED}${BOLD}⚠️   ${FAIL_COUNT} / ${TOTAL} test(s) failed.${NC}\n"
    exit 1
fi