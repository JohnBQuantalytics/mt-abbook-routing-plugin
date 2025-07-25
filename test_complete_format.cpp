//+------------------------------------------------------------------+
//| Complete Format Test - Build from Working Minimal Format       |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class CompleteFormatTester {
private:
    std::string ml_ip = "188.245.254.12";
    int ml_port = 50051;
    
    std::string EncodeVarint(uint64_t value) {
        std::string result;
        while (value >= 0x80) {
            result += (char)((value & 0x7F) | 0x80);
            value >>= 7;
        }
        result += (char)(value & 0x7F);
        return result;
    }
    
    std::string EncodeFloat(int field_number, float value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 5;
        result += EncodeVarint(field_tag);
        char* bytes = (char*)&value;
        for (int i = 0; i < 4; i++) {
            result += bytes[i];
        }
        return result;
    }
    
    std::string EncodeString(int field_number, const std::string& value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 2;
        result += EncodeVarint(field_tag);
        result += EncodeVarint(value.length());
        result += value;
        return result;
    }
    
    std::string CreateLengthPrefix(const std::string& body) {
        std::string message;
        uint32_t length = body.length();
        message += (char)((length >> 24) & 0xFF);
        message += (char)((length >> 16) & 0xFF);
        message += (char)((length >> 8) & 0xFF);
        message += (char)(length & 0xFF);
        message += body;
        return message;
    }
    
    float ParseScore(const std::string& response) {
        if (response.length() < 9) return -1.0f; // Not enough data
        
        // Skip length prefix (4 bytes)
        const char* protobuf_data = response.c_str() + 4;
        int protobuf_length = response.length() - 4;
        
        // Look for field 1, wire type 5 (score field)
        for (int i = 0; i < protobuf_length - 4; i++) {
            if ((unsigned char)protobuf_data[i] == 0x0D) { // Field 1, wire type 5
                float score;
                memcpy(&score, protobuf_data + i + 1, 4);
                return score;
            }
        }
        
        // Look for field 2, wire type 5 (alternative score field)
        for (int i = 0; i < protobuf_length - 4; i++) {
            if ((unsigned char)protobuf_data[i] == 0x15) { // Field 2, wire type 5
                float score;
                memcpy(&score, protobuf_data + i + 1, 4);
                return score;
            }
        }
        
        return -1.0f; // No score found
    }
    
    bool TestFormat(const std::string& protobuf_body, const std::string& description) {
        std::cout << "\n=== " << description << " ===" << std::endl;
        std::cout << "Size: " << protobuf_body.length() << " bytes" << std::endl;
        
        std::string full_message = CreateLengthPrefix(protobuf_body);
        
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        bool success = false;
        
        try {
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            
            int timeout_ms = 5000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(ml_port);
            inet_pton(AF_INET, ml_ip.c_str(), &serverAddr.sin_addr);
            
            if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                std::cout << "❌ Connection failed" << std::endl;
                return false;
            }
            
            if (send(sock, full_message.c_str(), full_message.length(), 0) == SOCKET_ERROR) {
                std::cout << "❌ Send failed" << std::endl;
                return false;
            }
            
            char response[256];
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received > 0) {
                std::string response_str(response, bytes_received);
                float score = ParseScore(response_str);
                
                if (score >= 0.0f && score <= 1.0f) {
                    std::cout << "✅ SUCCESS: Score = " << score << std::endl;
                    std::cout << "🎯 Routing: " << (score >= 0.08f ? "B-BOOK" : "A-BOOK") << std::endl;
                    success = true;
                } else {
                    std::cout << "⚠️ Response received but no valid score (got " << score << ")" << std::endl;
                }
            } else if (bytes_received == 0) {
                std::cout << "⚠️ Connection closed by server" << std::endl;
            } else {
                std::cout << "❌ Receive timeout/error" << std::endl;
            }
            
            closesocket(sock);
            WSACleanup();
            
        } catch (...) {
            std::cout << "❌ Exception occurred" << std::endl;
            if (sock != INVALID_SOCKET) closesocket(sock);
            WSACleanup();
        }
        
        return success;
    }
    
public:
    void TestProgressiveFormat() {
        std::cout << "MT4 A/B-book Plugin - Complete Format Test\n";
        std::cout << "==========================================\n";
        std::cout << "Building from working minimal format to complete message\n";
        
        // Test 1: Confirmed working - user_id only
        std::string test1 = EncodeString(51, "16813");
        if (!TestFormat(test1, "BASELINE: user_id only (field 51)")) {
            std::cout << "❌ Baseline test failed - something changed!" << std::endl;
            return;
        }
        
        // Test 2: Add open_price
        std::string test2;
        test2 += EncodeFloat(1, 0.59350f);      // open_price
        test2 += EncodeString(51, "16813");     // user_id
        if (!TestFormat(test2, "Add open_price (field 1)")) {
            std::cout << "❌ Adding open_price broke it. Use user_id only." << std::endl;
            return;
        }
        
        // Test 3: Add required numeric fields
        std::string test3;
        test3 += EncodeFloat(1, 0.59350f);      // open_price
        test3 += EncodeFloat(2, 0.59000f);      // sl
        test3 += EncodeFloat(3, 0.59700f);      // tp
        test3 += EncodeFloat(5, 1.0f);          // lot_volume
        test3 += EncodeString(51, "16813");     // user_id
        if (!TestFormat(test3, "Add core numeric fields")) {
            std::cout << "❌ Too many fields broke it. Reduce to minimum." << std::endl;
            return;
        }
        
        // Test 4: Add all required fields (the full message)
        std::string test4;
        test4 += EncodeFloat(1, 0.59350f);      // open_price
        test4 += EncodeFloat(2, 0.59000f);      // sl
        test4 += EncodeFloat(3, 0.59700f);      // tp
        test4 += EncodeFloat(5, 1.0f);          // lot_volume
        test4 += EncodeString(42, "MT4");       // platform
        test4 += EncodeString(51, "16813");     // user_id
        if (!TestFormat(test4, "Add platform field")) {
            std::cout << "❌ Platform field broke it." << std::endl;
            return;
        }
        
        std::cout << "\n🎯 COMPLETE WORKING FORMAT FOUND!" << std::endl;
        std::cout << "Use the last successful format in the main plugin." << std::endl;
    }
};

int main() {
    CompleteFormatTester tester;
    tester.TestProgressiveFormat();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 