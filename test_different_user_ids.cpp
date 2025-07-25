//+------------------------------------------------------------------+
//| Test Different User IDs - Is it ML or just a lookup table?    |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

class UserIdVariationTester {
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
    
    float TestUserId(const std::string& user_id) {
        std::string request = EncodeString(60, user_id);
        std::string full_message = CreateLengthPrefix(request);
        
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        float result = -1.0f;
        
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
                return -1.0f;
            }
            
            if (send(sock, full_message.c_str(), full_message.length(), 0) == SOCKET_ERROR) {
                return -1.0f;
            }
            
            char response[256];
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received >= 9) {
                // Extract float from field 2 (tag 0x15)
                const char* protobuf_data = response + 4;
                if ((unsigned char)protobuf_data[0] == 0x15) {
                    memcpy(&result, protobuf_data + 1, 4);
                }
            }
            
            closesocket(sock);
            WSACleanup();
            
        } catch (...) {
            if (sock != INVALID_SOCKET) closesocket(sock);
            WSACleanup();
        }
        
        return result;
    }
    
public:
    void TestUserIdVariations() {
        std::cout << "Testing Different User IDs - ML vs Lookup Table?\n";
        std::cout << "================================================\n\n";
        
        std::vector<std::string> test_user_ids = {
            "16813",      // Original
            "12345",      // Different number
            "99999",      // High number
            "1",          // Low number
            "0",          // Zero
            "abc123",     // Mixed alpha-numeric
            "user001",    // Text format
            "invalid",    // Clearly invalid
            "nonexistent" // Non-existent
        };
        
        std::cout << "Testing various user_ids to see if responses vary...\n\n";
        
        bool responses_vary = false;
        float first_valid_response = -1.0f;
        int valid_responses = 0;
        
        for (const auto& user_id : test_user_ids) {
            std::cout << "user_id: '" << user_id << "' ... ";
            
            float response = TestUserId(user_id);
            
            if (response >= 0.0f) {
                std::cout << "Response: " << response;
                
                if (first_valid_response < 0.0f) {
                    first_valid_response = response;
                } else if (abs(response - first_valid_response) > 0.000001f) {
                    responses_vary = true;
                    std::cout << " (DIFFERENT!)";
                }
                
                valid_responses++;
                std::cout << std::endl;
            } else {
                std::cout << "No response / Error" << std::endl;
            }
            
            Sleep(100); // Small delay between requests
        }
        
        std::cout << "\n=== ANALYSIS ===" << std::endl;
        std::cout << "Valid responses: " << valid_responses << "/" << test_user_ids.size() << std::endl;
        
        if (valid_responses == 0) {
            std::cout << "❌ No valid responses - service might be down or restricted" << std::endl;
        } else if (valid_responses == 1) {
            std::cout << "⚠️ Only one user_id works - might be allowlist/test mode" << std::endl;
        } else if (!responses_vary) {
            std::cout << "🚨 ALL RESPONSES IDENTICAL!" << std::endl;
            std::cout << "💡 This suggests:" << std::endl;
            std::cout << "   - NOT real ML (would vary based on user data)" << std::endl;
            std::cout << "   - Probably a DEFAULT/TEST response" << std::endl;
            std::cout << "   - Or service version/status code" << std::endl;
            std::cout << "   - Value: " << first_valid_response << " might be service metadata" << std::endl;
        } else {
            std::cout << "✅ Responses vary by user_id" << std::endl;
            std::cout << "💡 This suggests:" << std::endl;
            std::cout << "   - Lookup table with pre-computed scores" << std::endl;
            std::cout << "   - Historical user data analysis" << std::endl;
            std::cout << "   - Or real ML with cached user profiles" << std::endl;
        }
        
        std::cout << "\n🤔 USER'S POINT IS VALID:" << std::endl;
        std::cout << "Real trading ML should need trading data (price, volume, symbol)" << std::endl;
        std::cout << "Getting a score from just user_id suggests lookup table or test mode" << std::endl;
    }
};

int main() {
    UserIdVariationTester tester;
    tester.TestUserIdVariations();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 