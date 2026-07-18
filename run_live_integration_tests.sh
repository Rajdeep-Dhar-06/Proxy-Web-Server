#!/bin/bash

# Configuration ports
PROXY_PORT=8080
LIVE_ORIGIN="https://dummyjson.com"

echo "Running live integration tests ($LIVE_ORIGIN)..."

# 1. Clean up any existing instances on proxy port
kill -9 $(lsof -t -i:$PROXY_PORT) 2>/dev/null
sleep 1

# 2. Start Caching Proxy pointing to live HTTPS server
./build/caching-proxy --port $PROXY_PORT --origin $LIVE_ORIGIN > /dev/null 2>&1 &
PROXY_PID=$!
sleep 2 # Allow proxy to bind and initialize SSL contexts

# Verify proxy is running
if ! kill -0 $PROXY_PID 2>/dev/null; then
    echo "[ERROR] Failed to start proxy server pointing to $LIVE_ORIGIN"
    exit 1
fi

FAILED=0
PASSED_COUNT=0

# --- Test Case 1: Proxy content successfully from live HTTPS server ---
RES1=$(curl -s -i http://localhost:$PROXY_PORT/products/1)
STATUS=$(echo "$RES1" | grep -i "^HTTP" | awk '{print $2}')
BODY=$(echo "$RES1" | grep -v '^[A-Z]' | grep -v '^[a-z]' | tr -d '\r\n')

if [ "$STATUS" -eq 200 ] && [[ "$BODY" == *"Essence Mascara Lash Princess"* ]]; then
    echo "  [OK]  Proxied HTTP/HTTPS content successfully."
    ((PASSED_COUNT++))
else
    echo "  [FAIL] Live proxy content mismatch (Status: $STATUS)."
    FAILED=1
fi

# --- Test Case 2: Ensure RFC cache-bypass rules are respected (no-store) ---
XCACHE_REQ1=$(curl -s -i http://localhost:$PROXY_PORT/products/1 | grep -i "x-cache" | tr -d '\r\n' | awk '{print $2}')
XCACHE_REQ2=$(curl -s -i http://localhost:$PROXY_PORT/products/1 | grep -i "x-cache" | tr -d '\r\n' | awk '{print $2}')

# Convert to uppercase for standard checks
XCACHE_REQ1_UP=$(echo "$XCACHE_REQ1" | tr '[:lower:]' '[:upper:]')
XCACHE_REQ2_UP=$(echo "$XCACHE_REQ2" | tr '[:lower:]' '[:upper:]')

if [ "$XCACHE_REQ1_UP" = "MISS" ] && [ "$XCACHE_REQ2_UP" = "MISS" ]; then
    echo "  [OK]  Cache-Control: no-store bypassed cache."
    ((PASSED_COUNT++))
else
    echo "  [FAIL] Cache bypass check failed (Req1: $XCACHE_REQ1, Req2: $XCACHE_REQ2)."
    FAILED=1
fi

# --- Cleanup ---
kill $PROXY_PID 2>/dev/null
wait $PROXY_PID 2>/dev/null

echo "Summary: $PASSED_COUNT/2 tests passed."

if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
