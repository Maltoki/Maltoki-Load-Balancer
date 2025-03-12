#pragma once
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstring> // for memcpy

namespace ENCODING {

    // Function to decode a list from bytes
    std::vector<double> decodeList(const std::vector<unsigned char>& data) {
        if (data.size() < 2) {
            throw std::invalid_argument("Insufficient data for decoding");
        }

        bool signedType = data[0];
        char typeIndicator = data[1];
        const unsigned char* rawData = data.data() + 2;

        std::vector<double> result;
        size_t dataSize = data.size() - 2;

        if (typeIndicator == 'I') {
            if (dataSize % 8 != 0) {
                throw std::invalid_argument("Invalid data length for integers");
            }

            for (size_t i = 0; i < dataSize; i += 8) {
                int64_t value;
                std::memcpy(&value, rawData + i, sizeof(value));
                result.push_back(static_cast<double>(value));
            }
        }
        else if (typeIndicator == 'F') {
            if (dataSize % 8 != 0) {
                throw std::invalid_argument("Invalid data length for floats");
            }

            for (size_t i = 0; i < dataSize; i += 8) {
                double value;
                std::memcpy(&value, rawData + i, sizeof(value));
                result.push_back(value);
            }
        }
        else {
            throw std::invalid_argument("Invalid type indicator");
        }

        return result;
    }

    // Function to encode a list into bytes
    std::vector<unsigned char> encodeList(const std::vector<double>& data, bool signedType = true) {
        if (data.empty()) {
            throw std::invalid_argument("Attempt to send empty data");
        }

        std::vector<unsigned char> result;
        result.push_back(static_cast<unsigned char>(signedType));

        result.push_back('F');
        for (unsigned int i = 0; i < data.size(); i++) {
            unsigned char buffer[8];
            std::memcpy(buffer, &data[i], sizeof(data[i]));
            result.insert(result.begin() + result.size(), buffer, buffer + sizeof(data[i]));
        }

        return result;
    }

}