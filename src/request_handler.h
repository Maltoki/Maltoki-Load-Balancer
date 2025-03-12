#pragma once
#include <iostream>
#include <asio.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>
#include <queue>
#include <set>
#include <random>
#include <thread>
#include <mutex>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <tuple>
#include <condition_variable>
#include "encryption.h"

typedef std::vector<unsigned char> request_body_type;

typedef std::queue<std::tuple<long, request_body_type>> request_queue_type;

typedef std::unordered_map<std::string, request_queue_type> request_map_type;


const std::vector<unsigned char> TIMEOUT_BYTESTRING = { 'T','I','M','E','O','U','T' };

class RequestHandler {
public:
    RequestHandler(const std::string& host, int port):
        acceptor_(io_context_, asio::ip::tcp::endpoint(asio::ip::make_address(host), port)),
        key(nullptr),
        request_queues(new request_map_type),
        responses(new std::unordered_map<long, request_body_type>),
        in_use_request_ids(new std::set<long>)
    {}
    ~RequestHandler() {
        if (key)
            delete key;
        delete request_queues;
        delete responses;
        delete in_use_request_ids;
    }

    bool readKey(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return false;  // Return an empty vector on failure
        }

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char>* buffer = new std::vector<unsigned char>(fileSize);
        file.read(reinterpret_cast<char*>(buffer->data()), fileSize);

        if (!file) {
            std::cerr << "Error reading file: " << filename << std::endl;
            return false;  // Return an empty vector on failure
        }

        this->key = buffer;

        return true;  // Return the vector containing the file contents
    }

    void start() {
        do_accept();
        io_context_.run();
    }

    const request_body_type handle_request(const std::string & client_type, const request_body_type & out_packet) {
        long id = 0;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            id = get_request_id();
            this->request_queues->at(client_type).push(std::make_tuple(id, std::move(out_packet)));
            queue_cv.notify_all();
        }

        // Wait for response
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [this, id] { return responses->count(id) > 0; });
        }

        request_body_type response = responses->at(id);
        responses->erase(id);
        in_use_request_ids->erase(id);

        return response;
    }

    long get_request_id() {
        if (!in_use_request_ids->empty()) {
            // Get the maximum element using rbegin()
            long id = *in_use_request_ids->rbegin() + 1;
            in_use_request_ids->insert(id);
            return id;
        }
        else {
            // set empty
            long id = 1;
            in_use_request_ids->insert(id);
            return id;
        }
    }

public:
    std::mutex queue_mutex;
    std::condition_variable queue_cv;

