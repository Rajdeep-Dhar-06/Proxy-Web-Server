#include "network/HttpContext.hpp"
#include "network/HTTPParser.hpp"
#include <sockpp/tcp_socket.h>
#include <string>

std::string get_status_message(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        default: return "OK";
    }
}

void read_and_parse_request(HttpContext& ctx) {
    ctx.request = parse_request(ctx.socket);
}

std::string serialize_response(HttpContext& ctx) {
    std::string status_msg = get_status_message(ctx.response.status_code);
    std::string raw_resp = "HTTP/1.1 " + std::to_string(ctx.response.status_code) + " " + status_msg + "\r\n";

    // Set Content-Length if body is present and it is not already set
    if (ctx.response.headers.find("Content-Length") == ctx.response.headers.end() &&
        ctx.response.headers.find("content-length") == ctx.response.headers.end() &&
        ctx.response.headers.find("Transfer-Encoding") == ctx.response.headers.end() &&
        ctx.response.headers.find("transfer-encoding") == ctx.response.headers.end()) {
        ctx.response.headers["Content-Length"] = std::to_string(ctx.response.body.size());
    }

    // Write all headers
    for (const auto& pair : ctx.response.headers) {
        raw_resp += pair.first + ": " + pair.second + "\r\n";
    }

    raw_resp += "\r\n"; // End of headers block
    raw_resp += ctx.response.body;
    return raw_resp;
}

void send_response(HttpContext& ctx) {
    if (ctx.response.committed) {
        return;
    }
    std::string raw_resp = serialize_response(ctx);
    ctx.socket.write_n(raw_resp.data(), raw_resp.size());
    ctx.response.committed = true;
}

void inject_cache_header(std::string& response, const std::string& header) {
    auto pos = response.find("\r\n\r\n");
    if (pos != std::string::npos) {
        response.insert(pos + 2, header);
    }
}
