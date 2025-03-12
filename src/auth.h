#pragma once
#include <jwt-cpp/jwt.h>
#include <string>
#include <exception>
#include <chrono>
#include <string>
#include <map>

// TEST STUFF
constexpr auto TEST_SECRET = "jajladnadnladldwldalndawladwnlladwldawjadhwbdwbwbjdbwj";
static std::map<std::string, std::string> api_keys;

//\\TEST STUFF

std::string sha_256_hash(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

void add_api_key_to_database(std::string user_id, std::string api_key) {
    // Change to an actual database
    api_keys[user_id] = api_key;
}

std::string get_user_secret_from_database(std::string user_id) {
    if (api_keys.count(user_id)) {
        return api_keys[user_id];
    }
    throw std::exception("Unknown User ID");
}

bool validate_jwt_token(const std::string& token) {
    try {
        // Decode the token
        auto decoded = jwt::decode(token);

        auto hash_user_id = decoded.get_subject();
        
        if (sha_256_hash(decoded.get_payload_claim("user_secret").as_string()) != get_user_secret_from_database(hash_user_id))
            return false;

        std::string subscription_tier = "BASIC";
        // TODO: REPLACE WITH GET FROM DATABASE ^

        // Create a verifiers
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{ TEST_SECRET }) // TODO: Replace with your api key retrieved from database based on user id
            .with_issuer("maltoki.api")
            .with_type("JWT")
            .with_subject(hash_user_id)
            .with_claim("tier", jwt::claim(subscription_tier));

        // Verify the decoded token. If any check fails, an exception is thrown.
        verifier.verify(decoded);
        return true;
    }
    catch (const std::exception& e) {
        // Token validation failed (invalid signature, expired, wrong issuer, etc.)
        return false;
    }
}

std::string generate_jwt_token(const std::string& user_id, const std::string& user_secret, const std::string& subscription_tier) {
    auto now = std::chrono::system_clock::now();

    auto hash_user_id = sha_256_hash(user_id);
    auto hash_user_secret = sha_256_hash(user_secret);

    // Create and sign the token
    std::string token = jwt::create()
        .set_issuer("maltoki.api")
        .set_type("JWT")
        .set_subject(hash_user_id)
        .set_issued_at(now)
        .set_payload_claim("tier", jwt::claim(subscription_tier))
        .set_payload_claim("user_secret", jwt::claim(user_secret))
        .sign(jwt::algorithm::hs256{ TEST_SECRET });

    add_api_key_to_database(hash_user_id, hash_user_secret);

    return token;
}