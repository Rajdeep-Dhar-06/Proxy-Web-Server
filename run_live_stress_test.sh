#!/bin/bash

PROXY_PORT=8080
LIVE_ORIGIN="https://jsonplaceholder.typicode.com"

echo "Running live stress test ($LIVE_ORIGIN)..."

# 1. Clean up any existing proxy instances
kill -9 $(lsof -t -i:$PROXY_PORT) 2>/dev/null
sleep 1

# 2. Start Caching Proxy pointing to jsonplaceholder
./build/caching-proxy --port $PROXY_PORT --origin $LIVE_ORIGIN > /dev/null 2>&1 &
PROXY_PID=$!
sleep 2

if ! kill -0 $PROXY_PID 2>/dev/null; then
    echo "[ERROR] Failed to start proxy server."
    exit 1
fi

# Track request statuses in temporary files
TEMP_DIR=$(mktemp -d)

# 3. Fire 100 requests concurrently (25 of each method)
echo "  -> Dispatching 100 concurrent HTTP requests (GET, POST, PUT, DELETE)..."

CURL_PIDS=()
for i in {1..25}; do
    # GET
    curl -s --connect-timeout 4 --max-time 12 -o /dev/null -w "%{http_code}\n" http://localhost:$PROXY_PORT/posts/1 > "$TEMP_DIR/get_$i" &
    CURL_PIDS+=($!)
    
    # POST
    curl -s --connect-timeout 4 --max-time 12 -o /dev/null -w "%{http_code}\n" -X POST -H "Content-Type: application/json" -d '{"title":"foo","body":"bar","userId":1}' http://localhost:$PROXY_PORT/posts > "$TEMP_DIR/post_$i" &
    CURL_PIDS+=($!)
    
    # PUT
    curl -s --connect-timeout 4 --max-time 12 -o /dev/null -w "%{http_code}\n" -X PUT -H "Content-Type: application/json" -d '{"id":1,"title":"updated","body":"bar","userId":1}' http://localhost:$PROXY_PORT/posts/1 > "$TEMP_DIR/put_$i" &
    CURL_PIDS+=($!)
    
    # DELETE
    curl -s --connect-timeout 4 --max-time 12 -o /dev/null -w "%{http_code}\n" -X DELETE http://localhost:$PROXY_PORT/posts/1 > "$TEMP_DIR/delete_$i" &
    CURL_PIDS+=($!)
done

# Wait for all background requests to finish
wait "${CURL_PIDS[@]}"

# 4. Check results
PASSED_COUNT=0
FAILED_COUNT=0

for file in "$TEMP_DIR"/*; do
    CODE=$(cat "$file")
    # GET, PUT, DELETE return 200. POST returns 201.
    if [ "$CODE" -eq 200 ] || [ "$CODE" -eq 201 ]; then
        ((PASSED_COUNT++))
    else
        ((FAILED_COUNT++))
    fi
done

rm -rf "$TEMP_DIR"

# Cleanup proxy
kill $PROXY_PID 2>/dev/null
wait $PROXY_PID 2>/dev/null

echo "Summary: $PASSED_COUNT/100 requests completed successfully."

if [ $FAILED_COUNT -eq 0 ]; then
    echo "  [OK]  All concurrent requests completed with success codes."
    exit 0
else
    echo "  [FAIL] $FAILED_COUNT concurrent requests failed or timed out."
    exit 1
fi
