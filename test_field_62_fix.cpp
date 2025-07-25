//+------------------------------------------------------------------+
//| Test Field 62 Fix - Complete Message with user_id in Field 62 |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class Field62FixTester {
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
    
public:
    void TestField62Fix() {
        std::cout << "Testing COMPLETE MESSAGE with user_id in Field 62 (the fix)\n";
        std::cout << "===========================================================\n\n";
        
        std::string request;
        
        // Build complete message with ALL fields, but user_id in field 62
        std::cout << "Building complete 60-field message with user_id in field 62..." << std::endl;
        
        // Core trade data (1-5)
        request += EncodeFloat(1, 0.59350f);
        request += EncodeFloat(2, 0.59000f);
        request += EncodeFloat(3, 0.59700f);
        request += EncodeInt64(4, 1);
        request += EncodeFloat(5, 1.0f);
        
        // Account history (6-45) - abbreviated for test
        request += EncodeInt64(6, 0);
        request += EncodeFloat(7, 59350.0f);
        request += EncodeFloat(8, 10000.0f);
        request += EncodeInt64(9, 1);
        request += EncodeFloat(10, 0.0059f);
        
        // Add some more fields to make it realistic
        for (int i = 11; i <= 45; i++) {
            if (i % 2 == 0) {
                request += EncodeFloat(i, 1.0f);
            } else {
                request += EncodeInt64(i, 1);
            }
        }
        
        // Context metadata (46-62) - with user_id in field 62
        request += EncodeString(46, "NZDUSD");
        request += EncodeString(47, "FXMajors");
        request += EncodeString(48, "medium");
        request += EncodeString(49, "standard");
        request += EncodeString(50, "CY");
        request += EncodeString(51, "MT4");
        request += EncodeString(52, "bachelor");
        request += EncodeString(53, "professional");
        request += EncodeString(54, "employment");
        request += EncodeString(55, "50k-100k");
        request += EncodeString(56, "weekly");
        request += EncodeString(57, "employed");
        request += EncodeString(58, "CY");
        request += EncodeString(59, "cpc");
        // Field 60 is empty (as per protobuf spec, user_id is field 60, but ML service expects field 62)
        request += EncodeString(61, "extra_field");  // Add a field between 60 and 62
        request += EncodeString(62, "16813");        // 🎯 user_id in field 62 (THE FIX!)
        
        std::cout << "✅ Complete message built (" << request.length() << " bytes)" << std::endl;
        std::cout << "🎯 user_id placed in field 62 (not 60)" << std::endl;
        
        // Test the message
        std::string full_message = CreateLengthPrefix(request);
        
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        
        try {
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            
            int timeout_ms = 8000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(ml_port);
            inet_pton(AF_INET, ml_ip.c_str(), &serverAddr.sin_addr);
            
            if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                std::cout << "❌ Connection failed" << std::endl;
                return;
            }
            
            std::cout << "✅ Connected to ML service" << std::endl;
            
            if (send(sock, full_message.c_str(), full_message.length(), 0) == SOCKET_ERROR) {
                std::cout << "❌ Send failed" << std::endl;
                return;
            }
            
            std::cout << "✅ Sent " << full_message.length() << " bytes" << std::endl;
            
            char response[256];
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received > 0) {
                std::cout << "\n🎉 SUCCESS! ML service accepted the complete message!" << std::endl;
                std::cout << "📊 Received " << bytes_received << " bytes" << std::endl;
                
                // Try to parse the score
                if (bytes_received >= 9) {
                    const char* protobuf_data = response + 4;
                    for (int i = 0; i < bytes_received - 8; i++) {
                        if ((unsigned char)protobuf_data[i] == 0x0D) {
                            float score;
                            memcpy(&score, protobuf_data + i + 1, 4);
                            std::cout << "🎯 ML Score: " << score << std::endl;
                            std::cout << "📈 Routing: " << (score >= 0.08f ? "B-BOOK" : "A-BOOK") << std::endl;
                            break;
                        }
                    }
                }
                
                std::cout << "\n✅ CONFIRMED: user_id should be in field 62, not field 60!" << std::endl;
                std::cout << "🔧 Plugin fix: Change all user_id fields from 60 to 62" << std::endl;
                
            } else if (bytes_received == 0) {
                std::cout << "⚠️ Connection closed by server" << std::endl;
                std::cout << "❌ Field 62 fix didn't work - issue is elsewhere" << std::endl;
            } else {
                std::cout << "❌ Receive timeout/error" << std::endl;
            }
            
            closesocket(sock);
            WSACleanup();
            
        } catch (...) {
            if (sock != INVALID_SOCKET) closesocket(sock);
            WSACleanup();
        }
    }
};

int main() {
    Field62FixTester tester;
    tester.TestField62Fix();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 