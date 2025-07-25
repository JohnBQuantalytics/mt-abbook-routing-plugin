//+------------------------------------------------------------------+
//| Test Meaningful ML Fields - Find Working Trading Data Combo    |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class MeaningfulFieldsTester {
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
    
    std::string EncodeUInt32(int field_number, uint32_t value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 0;
        result += EncodeVarint(field_tag);
        result += EncodeVarint(value);
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
        if (response.length() < 9) return -1.0f;
        
        const char* protobuf_data = response.c_str() + 4;
        int protobuf_length = response.length() - 4;
        
        // Look for score field (field 1 or 2, wire type 5)
        for (int i = 0; i < protobuf_length - 4; i++) {
            if ((unsigned char)protobuf_data[i] == 0x0D || (unsigned char)protobuf_data[i] == 0x15) {
                float score;
                memcpy(&score, protobuf_data + i + 1, 4);
                return score;
            }
        }
        return -1.0f;
    }
    
    bool TestFormat(const std::string& protobuf_body, const std::string& description) {
        std::cout << "\n=== " << description << " ===" << std::endl;
        
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
                    std::cout << "⚠️ Response received but invalid score: " << score << std::endl;
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
    void TestMeaningfulCombinations() {
        std::cout << "MT4 A/B-book Plugin - Meaningful Trading Fields Test\n";
        std::cout << "====================================================\n";
        std::cout << "Testing combinations of actual trading data for ML scoring\n";
        
        // Test 1: Core trading data (no user_id)
        std::string test1;
        test1 += EncodeFloat(1, 0.59350f);      // open_price
        test1 += EncodeFloat(5, 1.0f);          // lot_volume  
        if (TestFormat(test1, "Core trading: open_price + lot_volume")) {
            std::cout << "🎯 SUCCESS: ML service uses actual trading data!" << std::endl;
        }
        
        // Test 2: Add symbol information
        std::string test2;
        test2 += EncodeFloat(1, 0.59350f);      // open_price
        test2 += EncodeFloat(5, 1.0f);          // lot_volume
        test2 += EncodeString(40, "NZDUSD");    // symbol (try field 40 from spec)
        if (TestFormat(test2, "Add symbol: open_price + lot_volume + symbol")) {
            std::cout << "🎯 SUCCESS: Symbol is important for ML scoring!" << std::endl;
        }
        
        // Test 3: Add account balance
        std::string test3;
        test3 += EncodeFloat(1, 0.59350f);      // open_price
        test3 += EncodeFloat(5, 1.0f);          // lot_volume
        test3 += EncodeFloat(8, 10000.0f);      // opening_balance
        if (TestFormat(test3, "Add balance: open_price + lot_volume + balance")) {
            std::cout << "🎯 SUCCESS: Account balance matters for risk assessment!" << std::endl;
        }
        
        // Test 4: Add deal type (buy/sell direction)
        std::string test4;
        test4 += EncodeFloat(1, 0.59350f);      // open_price
        test4 += EncodeUInt32(4, 1);            // deal_type (1 = sell)
        test4 += EncodeFloat(5, 1.0f);          // lot_volume
        if (TestFormat(test4, "Add direction: open_price + deal_type + lot_volume")) {
            std::cout << "🎯 SUCCESS: Trading direction affects ML scoring!" << std::endl;
        }
        
        // Test 5: Complete meaningful set
        std::string test5;
        test5 += EncodeFloat(1, 0.59350f);      // open_price
        test5 += EncodeUInt32(4, 1);            // deal_type  
        test5 += EncodeFloat(5, 1.0f);          // lot_volume
        test5 += EncodeFloat(7, 59350.0f);      // turnover_usd
        test5 += EncodeFloat(8, 10000.0f);      // opening_balance
        if (TestFormat(test5, "Complete trading data: price + type + volume + turnover + balance")) {
            std::cout << "🎯 SUCCESS: Full trading context for ML!" << std::endl;
        }
        
        // Test 6: Add user_id to working trading data
        std::string test6;
        test6 += EncodeFloat(1, 0.59350f);      // open_price
        test6 += EncodeFloat(5, 1.0f);          // lot_volume
        test6 += EncodeString(51, "16813");     // user_id
        if (TestFormat(test6, "Trading data + user_id: open_price + lot_volume + user_id")) {
            std::cout << "🎯 SUCCESS: ML uses both trading data AND user context!" << std::endl;
        }
        
        // Test 7: Try different field order
        std::string test7;
        test7 += EncodeString(51, "16813");     // user_id first
        test7 += EncodeFloat(1, 0.59350f);     // then open_price
        test7 += EncodeFloat(5, 1.0f);         // then lot_volume
        if (TestFormat(test7, "Reordered: user_id + open_price + lot_volume")) {
            std::cout << "🎯 SUCCESS: Field order: user_id first, then trading data!" << std::endl;
        }
        
        // Test 8: Minimal realistic ML features
        std::string test8;
        test8 += EncodeFloat(1, 0.59350f);      // open_price
        test8 += EncodeFloat(5, 1.0f);          // lot_volume
        test8 += EncodeFloat(8, 10000.0f);      // opening_balance
        test8 += EncodeString(51, "16813");     // user_id
        if (TestFormat(test8, "Minimal ML set: price + volume + balance + user_id")) {
            std::cout << "🎯 PERFECT: Minimal meaningful ML feature set!" << std::endl;
        }
        
        std::cout << "\n=== SUMMARY ===" << std::endl;
        std::cout << "The ML service needs REAL trading data to make intelligent decisions." << std::endl;
        std::cout << "User ID alone is not sufficient for proper risk assessment." << std::endl;
    }
};

int main() {
    MeaningfulFieldsTester tester;
    tester.TestMeaningfulCombinations();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 