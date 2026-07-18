const http = require('http');

const PORT = 9000;

const server = http.createServer((req, res) => {
  console.log(`[Mock Upstream] Received: ${req.method} ${req.url}`);

  // Flow Path 1: POST/PUT/DELETE on /data (Cache Invalidation)
  if (req.url === '/data' && ['POST', 'PUT', 'DELETE'].includes(req.method)) {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ message: `${req.method} successful! Cache should now be purged.` }));
    return;
  }

  // Flow Path 2: Standard Cacheable response
  if (req.url === '/data' && req.method === 'GET') {
    res.writeHead(200, {
      'Content-Type': 'application/json',
      'Cache-Control': 'public, max-age=10',
    });
    res.end(JSON.stringify({ message: "Hello from Upstream!", timestamp: Date.now() }));
  }

  // Flow Path 3: s-maxage vs max-age precedence (RFC 7234)
  else if (req.url === '/shared-cache') {
    res.writeHead(200, {
      'Content-Type': 'application/json',
      'Cache-Control': 'public, max-age=100, s-maxage=3', // Proxy should evict after 3s, not 100s
    });
    res.end(JSON.stringify({ message: "Prioritizes s-maxage", timestamp: Date.now() }));
  }

  // Flow Path 4: Latency & Coalescing simulation
  else if (req.url === '/slow') {
    setTimeout(() => {
      res.writeHead(200, {
        'Content-Type': 'application/json',
        'Cache-Control': 'public, max-age=10',
      });
      res.end(JSON.stringify({ message: "Slow response", timestamp: Date.now() }));
    }, 1000);
  }

  // Flow Path 5: Non-cacheable directive (no-store)
  else if (req.url === '/private') {
    res.writeHead(200, {
      'Content-Type': 'application/json',
      'Cache-Control': 'no-store',
    });
    res.end(JSON.stringify({ message: "Sensitive data", timestamp: Date.now() }));
  }

  // Flow Path 6: Empty Body Handling (204 No Content)
  else if (req.url === '/empty') {
    res.writeHead(204); // No body
    res.end();
  }

  // Flow Path 7: Chunked Transfer Encoding
  else if (req.url === '/chunked') {
    res.writeHead(200, {
      'Content-Type': 'text/plain',
      'Transfer-Encoding': 'chunked',
    });
    res.write("Chunk 1...\n");
    setTimeout(() => {
      res.write("Chunk 2...\n");
      setTimeout(() => {
        res.end("Done!");
      }, 300);
    }, 300);
  }

  // Flow Path 8: Upstream unexpected connection drop (502 Bad Gateway)
  else if (req.url === '/crash') {
    req.socket.destroy(); // Instantly destroy TCP socket without sending headers
  }

  // Flow Path 9: Upstream Read Timeout simulation (504 Gateway Timeout)
  else if (req.url === '/hang') {
    // Keep socket open but do absolutely nothing (times out the proxy)
  }

  // Fallback
  else {
    res.writeHead(404);
    res.end("Not Found");
  }
});

server.listen(PORT, () => {
  console.log(`Mock Upstream running at http://localhost:${PORT}`);
});
