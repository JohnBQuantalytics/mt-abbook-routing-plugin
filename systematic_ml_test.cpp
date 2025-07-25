//+------------------------------------------------------------------+
//| Systematic ML Service Test - Try Multiple Formats Until Success|
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

class SystematicMLTester {
private:
    std::string ml_ip = "188.245.254.12";
    int ml_port = 50051;
    int test_number = 1;
    
    void LogTest(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);
        
        printf("\n[TEST %d - %02d:%02d:%02d] %s\n", test_number, tm.tm_hour, tm.tm_min, tm.tm_sec, message.c_str());
        fflush(stdout);
    }
    
    void PrintHex(const std::string& data, const std::string& label) {
        std::cout << label << ": ";
        for (size_t i = 0; i < data.length() && i < 32; i++) {
            printf("%02X ", (unsigned char)data[i]);
        }
        if (data.length() > 32) std::cout << "...";
        std::cout << " (" << data.length() << " bytes)" << std::endl;
    }
    
    // Basic protobuf encoding functions
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
    
    // Alternative encoding - try single byte field tags (old method)
    std::string EncodeString_SingleByte(int field_number, const std::string& value) {
        std::string result;
        result += (char)((field_number << 3) | 2); // Single byte encoding
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
    
    bool SendAndReceive(const std::string& message, const std::string& description) {
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        bool success = false;
        
        try {
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            
            // Set short timeout to speed up tests
            int timeout_ms = 3000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(ml_port);
            inet_pton(AF_INET, ml_ip.c_str(), &serverAddr.sin_addr);
            
            if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                std::cout << "❌ Connection failed" << std::endl;
                return false;
            }
            
            if (send(sock, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
                std::cout << "❌ Send failed" << std::endl;
                return false;
            }
            
            char response[1024];
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received > 0) {
                std::cout << "✅ SUCCESS: Received " << bytes_received << " bytes!" << std::endl;
                
                // Try to parse as simple text first
                response[bytes_received] = '\0';
                bool has_printable = false;
                for (int i = 0; i < bytes_received; i++) {
                    if (response[i] >= 32 && response[i] <= 126) {
                        has_printable = true;
                        break;
                    }
                }
                
                if (has_printable) {
                    std::cout << "Response text: " << response << std::endl;
                }
                
                PrintHex(std::string(response, bytes_received), "Response hex");
                success = true;
                
            } else if (bytes_received == 0) {
                std::cout << "⚠️ Connection closed by server (no error - might be working!)" << std::endl;
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
    void RunAllTests() {
        std::cout << "MT4 A/B-book Plugin - Systematic ML Service Test\n";
        std::cout << "================================================\n";
        std::cout << "Testing multiple formats until we find one that works!\n";
        
        // Test 1: Minimal user_id only (field 51)
        test_number = 1;
        LogTest("Minimal user_id only (field 51)");
        std::string test1 = EncodeString(51, "16813");
        PrintHex(test1, "Protobuf body");
        std::string msg1 = CreateLengthPrefix(test1);
        if (SendAndReceive(msg1, "user_id field 51")) {
            std::cout << "🎯 TEST 1 WORKED! Use this format." << std::endl;
            return;
        }
        
        // Test 2: Try user_id as field 1 instead of 51
        test_number = 2;
        LogTest("user_id as field 1 (simple field number)");
        std::string test2 = EncodeString(1, "16813");
        PrintHex(test2, "Protobuf body");
        std::string msg2 = CreateLengthPrefix(test2);
        if (SendAndReceive(msg2, "user_id field 1")) {
            std::cout << "🎯 TEST 2 WORKED! Field number issue - use field 1 for user_id." << std::endl;
            return;
        }
        
        // Test 3: Try old single-byte encoding for field 51
        test_number = 3;
        LogTest("user_id field 51 with single-byte tag encoding");
        std::string test3 = EncodeString_SingleByte(51, "16813"); // This will fail for field > 15, but let's test
        PrintHex(test3, "Protobuf body");
        std::string msg3 = CreateLengthPrefix(test3);
        SendAndReceive(msg3, "user_id single-byte encoding");
        
        // Test 4: Try numeric user_id (as float)
        test_number = 4;
        LogTest("user_id as float instead of string");
        std::string test4 = EncodeFloat(51, 16813.0f);
        PrintHex(test4, "Protobuf body");
        std::string msg4 = CreateLengthPrefix(test4);
        if (SendAndReceive(msg4, "user_id as float")) {
            std::cout << "🎯 TEST 4 WORKED! Server expects user_id as float, not string!" << std::endl;
            return;
        }
        
        // Test 5: Minimal required fields (open_price + user_id)
        test_number = 5;
        LogTest("Minimal: open_price (field 1) + user_id (field 51)");
        std::string test5;
        test5 += EncodeFloat(1, 0.59350f);      // open_price
        test5 += EncodeString(51, "16813");     // user_id  
        PrintHex(test5, "Protobuf body");
        std::string msg5 = CreateLengthPrefix(test5);
        if (SendAndReceive(msg5, "open_price + user_id")) {
            std::cout << "🎯 TEST 5 WORKED! Use minimal fields only." << std::endl;
            return;
        }
        
        // Test 6: Try different field order (user_id first, then open_price)
        test_number = 6;
        LogTest("Reversed order: user_id (field 51) + open_price (field 1)");
        std::string test6;
        test6 += EncodeString(51, "16813");     // user_id first
        test6 += EncodeFloat(1, 0.59350f);     // open_price second
        PrintHex(test6, "Protobuf body");
        std::string msg6 = CreateLengthPrefix(test6);
        if (SendAndReceive(msg6, "reversed field order")) {
            std::cout << "🎯 TEST 6 WORKED! Field order matters - user_id must come first." << std::endl;
            return;
        }
        
        // Test 7: Try with user_id as different field numbers
        test_number = 7;
        LogTest("user_id as field 2, 3, 4, 5...");
        for (int field_num = 2; field_num <= 10; field_num++) {
            std::string test = EncodeString(field_num, "16813");
            std::string msg = CreateLengthPrefix(test);
            std::cout << "Trying field " << field_num << "... ";
            if (SendAndReceive(msg, "user_id field " + std::to_string(field_num))) {
                std::cout << "🎯 TEST 7 WORKED! user_id should be field " << field_num << std::endl;
                return;
            }
            Sleep(100); // Small delay between attempts
        }
        
        // Test 8: Raw text approach (maybe it's not protobuf at all?)
        test_number = 8;
        LogTest("Raw text instead of protobuf");
        std::string test8 = "{\"user_id\":\"16813\",\"open_price\":0.59350}";
        std::string msg8 = CreateLengthPrefix(test8);
        if (SendAndReceive(msg8, "JSON format")) {
            std::cout << "🎯 TEST 8 WORKED! Server expects JSON, not protobuf!" << std::endl;
            return;
        }
        
        // Test 9: Try without length prefix (direct protobuf)
        test_number = 9;
        LogTest("Direct protobuf without length prefix");
        std::string test9 = EncodeString(51, "16813");
        if (SendAndReceive(test9, "no length prefix")) {
            std::cout << "🎯 TEST 9 WORKED! Don't use length prefix!" << std::endl;
            return;
        }
        
        // Test 10: Full message but with corrected field numbers
        test_number = 10;
        LogTest("Full message with sequential field numbers");
        std::string test10;
        test10 += EncodeFloat(1, 0.59350f);         // open_price
        test10 += EncodeFloat(2, 0.59000f);         // sl
        test10 += EncodeFloat(3, 0.59700f);         // tp
        test10 += EncodeString(4, "16813");         // user_id as field 4
        PrintHex(test10, "Protobuf body");
        std::string msg10 = CreateLengthPrefix(test10);
        if (SendAndReceive(msg10, "sequential fields")) {
            std::cout << "🎯 TEST 10 WORKED! Use sequential field numbers!" << std::endl;
            return;
        }
        
        std::cout << "\n❌ ALL TESTS FAILED" << std::endl;
        std::cout << "The ML service may need:" << std::endl;
        std::cout << "- TLS/SSL encryption" << std::endl;
        std::cout << "- Authentication tokens" << std::endl;
        std::cout << "- Different protocol entirely" << std::endl;
    }
};

int main() {
    SystematicMLTester tester;
    tester.RunAllTests();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 