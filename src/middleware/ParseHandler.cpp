#include "middleware/ParseHandler.hpp"
#include <exception>

#include "utils/read_parse_request.hpp"
#include "utils/serialize_send_response.hpp"

void ParseHandler::process(HttpContext& ctx) {
    try {
        read_and_parse_request(ctx);
        // Parsing succeeded. Pass control to the next handler (CacheHandler)
        Middleware::process(ctx);
    } 
    catch (const std::exception& e) {
        // Short-circuit: Create error response and do NOT call Middleware::process()
        ctx.response.status_code = 400;
        ctx.response.body = "Malformed HTTP request";
        send_response(ctx);
    }
}