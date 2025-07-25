//+------------------------------------------------------------------+
//| Debug Field 60 (user_id) Encoding Issue                        |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <bitset>

#pragma comment(lib, "ws2_32.lib")

class Field60Debugger {
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
    
    // Method 1: Current implementation
    std::string EncodeString_Current(int field_number, const std::string& value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 2; // Wire type 2 for length-delimited
        result += EncodeVarint(field_tag);
        result += EncodeVarint(value.length());
        result += value;
        return result;
    }
    
    // Method 2: Explicit byte-by-byte for field 60
    std::string EncodeString_Manual(const std::string& value) {
        std::string result;
        
        // Field 60, wire type 2: (60 << 3) | 2 = 482
        // 482 in varint: 11110010 00000011 = 0xF2 0x03
        result += (char)0xF2; // First byte of varint 482
        result += (char)0x03; // Second byte of varint 482
        
        // Length of string as varint
        result += EncodeVarint(value.length());
        
        // String data
        result += value;
        
        return result;
    }
    
    void PrintHex(const std::string& data, const std::string& label) {
        std::cout << label << " (" << data.length() << " bytes): ";
        for (size_t i = 0; i < data.length(); i++) {
            printf("%02X ", (unsigned char)data[i]);
        }
        std::cout << std::endl;
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
        std::cout << "\n=== " << description << " ===" << std::endl;
        
        PrintHex(protobuf_body, "Protobuf body");
        
        std::string full_message = CreateLengthPrefix(protobuf_body);
        PrintHex(full_message, "Full message");
        
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
            
            std::cout << "✅ Sent successfully" << std::endl;
            
            char response[256];
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received > 0) {
                std::cout << "✅ SUCCESS: Received " << bytes_received << " bytes" << std::endl;
                
                // Try to parse score
                if (bytes_received >= 9) {
                    const char* protobuf_data = response + 4;
                    for (int i = 0; i < bytes_received - 8; i++) {
                        if ((unsigned char)protobuf_data[i] == 0x0D) { // Field 1, wire type 5
                            float score;
                            memcpy(&score, protobuf_data + i + 1, 4);
                            std::cout << "📊 Score: " << score << std::endl;
                            break;
                        }
                    }
                }
                success = true;
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
    void DebugField60Encoding() {
        std::cout << "Field 60 (user_id) Encoding Debug\n";
        std::cout << "=================================\n\n";
        
        std::string user_id = "16813";
        
        // Test 1: Current EncodeString method
        std::string test1 = EncodeString_Current(60, user_id);
        TestMessage(test1, "Test 1: Current EncodeString method");
        
        // Test 2: Manual encoding with explicit varint
        std::string test2 = EncodeString_Manual(user_id);
        TestMessage(test2, "Test 2: Manual varint encoding");
        
        // Test 3: Check field tag calculation
        std::cout << "\n=== Field Tag Analysis ===" << std::endl;
        uint32_t field_tag = (60 << 3) | 2;
        std::cout << "Field 60, wire type 2 calculation:" << std::endl;
        std::cout << "  (60 << 3) | 2 = " << field_tag << std::endl;
        std::cout << "  In binary: " << std::bitset<32>(field_tag) << std::endl;
        std::cout << "  In hex: 0x" << std::hex << field_tag << std::dec << std::endl;
        
        std::string varint_encoded = EncodeVarint(field_tag);
        PrintHex(varint_encoded, "Varint encoded field tag");
        
        // Test 4: Compare with working single-field version
        std::cout << "\n=== Comparison with known working version ===" << std::endl;
        
        // This was the version that worked in our previous tests
        std::string working_version;
        working_version += (char)0xF2; // Field tag byte 1
        working_version += (char)0x03; // Field tag byte 2  
        working_version += (char)0x05; // Length = 5
        working_version += "16813";    // String data
        
        PrintHex(working_version, "Previous working version");
        TestMessage(working_version, "Test 4: Known working single-field encoding");
        
        std::cout << "\n=== ANALYSIS ===" << std::endl;
        std::cout << "Comparing current vs working encoding to identify the issue..." << std::endl;
    }
};

int main() {
    Field60Debugger debugger;
    debugger.DebugField60Encoding();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 