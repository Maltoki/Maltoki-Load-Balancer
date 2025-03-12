#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <openssl/sha.h>


namespace ENCRYPT {

    // Constants for AES
    constexpr size_t AES_KEY_SIZE = 32; // 256 bits
    constexpr size_t AES_BLOCK_SIZE = 16;

    // Function to generate a random IV
    std::vector<unsigned char> generateIV() {
        std::vector<unsigned char> iv(AES_BLOCK_SIZE);
        if (!RAND_bytes(iv.data(), AES_BLOCK_SIZE)) {
            throw std::runtime_error("Failed to generate IV");
        }
        return iv;
    }


    // AES-256-CBC Encryption
    std::vector<unsigned char> encryptBytes(const std::vector<unsigned char>& plaintext, const std::vector<unsigned char>& key) {
        if (key.size() != AES_KEY_SIZE) throw std::runtime_error("Invalid key size");

        std::vector<unsigned char> iv = generateIV();
        std::vector<unsigned char> plaintextCopy = plaintext;

        std::vector<unsigned char> ciphertext(plaintextCopy.size() + AES_BLOCK_SIZE);
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create EVP_CIPHER_CTX");

        try {
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data())) {
                throw std::runtime_error("EVP_EncryptInit_ex failed");
            }

            int len = 0;
            int ciphertext_len = 0;
            if (!EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintextCopy.data(), plaintextCopy.size())) {
                throw std::runtime_error("EVP_EncryptUpdate failed");
            }
            ciphertext_len = len;

            if (!EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
                throw std::runtime_error("EVP_EncryptFinal_ex failed");
            }
            ciphertext_len += len;

            EVP_CIPHER_CTX_free(ctx);

            ciphertext.resize(ciphertext_len);

            // Prepend IV to ciphertext
            ciphertext.insert(ciphertext.begin(), iv.begin(), iv.begin() + iv.size());

            return ciphertext;
        }
        catch (std::exception& e) {
            EVP_CIPHER_CTX_free(ctx);
            throw e;
        }
    }

    // AES-256-CBC Decryption
    std::vector<unsigned char> decryptBytes(const std::vector<unsigned char>& inBytes, const std::vector<unsigned char>& key) {
        if (key.size() != AES_KEY_SIZE) {
            throw std::runtime_error("Invalid key size");
        }

        if (inBytes.size() < AES_BLOCK_SIZE) {
            throw std::runtime_error("Ciphertext too short");
        }

        // Extract the IV and ciphertext
        std::vector<unsigned char> iv(inBytes.begin(), inBytes.begin() + AES_BLOCK_SIZE);
        std::vector<unsigned char> ciphertext(inBytes.begin() + AES_BLOCK_SIZE, inBytes.end());

        std::vector<unsigned char> decryptedMessage(ciphertext.size());

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
        }

        try {
            // Initialize decryption operation
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data())) {
                throw std::runtime_error("EVP_DecryptInit_ex failed");
            }

            int len = 0;
            int plaintext_len = 0;

            // Decrypt the ciphertext
            if (!EVP_DecryptUpdate(ctx, decryptedMessage.data(), &len, ciphertext.data(), ciphertext.size())) {
                throw std::runtime_error("EVP_DecryptUpdate failed");
            }
            plaintext_len = len;

            // Finalize the decryption
            if (!EVP_DecryptFinal_ex(ctx, decryptedMessage.data() + len, &len)) {
                throw std::runtime_error("EVP_DecryptFinal_ex failed - incorrect key or corrupted data");
            }
            plaintext_len += len;

            // Free the context
            EVP_CIPHER_CTX_free(ctx);

            // Resize decrypted message
            decryptedMessage.resize(plaintext_len);

            // Unpad the decrypted message (matching Python's PKCS7 unpadding)
            return decryptedMessage;
        }
        catch (const std::exception& e) {
            std::cerr << e.what();
            EVP_CIPHER_CTX_free(ctx);
            throw e;
        }
    }

}
