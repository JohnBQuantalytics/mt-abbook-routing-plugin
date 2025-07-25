//+------------------------------------------------------------------+
//| Test Gradual Field Buildup - Find the Breaking Point          |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class GradualBuildupTester {
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
    
    std::string EncodeInt64(int field_number, int64_t value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 0;
        result += EncodeVarint(field_tag);
        result += EncodeVarint((uint64_t)value);
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
    
    bool TestMessage(const std::string& protobuf_body, const std::string& description) {
        std::cout << "Testing: " << description << " (" << protobuf_body.length() << " bytes)... ";
        
        std::string full_message = CreateLengthPrefix(protobuf_body);
        
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        bool success = false;
        
        try {
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            
            int timeout_ms = 3000;
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
                std::cout << "✅ SUCCESS (" << bytes_received << " bytes)" << std::endl;
                success = true;
            } else if (bytes_received == 0) {
                std::cout << "❌ Connection closed" << std::endl;
            } else {
                std::cout << "❌ Timeout/error" << std::endl;
            }
            
            closesocket(sock);
            WSACleanup();
            
        } catch (...) {
            std::cout << "❌ Exception" << std::endl;
            if (sock != INVALID_SOCKET) closesocket(sock);
            WSACleanup();
        }
        
        return success;
    }
    
public:
    void FindBreakingPoint() {
        std::cout << "Gradual Field Buildup Test - Finding the Breaking Point\n";
        std::cout << "=======================================================\n\n";
        
        std::string request;
        
        // Start with user_id only (we know this works)
        request = EncodeString(60, "16813");
        if (!TestMessage(request, "user_id only")) {
            std::cout << "❌ Even user_id alone fails - major issue!" << std::endl;
            return;
        }
        
        // Add core trade fields one by one
        request += EncodeFloat(1, 0.59350f);
        if (!TestMessage(request, "user_id + open_price")) {
            std::cout << "🚨 BREAKING POINT: Adding open_price breaks it!" << std::endl;
            return;
        }
        
        request += EncodeFloat(2, 0.59000f);
        if (!TestMessage(request, "user_id + open_price + sl")) {
            std::cout << "🚨 BREAKING POINT: Adding sl breaks it!" << std::endl;
            return;
        }
        
        request += EncodeFloat(3, 0.59700f);
        if (!TestMessage(request, "user_id + open_price + sl + tp")) {
            std::cout << "🚨 BREAKING POINT: Adding tp breaks it!" << std::endl;
            return;
        }
        
        request += EncodeInt64(4, 1);
        if (!TestMessage(request, "user_id + core trade data (1-4)")) {
            std::cout << "🚨 BREAKING POINT: Adding deal_type breaks it!" << std::endl;
            return;
        }
        
        request += EncodeFloat(5, 1.0f);
        if (!TestMessage(request, "user_id + core trade data (1-5)")) {
            std::cout << "🚨 BREAKING POINT: Adding lot_volume breaks it!" << std::endl;
            return;
        }
        
        // Add account data
        request += EncodeInt64(6, 0);
        if (!TestMessage(request, "user_id + core + is_bonus")) {
            std::cout << "🚨 BREAKING POINT: Adding is_bonus breaks it!" << std::endl;
            return;
        }
        
        request += EncodeFloat(7, 59350.0f);
        if (!TestMessage(request, "user_id + core + turnover")) {
            std::cout << "🚨 BREAKING POINT: Adding turnover breaks it!" << std::endl;
            return;
        }
        
        // Continue adding fields to find the limit
        for (int field = 8; field <= 20; field++) {
            if (field % 2 == 0) {
                request += EncodeFloat(field, 100.0f);
            } else {
                request += EncodeInt64(field, 10);
            }
            
            if (!TestMessage(request, "Fields 1-" + std::to_string(field) + " + user_id")) {
                std::cout << "🚨 BREAKING POINT: Adding field " << field << " breaks it!" << std::endl;
                return;
            }
        }
        
        std::cout << "\n🤔 Interesting: Message still works up to field 20..." << std::endl;
        std::cout << "🔍 Let's test message size limits..." << std::endl;
        
        // Test larger messages to find size limit
        int field_count = 21;
        while (field_count <= 60) {
            // Add 5 fields at a time
            for (int i = 0; i < 5 && field_count <= 60; i++, field_count++) {
                if (field_count % 2 == 0) {
                    request += EncodeFloat(field_count, 100.0f);
                } else {
                    request += EncodeInt64(field_count, 10);
                }
            }
            
            if (!TestMessage(request, "Fields 1-" + std::to_string(field_count-1) + " + user_id")) {
                std::cout << "🚨 BREAKING POINT: Message fails at ~" << (field_count-1) << " fields!" << std::endl;
                std::cout << "📏 Message size: " << request.length() << " bytes" << std::endl;
                return;
            }
        }
        
        std::cout << "\n🎉 All fields work! Issue must be with specific field values or combinations." << std::endl;
    }
};

int main() {
    GradualBuildupTester tester;
    tester.FindBreakingPoint();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 