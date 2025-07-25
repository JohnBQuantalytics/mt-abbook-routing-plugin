//+------------------------------------------------------------------+
//| Minimal User ID Only Test - Find the Exact Bug                 |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

class UserIdOnlyTester {
private:
    std::string ml_ip = "188.245.254.12";
    int ml_port = 50051;
    
    void LogWithTime(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);
        
        printf("[%02d:%02d:%02d] %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, message.c_str());
        fflush(stdout);
    }
    
    void PrintHex(const std::string& data, const std::string& label) {
        std::cout << label << ": ";
        for (size_t i = 0; i < data.length(); i++) {
            printf("%02X ", (unsigned char)data[i]);
        }
        std::cout << " (" << data.length() << " bytes)" << std::endl;
    }
    
    // EXACT copy of our encoding functions
    std::string EncodeVarint(uint64_t value) {
        std::string result;
        while (value >= 0x80) {
            result += (char)((value & 0x7F) | 0x80);
            value >>= 7;
        }
        result += (char)(value & 0x7F);
        return result;
    }
    
    std::string EncodeString(int field_number, const std::string& value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 2; // Wire type 2 for string
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
    
    void TestVarintEncoding() {
        LogWithTime("=== TESTING VARINT ENCODING ===");
        
        // Test field 51 calculation step by step
        uint32_t field_tag = (51 << 3) | 2;
        LogWithTime("Field 51, wire type 2 calculation: " + std::to_string(field_tag));
        
        std::string varint_result = EncodeVarint(field_tag);
        PrintHex(varint_result, "Field tag as varint");
        
        // Manual verification
        if (field_tag == 410) {
            LogWithTime("✅ Field tag calculation correct: 410");
        } else {
            LogWithTime("❌ Field tag calculation WRONG: " + std::to_string(field_tag));
        }
        
        // Expected: 410 = 0x19A
        // In varint: 0x9A 0x03 (since 410 = 154 + 256*1 = 154 + 256)
        if (varint_result.length() == 2 && 
            (unsigned char)varint_result[0] == 0x9A && 
            (unsigned char)varint_result[1] == 0x03) {
            LogWithTime("✅ Varint encoding correct");
        } else {
            LogWithTime("❌ Varint encoding WRONG");
        }
    }
    
public:
    void TestUserIdOnly() {
        LogWithTime("=== USER ID ONLY TEST ===");
        LogWithTime("Sending ONLY field 51 (user_id) to isolate the bug");
        LogWithTime("");
        
        TestVarintEncoding();
        
        // Create minimal message with ONLY user_id
        std::string user_id_field = EncodeString(51, "16813");
        PrintHex(user_id_field, "Field 51 (user_id) raw bytes");
        
        std::string full_message = CreateLengthPrefix(user_id_field);
        PrintHex(full_message, "Complete length-prefixed message");
        
        LogWithTime("Message breakdown:");
        LogWithTime("- Length prefix: " + std::to_string(user_id_field.length()) + " bytes");
        LogWithTime("- Field 51 tag: should be 0x9A 0x03");
        LogWithTime("- String length: should be 0x05");
        LogWithTime("- String data: should be '16813'");
        
        // Send to ML service
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        
        try {
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(ml_port);
            inet_pton(AF_INET, ml_ip.c_str(), &serverAddr.sin_addr);
            
            LogWithTime("Connecting to ML service...");
            if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                LogWithTime("❌ Connection failed");
                return;
            }
            LogWithTime("✅ Connected");
            
            LogWithTime("Sending minimal user_id-only message...");
            if (send(sock, full_message.c_str(), full_message.length(), 0) == SOCKET_ERROR) {
                LogWithTime("❌ Send failed");
                return;
            }
            LogWithTime("✅ Sent " + std::to_string(full_message.length()) + " bytes");
            
            LogWithTime("Waiting for response...");
            char response[256];
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received > 0) {
                LogWithTime("✅ Received " + std::to_string(bytes_received) + " bytes");
            } else if (bytes_received == 0) {
                LogWithTime("⚠️ Connection closed by server");
            } else {
                int error = WSAGetLastError();
                LogWithTime("❌ Receive failed (WSA error: " + std::to_string(error) + ")");
            }
            
            closesocket(sock);
            WSACleanup();
            
        } catch (...) {
            LogWithTime("❌ Exception occurred");
            if (sock != INVALID_SOCKET) closesocket(sock);
            WSACleanup();
        }
        
        LogWithTime("");
        LogWithTime("=== CHECK ML SERVICE LOGS FOR THIS TIMESTAMP ===");
        LogWithTime("If still getting ThirtyTwoBit error, the bug is in our varint encoding!");
    }
};

int main() {
    std::cout << "MT4 A/B-book Plugin - User ID Only Debug\n";
    std::cout << "========================================\n";
    std::cout << "Minimal test to isolate the wire type bug\n\n";
    
    UserIdOnlyTester tester;
    tester.TestUserIdOnly();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 