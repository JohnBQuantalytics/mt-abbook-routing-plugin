//+------------------------------------------------------------------+
//| Debug Exact Field 60 Conflict - Copy Our Complete Message     |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class ExactConflictDebugger {
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
        
        // Debug print for field 60 conflicts
        if (field_number == 60 || field_tag == 485) {
            std::cout << "🚨 CONFLICT DETECTED: EncodeFloat(" << field_number << ", " << value << ")" << std::endl;
            std::cout << "    Field tag = " << field_tag << " (should create field " << (field_tag >> 3) << " wire type " << (field_tag & 0x7) << ")" << std::endl;
        }
        
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
        
        // Debug print for field 60
        if (field_number == 60) {
            std::cout << "✅ CORRECT: EncodeString(" << field_number << ", '" << value << "')" << std::endl;
            std::cout << "    Field tag = " << field_tag << " (creates field " << (field_tag >> 3) << " wire type " << (field_tag & 0x7) << ")" << std::endl;
        }
        
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
    void TestExactMessage() {
        std::cout << "Testing exact copy of our complete 60-field message...\n";
        std::cout << "Looking for field 60 conflicts...\n\n";
        
        std::string request;
        
        // Copy the EXACT same message from our plugin
        std::cout << "=== BUILDING MESSAGE (watching for field 60 conflicts) ===" << std::endl;
        
        // === CORE TRADE DATA (Fields 1-5) ===
        request += EncodeFloat(1, 0.59350f);
        request += EncodeFloat(2, 0.59000f);
        request += EncodeFloat(3, 0.59700f);
        request += EncodeInt64(4, 1);
        request += EncodeFloat(5, 1.0f);
        
        // === ACCOUNT & TRADING HISTORY (Fields 6-36) ===
        request += EncodeInt64(6, 0);
        
        float turnover = 59350.0f;
        request += EncodeFloat(7, turnover);
        request += EncodeFloat(8, 10000.0f);
        request += EncodeInt64(9, 1);
        
        float sl_perc = 0.0059f;
        float tp_perc = 0.0059f;
        request += EncodeFloat(10, sl_perc);
        request += EncodeFloat(11, tp_perc);
        request += EncodeInt64(12, 1);
        request += EncodeInt64(13, 1);
        
        request += EncodeFloat(14, 0.6f);
        request += EncodeInt64(15, 3);
        request += EncodeInt64(16, 50);
        request += EncodeInt64(17, 35);
        request += EncodeInt64(18, 90);
        request += EncodeFloat(19, 15000.0f);
        request += EncodeInt64(20, 5);
        request += EncodeFloat(21, 2000.0f);
        request += EncodeInt64(22, 2);
        request += EncodeInt64(23, 0);
        request += EncodeInt64(24, 3600);
        request += EncodeFloat(25, 100000.0f);
        request += EncodeFloat(26, -500.0f);
        request += EncodeFloat(27, 800.0f);
        request += EncodeFloat(28, 5.0f);
        request += EncodeFloat(29, 90.0f);
        request += EncodeFloat(30, 7.5f);
        request += EncodeInt64(31, 1);
        request += EncodeInt64(32, 1);
        request += EncodeFloat(33, 0.59f);
        request += EncodeFloat(34, 0.055f);
        request += EncodeFloat(35, 0.022f);
        request += EncodeFloat(36, 1187.0f);
        
        // === RECENT PERFORMANCE METRICS (Fields 37-45) ===
        request += EncodeFloat(37, 0.65f);
        request += EncodeFloat(38, 0.58f);
        request += EncodeFloat(39, 0.62f);
        request += EncodeInt64(40, 8);
        request += EncodeInt64(41, 15);
        request += EncodeInt64(42, 22);
        request += EncodeFloat(43, 45.0f);
        request += EncodeFloat(44, 38.0f);
        request += EncodeFloat(45, 41.0f);
        
        // === CONTEXT & METADATA (Fields 46-60) ===
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
        request += EncodeString(60, "16813");  // This should be the ONLY field 60!
        
        std::cout << "\n=== MESSAGE BUILT (" << request.length() << " bytes) ===" << std::endl;
        std::cout << "If no conflicts were detected above, the issue is elsewhere." << std::endl;
        
        // Test the message
        std::string full_message = CreateLengthPrefix(request);
        
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        
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
                return;
            }
            
            if (send(sock, full_message.c_str(), full_message.length(), 0) == SOCKET_ERROR) {
                std::cout << "❌ Send failed" << std::endl;
                return;
            }
            
            std::cout << "✅ Sent " << full_message.length() << " bytes" << std::endl;
            
            char response[256];
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received > 0) {
                std::cout << "✅ SUCCESS: Received " << bytes_received << " bytes" << std::endl;
                std::cout << "🎉 Complete message works! Issue must be in our plugin logic." << std::endl;
            } else if (bytes_received == 0) {
                std::cout << "⚠️ Connection closed by server" << std::endl;
                std::cout << "🔍 Field 60 conflict confirmed - check debug output above." << std::endl;
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
    ExactConflictDebugger debugger;
    debugger.TestExactMessage();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 