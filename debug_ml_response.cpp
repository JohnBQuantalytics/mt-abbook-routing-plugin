//+------------------------------------------------------------------+
//| Debug ML Response - Analyze Exact Response Format             |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

class MLResponseDebugger {
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
    
    std::string CreateLengthPrefix(const std::string& body) {
        std::string message;
        uint32_t length = body.length();
        message += (char)((length >> 24) & 0xFF);  // Big-endian
        message += (char)((length >> 16) & 0xFF);
        message += (char)((length >> 8) & 0xFF);
        message += (char)(length & 0xFF);
        message += body;
        return message;
    }
    
    void PrintHexDump(const char* data, int length, const std::string& title) {
        std::cout << "\n=== " << title << " ===" << std::endl;
        std::cout << "Length: " << length << " bytes" << std::endl;
        std::cout << "Hex: ";
        for (int i = 0; i < length; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << (unsigned char)data[i] << " ";
        }
        std::cout << std::dec << std::endl;
        
        // Try to decode protobuf fields
        std::cout << "Protobuf Analysis:" << std::endl;
        for (int i = 0; i < length - 1; i++) {
            unsigned char byte = (unsigned char)data[i];
            
            // Check if this looks like a field tag
            if (byte & 0x80) continue; // Skip varint continuation bytes
            
            int field_number = byte >> 3;
            int wire_type = byte & 0x07;
            
            if (field_number > 0 && field_number <= 100 && wire_type <= 5) {
                std::cout << "  Field " << field_number << ", Wire Type " << wire_type;
                
                switch (wire_type) {
                    case 0: std::cout << " (varint)"; break;
                    case 1: std::cout << " (64-bit)"; break;
                    case 2: std::cout << " (length-delimited)"; break;
                    case 3: std::cout << " (start group)"; break;
                    case 4: std::cout << " (end group)"; break;
                    case 5: std::cout << " (32-bit/float)"; break;
                }
                
                if (wire_type == 5 && i + 4 < length) {
                    float value;
                    memcpy(&value, data + i + 1, 4);
                    std::cout << " = " << value;
                }
                
                std::cout << std::endl;
            }
        }
    }
    
public:
    void TestMinimalRequest() {
        std::cout << "🔍 ML Response Debugger" << std::endl;
        std::cout << "Sending the exact 45-byte request from plugin..." << std::endl;
        
        // Create the same minimal request as the plugin
        std::string request;
        request += EncodeString(1, "16813");          // user_id
        request += EncodeFloat(2, 0.59350f);          // open_price
        request += EncodeFloat(3, 0.59000f);          // sl
        request += EncodeFloat(4, 0.59700f);          // tp
        request += EncodeFloat(5, 0.0f);              // deal_type (BUY = 0)
        request += EncodeFloat(6, 1.0f);              // lot_volume
        request += EncodeString(40, "NZDUSD");        // symbol
        
        std::string full_message = CreateLengthPrefix(request);
        
        PrintHexDump(full_message.c_str(), full_message.length(), "REQUEST TO ML SERVICE");
        
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
                PrintHexDump(response, bytes_received, "RESPONSE FROM ML SERVICE");
                
                // Extract just the protobuf part (skip length prefix)
                if (bytes_received >= 4) {
                    uint32_t protobuf_length = 
                        ((unsigned char)response[0] << 24) |
                        ((unsigned char)response[1] << 16) |
                        ((unsigned char)response[2] << 8) |
                        ((unsigned char)response[3]);
                    
                    std::cout << "\n📋 ANALYSIS:" << std::endl;
                    std::cout << "Response length prefix: " << protobuf_length << " bytes" << std::endl;
                    
                    if (bytes_received >= 4 + protobuf_length) {
                        PrintHexDump(response + 4, protobuf_length, "PROTOBUF RESPONSE BODY");
                        
                        // Try all possible field numbers with wire type 5 (float)
                        std::cout << "\n🔍 SEARCHING FOR SCORE FIELDS:" << std::endl;
                        for (int field = 1; field <= 10; field++) {
                            unsigned char target_tag = (field << 3) | 5; // Wire type 5
                            for (int i = 0; i < protobuf_length - 4; i++) {
                                if ((unsigned char)response[4 + i] == target_tag) {
                                    float value;
                                    memcpy(&value, response + 4 + i + 1, 4);
                                    std::cout << "  Field " << field << " (tag 0x" << std::hex << (int)target_tag << std::dec << ") = " << value << std::endl;
                                }
                            }
                        }
                    }
                }
                
            } else {
                std::cout << "❌ No response or connection closed: " << WSAGetLastError() << std::endl;
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
    MLResponseDebugger debugger;
    debugger.TestMinimalRequest();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 