#!/bin/bash

# Configuration ports
BACKEND_PORT=9000
PROXY_PORT=8080

echo "Running local integration tests..."

# 1. Clean up any existing instances on our ports
kill -9 $(lsof -t -i:$BACKEND_PORT) 2>/dev/null
kill -9 $(lsof -t -i:$PROXY_PORT) 2>/dev/null

# 2. Start Node.js Mock Backend in the background
node server.js > /dev/null 2>&1 &
BACKEND_PID=$!
sleep 1 # Allow server to bind

# 3. Start Caching Proxy in the background
./build/caching-proxy --port $PROXY_PORT --origin http://localhost:$BACKEND_PORT > /dev/null 2>&1 &
PROXY_PID=$!
sleep 1 # Allow proxy to bind

# Verify both are running
if ! kill -0 $BACKEND_PID 2>/dev/null; then
    echo "[ERROR] Failed to start mock server"
    exit 1
fi
if ! kill -0 $PROXY_PID 2>/dev/null; then
    echo "[ERROR] Failed to start proxy server"
    kill $BACKEND_PID
    exit 1
fi

FAILED=0
PASSED_COUNT=0

# --- Test Case 1: Standard Cache Hit ---
RES1=$(curl -s http://localhost:$PROXY_PORT/data)
sleep 1
RES2=$(curl -s http://localhost:$PROXY_PORT/data)

if [ "$RES1" = "$RES2" ]; then
    echo "  [OK]  Cache hit served matching timestamps."
    ((PASSED_COUNT++))
else
    echo "  [FAIL] Cache hit mismatch."
    FAILED=1
fi

# --- Test Case 2: Cache Invalidation on POST ---
curl -s -X POST http://localhost:$PROXY_PORT/data > /dev/null
RES3=$(curl -s http://localhost:$PROXY_PORT/data)

if [ "$RES1" != "$RES3" ]; then
    echo "  [OK]  Mutation purged cached key."
    ((PASSED_COUNT++))
else
    echo "  [FAIL] Mutation failed to purge cache."
    FAILED=1
fi

# --- Test Case 3: s-maxage Precedence ---
RES_S1=$(curl -s http://localhost:$PROXY_PORT/shared-cache)
sleep 4 # s-maxage is set to 3s
RES_S2=$(curl -s http://localhost:$PROXY_PORT/shared-cache)

if [ "$RES_S1" != "$RES_S2" ]; then
    echo "  [OK]  s-maxage evicted key after timeout."
    ((PASSED_COUNT++))
else
    echo "  [FAIL] s-maxage ignored."
    FAILED=1
fi

# --- Test Case 4: Upstream Socket Crash ---
STATUS_CODE=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:$PROXY_PORT/crash)

if [ "$STATUS_CODE" -eq 502 ]; then
    echo "  [OK]  Closed socket returned 502 Bad Gateway."
    ((PASSED_COUNT++))
else
    echo "  [FAIL] Crash test returned HTTP $STATUS_CODE (expected 502)."
    FAILED=1
fi

# --- Cleanup ---
kill $BACKEND_PID $PROXY_PID 2>/dev/null
wait $BACKEND_PID $PROXY_PID 2>/dev/null

echo "Summary: $PASSED_COUNT/4 tests passed."

if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
