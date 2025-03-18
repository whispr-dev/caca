// sha1.cpp
#include "sha1.hpp"
#include <cstring>

namespace nist_sts {
namespace generators {

SHA1Generator::SHA1Generator() {
    // Initialize key with default seed
    std::string keyHex = "ec822a619d6ed5d9492218a7a4c5b15d57c61601";
    hexToBytes(keyHex, key);
}

BitSequence SHA1Generator::generate(size_t length) {
    BitSequence sequence(length);
    
    // Number of bytes needed to generate requested bits
    size_t numBytes = (length + 7) / 8;
    
    // Number of SHA-1 operations needed (20 bytes per SHA-1 output)
    size_t numOperations = (numBytes + 19) / 20;
    
    // Buffer to hold generated bytes
    std::vector<uint8_t> buffer(numOperations * 20);
    
    for (size_t i = 0; i < numOperations; i++) {
        // Prepare message: key (X) || zero padding
        std::vector<uint8_t> message(64, 0);
        std::copy(key.begin(), key.end(), message.begin());
        
        // Generate SHA-1 digest
        std::vector<uint8_t> digest(20);
        sha1Process(message, digest);
        
        // Copy digest to buffer
        std::copy(digest.begin(), digest.end(), buffer.begin() + i * 20);
        
        // Update key = G + one (key incremented by 1)
        key = digest;
        uint8_t carry = 1;
        for (auto it = key.rbegin(); it != key.rend() && carry; ++it) {
            uint16_t sum = *it + carry;
            *it = sum & 0xFF;
            carry = sum >> 8;
        }
    }
    
    // Convert bytes to bits
    for (size_t i = 0; i < length; i++) {
        size_t byteIndex = i / 8;
        size_t bitIndex = 7 - (i % 8);  // MSB first
        sequence[i] = (buffer[byteIndex] >> bitIndex) & 0x01;
    }
    
    return sequence;
}

void SHA1Generator::sha1Process(const std::vector<uint8_t>& message, std::vector<uint8_t>& digest) {
    // SHA-1 constants
    const uint32_t K[] = {
        0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
    };
    
    // Initialize hash values
    uint32_t H[] = {
        0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
    };
    
    // Process the message in 512-bit chunks
    for (size_t chunk = 0; chunk < message.size(); chunk += 64) {
        uint32_t W[80];
        
        // Prepare message schedule
        for (size_t t = 0; t < 16; t++) {
            W[t] = (message[chunk + t*4] << 24) |
                   (message[chunk + t*4 + 1] << 16) |
                   (message[chunk + t*4 + 2] << 8) |
                   (message[chunk + t*4 + 3]);
        }
        
        for (size_t t = 16; t < 80; t++) {
            W[t] = (W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
            W[t] = (W[t] << 1) | (W[t] >> 31);  // Left rotate 1
        }
        
        // Initialize working variables
        uint32_t a = H[0];
        uint32_t b = H[1];
        uint32_t c = H[2];
        uint32_t d = H[3];
        uint32_t e = H[4];
        
        // Main loop
        for (size_t t = 0; t < 80; t++) {
            uint32_t temp = ((a << 5) | (a >> 27)) + e + W[t];  // Left rotate a by 5
            
            if (t < 20) {
                temp += ((b & c) | ((~b) & d)) + K[0];
            } else if (t < 40) {
                temp += (b ^ c ^ d) + K[1];
            } else if (t < 60) {
                temp += ((b & c) | (b & d) | (c & d)) + K[2];
            } else {
                temp += (b ^ c ^ d) + K[3];
            }
            
            e = d;
            d = c;
            c = (b << 30) | (b >> 2);  // Left rotate b by 30
            b = a;
            a = temp;
        }
        
        // Add the compressed chunk to the current hash value
        H[0] += a;
        H[1] += b;
        H[2] += c;
        H[3] += d;
        H[4] += e;
    }
    
    // Produce the final hash value
    for (size_t i = 0; i < 5; i++) {
        digest[i*4] = (H[i] >> 24) & 0xFF;
        digest[i*4 + 1] = (H[i] >> 16) & 0xFF;
        digest[i*4 + 2] = (H[i] >> 8) & 0xFF;
        digest[i*4 + 3] = H[i] & 0xFF;
    }
}

void SHA1Generator::hexToBytes(const std::string& hex, std::vector<uint8_t>& bytes) {
    bytes.resize(hex.length() / 2);
    
    for (size_t i = 0; i < bytes.size(); i++) {
        std::string byteString = hex.substr(i * 2, 2);
        bytes[i] = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
    }
}

} // namespace generators
} // namespace nist_sts