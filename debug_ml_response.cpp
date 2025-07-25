//+------------------------------------------------------------------+
//| Enhanced ML Service Response Debug - Find the Issue            |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

class MLResponseDebugger {
private:
    std::string ml_ip = "188.245.254.12";
    int ml_port = 50051;
    int timeout_ms = 5000; // Shorter timeout for testing
    
    void LogWithTime(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);
        
        printf("[%02d:%02d:%02d] %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, message.c_str());
        fflush(stdout); // Force immediate output
    }
    
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
    
    std::string CreateSimpleRequest() {
        std::string request;
        request += EncodeFloat(1, 0.59350f);        // open_price
        request += EncodeString(51, "16813");       // user_id
        return request;
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
    void DebugMLResponse() {
        LogWithTime("=== ENHANCED ML RESPONSE DEBUG ===");
        LogWithTime("Target: " + ml_ip + ":" + std::to_string(ml_port));
        LogWithTime("Timeout: " + std::to_string(timeout_ms / 1000) + " seconds");
        LogWithTime("");
        
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        
        try {
            LogWithTime("Step 1: Initialize Winsock");
            int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            LogWithTime("WSAStartup result: " + std::to_string(wsa_result));
            
            LogWithTime("Step 2: Create socket");
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) {
                int error = WSAGetLastError();
                LogWithTime("❌ Socket creation failed: " + std::to_string(error));
                return;
            }
            LogWithTime("✅ Socket created: " + std::to_string((int)sock));
            
            LogWithTime("Step 3: Set socket timeouts");
            int recv_result = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            int send_result = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            LogWithTime("Recv timeout result: " + std::to_string(recv_result));
            LogWithTime("Send timeout result: " + std::to_string(send_result));
            
            LogWithTime("Step 4: Connect to server");
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(ml_port);
            int inet_result = inet_pton(AF_INET, ml_ip.c_str(), &serverAddr.sin_addr);
            LogWithTime("inet_pton result: " + std::to_string(inet_result));
            
            auto connect_start = std::chrono::high_resolution_clock::now();
            int connect_result = connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
            auto connect_end = std::chrono::high_resolution_clock::now();
            auto connect_duration = std::chrono::duration_cast<std::chrono::milliseconds>(connect_end - connect_start);
            
            if (connect_result == SOCKET_ERROR) {
                int error = WSAGetLastError();
                LogWithTime("❌ Connection failed in " + std::to_string(connect_duration.count()) + "ms");
                LogWithTime("❌ Connect WSA error: " + std::to_string(error));
                return;
            }
            LogWithTime("✅ Connected successfully in " + std::to_string(connect_duration.count()) + "ms");
            
            LogWithTime("Step 5: Create and send test message");
            std::string protobuf_body = CreateSimpleRequest();
            std::string full_message = CreateLengthPrefix(protobuf_body);
            LogWithTime("Message size: " + std::to_string(full_message.length()) + " bytes");
            
            auto send_start = std::chrono::high_resolution_clock::now();
            int bytes_sent = send(sock, full_message.c_str(), full_message.length(), 0);
            auto send_end = std::chrono::high_resolution_clock::now();
            auto send_duration = std::chrono::duration_cast<std::chrono::milliseconds>(send_end - send_start);
            
            if (bytes_sent == SOCKET_ERROR) {
                int error = WSAGetLastError();
                LogWithTime("❌ Send failed: WSA error " + std::to_string(error));
                return;
            }
            LogWithTime("✅ Sent " + std::to_string(bytes_sent) + " bytes in " + std::to_string(send_duration.count()) + "ms");
            
            LogWithTime("Step 6: DETAILED recv() analysis");
            char response[1024];
            memset(response, 0, sizeof(response));
            
            LogWithTime("About to call recv() - this is where the issue might be...");
            
            auto recv_start = std::chrono::high_resolution_clock::now();
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            auto recv_end = std::chrono::high_resolution_clock::now();
            auto recv_duration = std::chrono::duration_cast<std::chrono::milliseconds>(recv_end - recv_start);
            
            LogWithTime("recv() returned after " + std::to_string(recv_duration.count()) + "ms");
            LogWithTime("recv() return value: " + std::to_string(bytes_received));
            
            if (bytes_received > 0) {
                LogWithTime("✅ SUCCESS: Received " + std::to_string(bytes_received) + " bytes!");
                LogWithTime("First 32 bytes as hex:");
                for (int i = 0; i < bytes_received && i < 32; i++) {
                    printf("%02X ", (unsigned char)response[i]);
                    if ((i + 1) % 16 == 0) printf("\n");
                }
                printf("\n");
            } else if (bytes_received == 0) {
                LogWithTime("⚠️ recv() returned 0 - server closed connection cleanly");
            } else {
                int error = WSAGetLastError();
                LogWithTime("❌ recv() failed with WSA error: " + std::to_string(error));
                
                // Decode common WSA errors
                switch (error) {
                    case WSAETIMEDOUT:
                        LogWithTime("   -> WSAETIMEDOUT (10060): Operation timed out");
                        break;
                    case WSAECONNRESET:
                        LogWithTime("   -> WSAECONNRESET (10054): Connection reset by peer");
                        break;
                    case WSAECONNABORTED:
                        LogWithTime("   -> WSAECONNABORTED (10053): Connection aborted");
                        break;
                    case WSAESHUTDOWN:
                        LogWithTime("   -> WSAESHUTDOWN (10058): Socket shutdown");
                        break;
                    default:
                        LogWithTime("   -> Unknown WSA error code");
                        break;
                }
            }
            
            LogWithTime("Step 7: Cleanup");
            closesocket(sock);
            WSACleanup();
            LogWithTime("✅ Cleanup complete");
            
        } catch (const std::exception& e) {
            LogWithTime("🚨 EXCEPTION CAUGHT: " + std::string(e.what()));
            if (sock != INVALID_SOCKET) {
                closesocket(sock);
            }
            WSACleanup();
        } catch (...) {
            LogWithTime("🚨 UNKNOWN EXCEPTION CAUGHT");
            if (sock != INVALID_SOCKET) {
                closesocket(sock);
            }
            WSACleanup();
        }
        
        LogWithTime("=== DEBUG COMPLETE ===");
    }
};

int main() {
    std::cout << "MT4 A/B-book Plugin - Enhanced ML Response Debug\n";
    std::cout << "===============================================\n";
    std::cout << "This will show exactly what happens during recv()\n\n";
    
    MLResponseDebugger debugger;
    debugger.DebugMLResponse();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 