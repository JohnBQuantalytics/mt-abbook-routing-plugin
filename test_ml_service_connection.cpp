//+------------------------------------------------------------------+
//| ML Service Connection Test - Standalone                         |
//| Tests direct connection to ML service at 188.245.254.12:50051   |
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

class MLServiceTester {
private:
    std::string ml_ip = "188.245.254.12";
    int ml_port = 50051;
    int timeout_ms = 10000; // 10 seconds for testing
    
    void LogWithTime(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);
        
        printf("[%02d:%02d:%02d] %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, message.c_str());
    }
    
public:
    void TestConnection() {
        LogWithTime("=== ML SERVICE CONNECTION TEST ===");
        LogWithTime("Target: " + ml_ip + ":" + std::to_string(ml_port));
        LogWithTime("Timeout: " + std::to_string(timeout_ms / 1000) + " seconds");
        LogWithTime("");
        
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        
        try {
            // Initialize Winsock
            LogWithTime("Step 1: Initializing Winsock...");
            int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (wsa_result != 0) {
                LogWithTime("ERROR: WSAStartup failed with code: " + std::to_string(wsa_result));
                return;
            }
            LogWithTime("✅ Winsock initialized successfully");
            
            // Create socket
            LogWithTime("Step 2: Creating socket...");
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) {
                int error_code = WSAGetLastError();
                LogWithTime("ERROR: Socket creation failed (WSA error: " + std::to_string(error_code) + ")");
                WSACleanup();
                return;
            }
            LogWithTime("✅ Socket created successfully");
            
            // Set socket timeouts
            LogWithTime("Step 3: Setting socket timeouts...");
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            LogWithTime("✅ Socket timeouts set to " + std::to_string(timeout_ms / 1000) + " seconds");
            
            // Prepare server address
            LogWithTime("Step 4: Preparing server address...");
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(ml_port);
            
            int inet_result = inet_pton(AF_INET, ml_ip.c_str(), &serverAddr.sin_addr);
            if (inet_result != 1) {
                LogWithTime("ERROR: Invalid IP address format");
                closesocket(sock);
                WSACleanup();
                return;
            }
            LogWithTime("✅ Server address prepared");
            
            // Attempt connection
            LogWithTime("Step 5: Connecting to ML service...");
            auto start_time = std::chrono::high_resolution_clock::now();
            
            if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                
                int error_code = WSAGetLastError();
                std::string error_msg;
                
                switch (error_code) {
                    case WSAECONNREFUSED:
                        error_msg = "Connection refused (service not running or port closed)";
                        break;
                    case WSAENETUNREACH:
                        error_msg = "Network unreachable";
                        break;
                    case WSAETIMEDOUT:
                        error_msg = "Connection timed out after " + std::to_string(duration.count()) + "ms";
                        break;
                    case WSAEHOSTUNREACH:
                        error_msg = "Host unreachable";
                        break;
                    default:
                        error_msg = "Connection failed (WSA error: " + std::to_string(error_code) + ")";
                        break;
                }
                
                LogWithTime("❌ CONNECTION FAILED: " + error_msg);
                closesocket(sock);
                WSACleanup();
                return;
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            LogWithTime("✅ Connected successfully in " + std::to_string(duration.count()) + "ms");
            
            // Test ML service request
            LogWithTime("Step 6: Testing ML service request...");
            TestMLRequest(sock);
            
            // Clean up
            closesocket(sock);
            WSACleanup();
            LogWithTime("✅ Connection closed cleanly");
            
        } catch (const std::exception& e) {
            LogWithTime("EXCEPTION: " + std::string(e.what()));
            if (sock != INVALID_SOCKET) {
                closesocket(sock);
            }
            WSACleanup();
        } catch (...) {
            LogWithTime("UNKNOWN EXCEPTION occurred");
            if (sock != INVALID_SOCKET) {
                closesocket(sock);
            }
            WSACleanup();
        }
    }
    
    void TestMLRequest(SOCKET sock) {
        // Test different request formats to see what the ML service expects
        const char* test_requests[] = {
            "SCORE_REQUEST|ORDER:75|LOGIN:16813|SYMBOL:NZDUSD|CMD:1|VOLUME:100|PRICE:0.59350|END\n",
            "{\"symbol\":\"NZDUSD\",\"cmd\":1,\"volume\":100,\"price\":0.59350}\n",
            "NZDUSD SELL 100 0.59350\n",
            "NZDUSD\n"
        };
        
        int num_requests = sizeof(test_requests) / sizeof(test_requests[0]);
        
        for (int i = 0; i < num_requests; i++) {
            LogWithTime("Testing request format " + std::to_string(i + 1) + ":");
            
            std::string request_str(test_requests[i]);
            std::string display_str = request_str.substr(0, request_str.find('\n'));
            LogWithTime("Request: " + display_str);
            
            // Send request
            int send_len = strlen(test_requests[i]);
            if (send(sock, test_requests[i], send_len, 0) == SOCKET_ERROR) {
                int error_code = WSAGetLastError();
                LogWithTime("❌ Failed to send request (WSA error: " + std::to_string(error_code) + ")");
                continue;
            }
            LogWithTime("✅ Request sent successfully");
            
            // Receive response
            char response[1024];
            memset(response, 0, sizeof(response));
            
            auto start_time = std::chrono::high_resolution_clock::now();
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            if (bytes_received > 0) {
                response[bytes_received] = '\0';
                LogWithTime("✅ Response received in " + std::to_string(duration.count()) + "ms:");
                LogWithTime("Response: [" + std::string(response) + "]");
                
                // Try to parse score
                char* score_pos = strstr(response, "SCORE:");
                if (score_pos && strlen(score_pos) > 6) {
                    double score = atof(score_pos + 6);
                    LogWithTime("✅ Parsed score: " + std::to_string(score));
                } else {
                    LogWithTime("⚠️ Could not parse score from response");
                }
                
                LogWithTime(""); // Empty line for readability
                return; // Stop after first successful response
                
            } else if (bytes_received == 0) {
                LogWithTime("❌ Connection closed by server after " + std::to_string(duration.count()) + "ms");
            } else {
                int error_code = WSAGetLastError();
                if (error_code == WSAETIMEDOUT) {
                    LogWithTime("❌ Response timeout after " + std::to_string(duration.count()) + "ms (WSA error: 10060)");
                } else {
                    LogWithTime("❌ Failed to receive response (WSA error: " + std::to_string(error_code) + ")");
                }
            }
            
            LogWithTime(""); // Empty line for readability
        }
    }
};

int main() {
    std::cout << "MT4 A/B-book Plugin - ML Service Connection Test\n";
    std::cout << "=================================================\n\n";
    
    MLServiceTester tester;
    tester.TestConnection();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 