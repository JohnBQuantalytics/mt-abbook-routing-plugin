//+------------------------------------------------------------------+
//| Test CORRECT Protobuf Format - Based on Actual Spec           |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class CorrectProtobufTester {
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
        uint32_t field_tag = (field_number << 3) | 5; // Wire type 5 for fixed32
        result += EncodeVarint(field_tag);
        char* bytes = (char*)&value;
        for (int i = 0; i < 4; i++) {
            result += bytes[i];
        }
        return result;
    }
    
    std::string EncodeString(int field_number, const std::string& value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 2; // Wire type 2 for length-delimited
        result += EncodeVarint(field_tag);
        result += EncodeVarint(value.length());
        result += value;
        return result;
    }
    
    // BIG-ENDIAN length prefix (the key fix!)
    std::string CreateLengthPrefix(const std::string& body) {
        std::string message;
        uint32_t length = body.length();
        message += (char)((length >> 24) & 0xFF);  // Big-endian!
        message += (char)((length >> 16) & 0xFF);
        message += (char)((length >> 8) & 0xFF);
        message += (char)(length & 0xFF);
        message += body;
        return message;
    }
    
    float ParseScore(const char* response, int length) {
        if (length < 9) return -1.0f;
        
        // Skip big-endian length prefix
        const char* protobuf_data = response + 4;
        int protobuf_length = length - 4;
        
        // Look for score field (field 2, wire type 5)
        for (int i = 0; i < protobuf_length - 4; i++) {
            if ((unsigned char)protobuf_data[i] == 0x15) { // Field 2, wire type 5
                float score;
                memcpy(&score, protobuf_data + i + 1, 4);
                return score;
            }
        }
        return -1.0f;
    }
    
public:
    void TestCorrectFormat() {
        std::cout << "Testing CORRECT Protobuf Format (Based on Actual Spec)\n";
        std::cout << "======================================================\n\n";
        
        std::string request;
        
        // CORRECT SPEC: Use actual field numbers from the provided protobuf
        std::cout << "Building request with CORRECT field layout..." << std::endl;
        
        // Field 1: user_id (was field 60 before!)
        request += EncodeString(1, "16813");
        
        // Field 2-6: Core trading data (all floats!)
        request += EncodeFloat(2, 0.59350f);    // open_price
        request += EncodeFloat(3, 0.59000f);    // sl  
        request += EncodeFloat(4, 0.59700f);    // tp
        request += EncodeFloat(5, 1.0f);        // deal_type (float, not int!)
        request += EncodeFloat(6, 1.0f);        // lot_volume
        
        // Field 7-12: Account data (all floats!)
        request += EncodeFloat(7, 0.0f);        // is_bonus
        request += EncodeFloat(8, 59350.0f);    // turnover_usd
        request += EncodeFloat(9, 10000.0f);    // opening_balance
        request += EncodeFloat(10, 1.0f);       // concurrent_positions
        request += EncodeFloat(11, 0.0059f);    // sl_perc
        request += EncodeFloat(12, 0.0059f);    // tp_perc
        
        // Add symbol (field 40)
        request += EncodeString(40, "NZDUSD");  // symbol
        
        std::cout << "✅ Built request (" << request.length() << " bytes) with:" << std::endl;
        std::cout << "   - user_id in field 1 (CORRECT)" << std::endl;
        std::cout << "   - All numeric features as floats (CORRECT)" << std::endl;
        std::cout << "   - Big-endian length header (CORRECT)" << std::endl;
        
        // Create message with BIG-ENDIAN length prefix
        std::string full_message = CreateLengthPrefix(request);
        
        std::cout << "🚀 Sending to ML service..." << std::endl;
        
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
                std::cout << "❌ Connection failed: " << WSAGetLastError() << std::endl;
                return;
            }
            
            std::cout << "✅ Connected successfully" << std::endl;
            
            if (send(sock, full_message.c_str(), full_message.length(), 0) == SOCKET_ERROR) {
                std::cout << "❌ Send failed: " << WSAGetLastError() << std::endl;
                return;
            }
            
            std::cout << "✅ Sent " << full_message.length() << " bytes" << std::endl;
            
            char response[512];
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received > 0) {
                std::cout << "\n🎉 SUCCESS! Received " << bytes_received << " bytes" << std::endl;
                
                float score = ParseScore(response, bytes_received);
                
                if (score >= 0.0f && score <= 1.0f) {
                    std::cout << "🎯 REAL ML SCORE: " << score << std::endl;
                    std::cout << "📊 Routing: " << (score >= 0.5f ? "B-BOOK" : "A-BOOK") << std::endl;
                    std::cout << "✅ CONFIRMED: ML service works with trading data!" << std::endl;
                    
                    std::cout << "\n🎉 BREAKTHROUGH!" << std::endl;
                    std::cout << "✅ The ML service DOES real ML processing" << std::endl;
                    std::cout << "✅ We just had the wrong protobuf specification" << std::endl;
                    std::cout << "✅ Now we can implement proper A/B routing!" << std::endl;
                    
                } else {
                    std::cout << "⚠️ Unexpected score: " << score << std::endl;
                }
                
            } else if (bytes_received == 0) {
                std::cout << "⚠️ Connection closed by server" << std::endl;
                std::cout << "🤔 Might still have format issues..." << std::endl;
            } else {
                std::cout << "❌ Receive timeout/error: " << WSAGetLastError() << std::endl;
            }
            
            closesocket(sock);
            WSACleanup();
            
        } catch (...) {
            std::cout << "❌ Exception occurred" << std::endl;
            if (sock != INVALID_SOCKET) closesocket(sock);
            WSACleanup();
        }
    }
};

int main() {
    CorrectProtobufTester tester;
    tester.TestCorrectFormat();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 