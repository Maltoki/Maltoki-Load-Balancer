#include <crow.h>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstring> // For memset()
#include "request_handler.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: ./server.exe key_file_path socket_server_port api_port\n";
        return 1;
    }

    std::cout << "STARTING SERVER thing"
        << "\n    API PORT    [" << argv[3] << "]"
        << "\n    WORKER PORT [" << argv[2] << "]"
        << "\n    KEY_PATH    " << argv[1] << "\n\n";

    auto key_path = argv[1];
    auto socket_server_port = std::stoi(argv[2]);
    auto api_port = std::stoi(argv[3]);
    std::string host("0.0.0.0");
    RequestHandler * request_handler = new RequestHandler(host, socket_server_port);
    if (!request_handler->readKey(key_path))
        return 1;

    crow::SimpleApp app;

    CROW_ROUTE(app, "/workload/<string>").methods(crow::HTTPMethod::Post)([request_handler](const crow::request& req, std::string client_type) {
        if (client_type == "Auth" || !request_handler->service_exists(client_type))
            return crow::response(404, "The requested service does not exist.");
        crow::json::rvalue json_body = crow::json::load(req.body);
        if (!json_body)
            return crow::response(400, "Invalid JSON");

        const auto& auth_header = req.get_header_value("Authorization");

        if (auth_header.empty() || auth_header.substr(0, 7) != "Bearer ") {
            return crow::response(401, "Unauthorized: No token provided");
        }

        std::string auth_token = auth_header.substr(7);  // Extract the token after "Bearer "

        try {
            if (!request_handler->validate_api_key(auth_token))
                return crow::response(401, "Invalid authorization token.");
        }
        catch (std::runtime_error& e) {
            return crow::response(503, "Authorization service is currently unavailable.");
        }
        
        

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

    CROW_ROUTE(app, "/auth/<string>").methods(crow::HTTPMethod::Post)([request_handler](const crow::request& req, std::string service) {
        if (service == "register") {
            crow::json::rvalue json_body = crow::json::load(req.body);
            if (
                json_body.has("email") &&
                json_body.has("password") &&
                json_body.has("re-password") &&
                json_body.has("first_name") &&
                json_body.has("last_name") &&
                json_body.has("DOB")
                ) {
                auto res = request_handler->register_account(json_body["email"].s(), json_body["password"].s(), json_body["re-password"].s(), json_body["first_name"].s(), json_body["last_name"].s(), json_body["DOB"].s());
                if (res.has("success") && !res["success"].b())
                    return crow::response(401, res.has("error") ? std::string(res["error"].s()) : "Registration failed.");
                crow::json::wvalue res_writer = std::move(res);
                return crow::response(200, res_writer.dump());
            }
            return crow::response(401, "Missing required fields.");
        }
        else if (service == "login") {
            crow::json::rvalue json_body = crow::json::load(req.body);
            if (
                json_body.has("email") &&
                json_body.has("password")
                ) {
                auto res = request_handler->login_account(json_body["email"].s(), json_body["password"].s());
                if (res.has("success") && !res["success"].b())
                    return crow::response(401, res.has("error") ? std::string(res["error"].s()) : "Login failed.");
                crow::json::wvalue res_writer = std::move(res);
                return crow::response(200, res_writer.dump());
            }
            return crow::response(401, "Missing required fields.");
        }
        return crow::response(404, "The requested auth service does not exist.");
    });

    std::thread([&request_handler]() {request_handler->start();}).detach();
    app.port(api_port).multithreaded().run();
    delete request_handler;
    return 0;
}
