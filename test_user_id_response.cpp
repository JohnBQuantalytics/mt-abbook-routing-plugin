//+------------------------------------------------------------------+
//| Test user_id Response - Show ML Service Output                 |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

class UserIdResponseTester {
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
    
    void PrintHexDump(const char* data, int length, const std::string& label) {
        std::cout << "\n=== " << label << " ===" << std::endl;
        std::cout << "Length: " << length << " bytes" << std::endl;
        std::cout << "Hex dump: ";
        for (int i = 0; i < length; i++) {
            printf("%02X ", (unsigned char)data[i]);
            if ((i + 1) % 16 == 0) {
                std::cout << std::endl << "          ";
            }
        }
        std::cout << std::endl;
        
        std::cout << "ASCII:    ";
        for (int i = 0; i < length; i++) {
            char c = data[i];
            if (c >= 32 && c <= 126) {
                std::cout << c << "  ";
            } else {
                std::cout << ".  ";
            }
            if ((i + 1) % 16 == 0) {
                std::cout << std::endl << "          ";
            }
        }
        std::cout << std::endl;
    }
    
    float ParseProtobufScore(const char* data, int length) {
        std::cout << "\n=== PARSING PROTOBUF RESPONSE ===" << std::endl;
        
        if (length < 9) {
            std::cout << "❌ Response too short for protobuf format" << std::endl;
            return -1.0f;
        }
        
        // First 4 bytes should be length prefix
        uint32_t protobuf_length = ((unsigned char)data[0] << 24) |
                                   ((unsigned char)data[1] << 16) |
                                   ((unsigned char)data[2] << 8) |
                                   ((unsigned char)data[3]);
        
        std::cout << "Length prefix: " << protobuf_length << " bytes" << std::endl;
        
        if (protobuf_length != length - 4) {
            std::cout << "⚠️ Length mismatch: expected " << protobuf_length << ", got " << (length - 4) << std::endl;
        }
        
        // Parse the protobuf data (after length prefix)
        const char* protobuf_data = data + 4;
        int protobuf_actual_length = length - 4;
        
        std::cout << "Protobuf data (" << protobuf_actual_length << " bytes): ";
        for (int i = 0; i < protobuf_actual_length; i++) {
            printf("%02X ", (unsigned char)protobuf_data[i]);
        }
        std::cout << std::endl;
        
        // Look for score field (field 1, wire type 5 = fixed32 float)
        for (int i = 0; i < protobuf_actual_length - 4; i++) {
            unsigned char field_tag = (unsigned char)protobuf_data[i];
            
            std::cout << "Byte " << i << ": 0x" << std::hex << (int)field_tag << std::dec;
            
            if (field_tag == 0x0D) {
                std::cout << " → Field 1, wire type 5 (float) - SCORE FOUND!" << std::endl;
                
                // Next 4 bytes are the float value
                float score;
                memcpy(&score, protobuf_data + i + 1, 4);
                
                std::cout << "Raw float bytes: ";
                for (int j = 1; j <= 4; j++) {
                    printf("%02X ", (unsigned char)protobuf_data[i + j]);
                }
                std::cout << std::endl;
                
                std::cout << "🎯 DECODED SCORE: " << std::fixed << std::setprecision(6) << score << std::endl;
                std::cout << "📊 Score range: " << (score >= 0.0f && score <= 1.0f ? "VALID (0.0-1.0)" : "INVALID") << std::endl;
                std::cout << "🎲 Routing decision: " << (score >= 0.08f ? "B-BOOK (risky)" : "A-BOOK (safe)") << std::endl;
                
                return score;
            } else {
                std::cout << " → Field " << (field_tag >> 3) << ", wire type " << (field_tag & 0x7) << std::endl;
            }
        }
        
        std::cout << "❌ No score field found in protobuf data" << std::endl;
        return -1.0f;
    }
    
public:
    void TestUserIdResponse() {
        std::cout << "ML Service Response Analysis - user_id Only\n";
        std::cout << "===========================================\n\n";
        std::cout << "Sending user_id='16813' to ML service and analyzing full response...\n";
        
        // Create request with only user_id
        std::string request = EncodeString(60, "16813");  // Use field 60 as per protobuf spec
        std::string full_message = CreateLengthPrefix(request);
        
        PrintHexDump(full_message.c_str(), full_message.length(), "REQUEST SENT");
        
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
            
            std::cout << "\nConnecting to ML service " << ml_ip << ":" << ml_port << "..." << std::endl;
            
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
                std::cout << "\n🎉 SUCCESS: Received " << bytes_received << " bytes from ML service!" << std::endl;
                
                PrintHexDump(response, bytes_received, "ML SERVICE RESPONSE");
                
                float score = ParseProtobufScore(response, bytes_received);
                
                if (score >= 0.0f && score <= 1.0f) {
                    std::cout << "\n=== FINAL ANALYSIS ===" << std::endl;
                    std::cout << "✅ Valid ML score received: " << std::fixed << std::setprecision(6) << score << std::endl;
                    std::cout << "🎯 Trading decision: Route to " << (score >= 0.08f ? "B-BOOK" : "A-BOOK") << std::endl;
                    std::cout << "💡 ML service is working correctly with user_id only" << std::endl;
                    
                    if (score < 0.01f) {
                        std::cout << "⚠️ Very low score - user likely to be profitable (A-book recommended)" << std::endl;
                    } else if (score > 0.5f) {
                        std::cout << "⚠️ High score - user likely to lose money (good for B-book)" << std::endl;
                    } else {
                        std::cout << "ℹ️ Moderate score - standard risk profile" << std::endl;
                    }
                } else {
                    std::cout << "\n❌ Invalid or no score found in response" << std::endl;
                }
                
            } else if (bytes_received == 0) {
                std::cout << "⚠️ Connection closed by server - no response received" << std::endl;
            } else {
                int error = WSAGetLastError();
                std::cout << "❌ Receive error: " << error << std::endl;
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
    UserIdResponseTester tester;
    tester.TestUserIdResponse();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 