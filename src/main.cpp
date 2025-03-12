#include <crow.h>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstring> // For memset()
#include "auth.h"
#include "request_handler.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: ./server.exe key_file_path socket_server_port api_port\n";
        return 1;
    }

    std::cout << "STARTING SERVER"
        << "\n    API PORT    [" << argv[3] << "]"
        << "\n    WORKER PORT [" << argv[2] << "]"
        << "\n    KEY_PATH    " << argv[1] << "\n\n";

    auto key_path = argv[1];
    auto socket_server_port = std::stoi(argv[2]);
    auto api_port = std::stoi(argv[3]);
    std::string host("127.0.0.1");
    RequestHandler * request_handler = new RequestHandler(host, socket_server_port);
    if (!request_handler->readKey(key_path))
        return 1;

    crow::SimpleApp app;

    CROW_ROUTE(app, "/workload/<string>").methods(crow::HTTPMethod::Post)([request_handler](const crow::request& req, std::string client_type) {
        crow::json::rvalue json_body = crow::json::load(req.body);
        if (!json_body)
            return crow::response(400, "Invalid JSON");

        const auto& auth_header = req.get_header_value("Authorization");

        if (auth_header.empty() || auth_header.substr(0, 7) != "Bearer ") {
            return crow::response(401, "Unauthorized: No token provided");
        }

        std::string token = auth_header.substr(7);  // Extract the token after "Bearer "

        if (!validate_jwt_token(token))
            return crow::response(401, "Invalid authorization token.");
        

        if (json_body.has("body")) {
            crow::json::wvalue json_body_writer = std::move(json_body);
            std::string json_str = json_body_writer["body"].dump();
            request_body_type in_vec(json_str.begin(), json_str.end());
            std::cout << "\n HTTP -("<< json_str <<")-> [" << client_type << "]\n";

            request_body_type raw_response = request_handler->handle_request(client_type, in_vec);

            std::string successful_response_string(raw_response.begin(), raw_response.end());

            return crow::response(200, "application/json", successful_response_string);
        } else
            return crow::response(400, "Request must have a \"body\" key top level. EX: {\"body\":[1,2,3,4,5]}");
    });

    CROW_ROUTE(app, "/test/GENAPIKEY/<string>/<string>").methods(crow::HTTPMethod::Post)([request_handler](const crow::request& req, std::string TIER, std::string USER_SECRET) {

        return crow::response(200, "text/plain", generate_jwt_token("William", USER_SECRET, TIER));
    });

    std::thread([&request_handler]() {request_handler->start();}).detach();
    app.port(api_port).multithreaded().run();
    delete request_handler;
    return 0;
}
