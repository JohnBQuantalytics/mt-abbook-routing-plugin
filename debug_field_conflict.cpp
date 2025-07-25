//+------------------------------------------------------------------+
//| Debug Field Conflict - Find what's overwriting user_id         |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class FieldConflictDebugger {
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
    
    std::string EncodeInt64(int field_number, int64_t value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 0; // Wire type 0 for varint
        result += EncodeVarint(field_tag);
        result += EncodeVarint((uint64_t)value);
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
        std::cout << "Message size: " << protobuf_body.length() << " bytes" << std::endl;
        
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
                std::cout << "✅ SUCCESS: Received " << bytes_received << " bytes" << std::endl;
                success = true;
            } else if (bytes_received == 0) {
                std::cout << "⚠️ Connection closed by server" << std::endl;
            } else {
                std::cout << "❌ Receive timeout/error" << std::endl;
            }
            
            closesocket(sock);
            WSACleanup();            
        } catch (...) {
            if (sock != INVALID_SOCKET) closesocket(sock);
            WSACleanup();
        }
        
        return success;
    }
    
public:
    void DebugFieldConflicts() {
        std::cout << "Field Conflict Analysis - Finding what overwrites user_id\n";
        std::cout << "========================================================\n";
        
        // Test 1: Just user_id field 60 (current encoding)
        std::string test1 = EncodeString(60, "16813");
        TestMessage(test1, "Test 1: Field 60 (user_id) alone - current encoding");
        
        // Test 2: Fields that might use wire type 5 around field 60
        std::cout << "\n🔍 Checking fields that might conflict with field 60..." << std::endl;
        
        // Check if any of our float fields accidentally encode as field 60
        for (int test_field = 58; test_field <= 62; test_field++) {
            std::string test_msg = EncodeFloat(test_field, 12345.0f);  // Random float
            
            // Calculate what field number this creates
            uint32_t field_tag = (test_field << 3) | 5;
            std::cout << "Field " << test_field << " with wire type 5 = tag " << field_tag;
            
            // Check if this tag could be misinterpreted as field 60
            if ((field_tag >> 3) == 60) {
                std::cout << " ⚠️ CONFLICT! This creates field 60!" << std::endl;
            } else {
                std::cout << " (OK)" << std::endl;
            }
        }
        
        // Test 3: Core trade data + user_id (gradual build up)
        std::string test3;
        test3 += EncodeFloat(1, 0.59350f);      // open_price
        test3 += EncodeString(60, "16813");     // user_id
        TestMessage(test3, "Test 3: open_price + user_id");
        
        // Test 4: Add more fields one by one to find the culprit
        std::string test4;
        test4 += EncodeFloat(1, 0.59350f);      // open_price  
        test4 += EncodeFloat(2, 0.59000f);      // sl
        test4 += EncodeString(60, "16813");     // user_id
        TestMessage(test4, "Test 4: open_price + sl + user_id");
        
        std::string test5;
        test5 += EncodeFloat(1, 0.59350f);      // open_price
        test5 += EncodeFloat(2, 0.59000f);      // sl
        test5 += EncodeFloat(3, 0.59700f);      // tp
        test5 += EncodeString(60, "16813");     // user_id
        TestMessage(test5, "Test 5: open_price + sl + tp + user_id");
        
        std::string test6;
        test6 += EncodeFloat(1, 0.59350f);      // open_price
        test6 += EncodeFloat(2, 0.59000f);      // sl
        test6 += EncodeFloat(3, 0.59700f);      // tp
        test6 += EncodeInt64(4, 1);             // deal_type
        test6 += EncodeString(60, "16813");     // user_id
        TestMessage(test6, "Test 6: Core trade data + user_id");
        
        // Test 7: Add the problematic fields from our complete message
        std::string test7;
        test7 += EncodeFloat(1, 0.59350f);      // open_price
        test7 += EncodeFloat(2, 0.59000f);      // sl  
        test7 += EncodeFloat(3, 0.59700f);      // tp
        test7 += EncodeInt64(4, 1);             // deal_type
        test7 += EncodeFloat(5, 1.0f);          // lot_volume
        // ... skip to the end fields that might conflict
        test7 += EncodeString(58, "CY");        // country_code
        test7 += EncodeString(59, "cpc");       // utm_medium  
        test7 += EncodeString(60, "16813");     // user_id
        TestMessage(test7, "Test 7: Core data + end metadata + user_id");
        
        std::cout << "\n=== DIAGNOSIS ===" << std::endl;
        std::cout << "If any test above fails, we know which field combination breaks user_id encoding." << std::endl;
        std::cout << "The field that causes the issue encodes user_id with wrong wire type." << std::endl;
    }
};

int main() {
    FieldConflictDebugger debugger;
    debugger.DebugFieldConflicts();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 