private:
    std::set<long>* in_use_request_ids;
    asio::io_context io_context_;
    asio::ip::tcp::acceptor acceptor_;
    request_map_type* request_queues;
    // stores the request_id and in_data in a queue under client_type name
    std::unordered_map<long, request_body_type>* responses;
    std::vector<unsigned char>* key;
    std::mutex response_mutex;

    void do_accept() {
        acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                std::cout << "\n + CONNECTION -*-> {" << socket.remote_endpoint() << "}\n";
                std::thread(&RequestHandler::handle_client, this, std::move(socket)).detach();
            }
            do_accept();
        });
    }

    void handle_client(asio::ip::tcp::socket socket) {
        request_body_type out_packet;
        long request_id = 0;
        // the double is either a 64 bit integer or floats cast to longs.
        try {
            // Example of reading and writing data
            // Implement encryption, decryption, and other logic here

            // Example: Read client type
            std::vector<unsigned char> client_type_buffer(1024);
            asio::error_code error;
            size_t length = socket.read_some(asio::buffer(client_type_buffer), error);
            if (handle_error(error) == asio::error::eof)
                return; // Connection closed cleanly by peer

            std::string client_type(client_type_buffer.begin(), client_type_buffer.begin() + length);

            std::cout << "\n + CLIENT -*-> [" << client_type << "]\n";

            request_queue_type* local_queue = nullptr;

            // SET THE QUEUE FOR THIS THREAD TO USE:
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (this->request_queues->count(client_type) > 0) {
                    local_queue = &this->request_queues->at(client_type);
                }
                else {
                    this->request_queues->insert(std::make_pair(client_type, request_queue_type()));
                    local_queue = &this->request_queues->at(client_type);
                    std::cout << "\n + ADDED -*-> [" << client_type << "]\n";
                }
            }

            auto failure = false;

            // Process client type and implement the rest of the logic
            // Use ENCRYPT::encryptBytes() and ENCRYPT::decryptBytes() for encryption and decryption
            
            while (true) {
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    queue_cv.wait(lock, [local_queue, failure] { return !local_queue->empty() || failure; });
                    if (failure)
                        out_packet = out_packet;
                    else {
                        request_id = std::get<0>(local_queue->front());
                        out_packet = std::get<1>(local_queue->front());
                        local_queue->pop();
                    }
                }

                auto out_cypher_msg_raw = ENCRYPT::encryptBytes(out_packet, *key);

                //// SEND CYPHER'D PACKET
                // Format the length as a 6-character hexadecimal string
                std::stringstream ss;
                ss << std::setw(6) << std::setfill('0') << std::hex << out_cypher_msg_raw.size();
                std::string length_header = ss.str();

                // Prepare the full message to send
                std::vector<unsigned char> message(length_header.begin(), length_header.begin() + 6);
                message.insert(message.end(), out_cypher_msg_raw.begin(), out_cypher_msg_raw.end());

                // Send the message
                socket.write_some(asio::buffer(message), error);
                if (handle_error(error)) {
                    std::cerr << "Error sending data: " << error.message() << std::endl;
                    failure = true;
                    continue;
                }
                else {
                    std::cout << "\n [" << client_type << "] SENT ~~~> ";
                    std::cout << "\"" << std::string(out_packet.begin(), out_packet.end()) << "\"\n\n";
                }

                /// RECIEVE RESPONSE
                std::vector<unsigned char> client_response_buffer(1024);
                asio::error_code error;
                size_t length = socket.read_some(asio::buffer(client_response_buffer), error);
                if (handle_error(error) == asio::error::eof)
                    break; // Connection closed cleanly by peer

                client_response_buffer.resize(length);

                if (client_response_buffer.size() == 0)
                    break;

                if (!std::equal(TIMEOUT_BYTESTRING.begin(), TIMEOUT_BYTESTRING.end(), client_response_buffer.begin())) {
                    auto recv_data = ENCRYPT::decryptBytes(client_response_buffer, *key);

                    std::cout << "\n [" << client_type << "] RECEIVED <~~~ ";
                    std::cout << "\"" << std::string(recv_data.begin(), recv_data.end()) << "\"\n\n";
                    
                    {
                        std::lock_guard<std::mutex> lock(queue_mutex);
                        responses->insert_or_assign(request_id, recv_data);
                    }
                    queue_cv.notify_all();

                    failure = false;
                    continue;
                }
                else {
                    std::cout << "\n !!! XXX !!! [" << client_type << "] RECEIVE FAILED\n"
                    << "    REASON : TIMEOUT\n"
                    << "    SENT   : ";
                    std::cout << "\"" << std::string(out_packet.begin(), out_packet.end()) << "\"\n\n";
                }

            }
            
        }
        catch (std::exception& e) {
            std::cerr << e.what();
        }
    }

    const asio::error_code & handle_error(const asio::error_code & error) {
        // this function throws for the appropriate errors to remove the connection
        switch (error.value())
        {
        case asio::error::eof:
        case asio::error::connection_reset:
        case asio::error::connection_aborted:
            throw std::runtime_error("\n - CLIENT <--- !" + error.message() + "!\n");
            break;

        case asio::error::network_down:
        case asio::error::network_unreachable:
        case asio::error::network_reset:
            throw std::runtime_error("\n - CLIENT <--- !" + error.message() + "!\n");
            break;

        case asio::error::access_denied:
        case asio::error::no_buffer_space:
        case asio::error::no_descriptors:
            throw std::runtime_error("\n - CLIENT <--- !" + error.message() + "!\n");
            break;
        }
        return error;
    }
};

