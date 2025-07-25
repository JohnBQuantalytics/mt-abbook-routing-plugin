//+------------------------------------------------------------------+
//| Test Field 62 Verification - Does it actually work?           |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class Field62Verifier {
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
    
    bool TestFieldNumber(int field_number, const std::string& description) {
        std::cout << "\n=== Testing " << description << " ===" << std::endl;
        
        std::string request = EncodeString(field_number, "16813");
        std::string full_message = CreateLengthPrefix(request);
        
        std::cout << "Field " << field_number << " encoded message size: " << full_message.length() << " bytes" << std::endl;
        
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
                
                // Try to parse score
                if (bytes_received >= 9) {
                    const char* protobuf_data = response + 4;
                    for (int i = 0; i < bytes_received - 8; i++) {
                        if ((unsigned char)protobuf_data[i] == 0x0D) {
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
            if (sock != INVALID_SOCKET) closesocket(sock);
            WSACleanup();
        }
        
        return success;
    }
    
public:
    void VerifyField62() {
        std::cout << "Field 62 Verification Test\n";
        std::cout << "==========================\n";
        std::cout << "Testing if field 62 actually works for user_id...\n";
        
        // Test field 60 (original - should fail)
        bool field60_works = TestFieldNumber(60, "Field 60 (original - expect failure)");
        
        // Test field 62 (our fix - should work)
        bool field62_works = TestFieldNumber(62, "Field 62 (our fix - expect success)");
        
        // Test a few nearby fields to see what actually works
        bool field61_works = TestFieldNumber(61, "Field 61 (testing nearby)");
        bool field63_works = TestFieldNumber(63, "Field 63 (testing nearby)");
        
        std::cout << "\n=== RESULTS ===" << std::endl;
        std::cout << "Field 60: " << (field60_works ? "✅ WORKS" : "❌ FAILS") << std::endl;
        std::cout << "Field 61: " << (field61_works ? "✅ WORKS" : "❌ FAILS") << std::endl;
        std::cout << "Field 62: " << (field62_works ? "✅ WORKS" : "❌ FAILS") << std::endl;
        std::cout << "Field 63: " << (field63_works ? "✅ WORKS" : "❌ FAILS") << std::endl;
        
        if (field62_works) {
            std::cout << "\n🎉 CONFIRMED: Field 62 fix works!" << std::endl;
            std::cout << "✅ Plugin should work with complete message now" << std::endl;
        } else {
            std::cout << "\n❌ Field 62 fix didn't work" << std::endl;
            if (field61_works) {
                std::cout << "🔧 Try field 61 instead" << std::endl;
            } else if (field63_works) {
                std::cout << "🔧 Try field 63 instead" << std::endl;
            } else {
                std::cout << "🤔 None of the nearby fields work - issue is elsewhere" << std::endl;
            }
        }
    }
};

int main() {
    Field62Verifier verifier;
    verifier.VerifyField62();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 