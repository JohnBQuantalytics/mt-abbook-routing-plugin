//+------------------------------------------------------------------+
//| ML Service Protobuf Test - ScoringRequest Simulation            |
//| Tests sending protobuf-like request to ML service               |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

class ProtobufMLTester {
private:
    std::string ml_ip = "188.245.254.12";
    int ml_port = 50051;
    int timeout_ms = 15000; // 15 seconds for testing
    
    void LogWithTime(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);
        
        printf("[%02d:%02d:%02d] %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, message.c_str());
    }
    
    // Simple protobuf-like encoding for basic types
    std::string EncodeString(int field_number, const std::string& value) {
        std::string result;
        result += (char)((field_number << 3) | 2); // Wire type 2 for strings
        result += (char)value.length();
        result += value;
        return result;
    }
    
    std::string EncodeFloat(int field_number, float value) {
        std::string result;
        result += (char)((field_number << 3) | 5); // Wire type 5 for fixed32
        char* bytes = (char*)&value;
        for (int i = 0; i < 4; i++) {
            result += bytes[i];
        }
        return result;
    }
    
    std::string CreateScoringRequest() {
        std::string request;
        
        LogWithTime("Creating ScoringRequest with sample trading data...");
        
        // Sample trade data (simulating NZDUSD SELL order)
        request += EncodeString(1, "16813");        // user_id
        request += EncodeFloat(2, 0.59350f);        // open_price
        request += EncodeFloat(3, 0.59000f);        // sl (stop loss)
        request += EncodeFloat(4, 0.59700f);        // tp (take profit)
        request += EncodeFloat(5, 1.0f);            // deal_type (1 = SELL)
        request += EncodeFloat(6, 1.0f);            // lot_volume (1 lot)
        request += EncodeFloat(7, 0.0f);            // is_bonus
        request += EncodeFloat(8, 5000.0f);         // turnover_usd
        request += EncodeFloat(9, 10000.0f);        // opening_balance
        request += EncodeFloat(10, 2.0f);           // concurrent_positions
        request += EncodeFloat(11, 0.59f);          // sl_perc
        request += EncodeFloat(12, 0.59f);          // tp_perc
        request += EncodeFloat(13, 1.0f);           // has_sl
        request += EncodeFloat(14, 1.0f);           // has_tp
        request += EncodeFloat(15, 0.65f);          // profitable_ratio
        request += EncodeFloat(16, 5.0f);           // num_open_trades
        request += EncodeFloat(17, 150.0f);         // num_closed_trades
        request += EncodeFloat(18, 35.0f);          // age
        request += EncodeFloat(19, 365.0f);         // days_since_reg
        request += EncodeFloat(20, 25000.0f);       // deposit_lifetime
        request += EncodeFloat(21, 5.0f);           // deposit_count
        request += EncodeFloat(22, 15000.0f);       // withdraw_lifetime
        request += EncodeFloat(23, 3.0f);           // withdraw_count
        request += EncodeFloat(24, 0.0f);           // vip
        request += EncodeFloat(25, 3600.0f);        // holding_time_sec
        request += EncodeFloat(26, 0.0f);           // sl_missing
        request += EncodeFloat(27, 0.0f);           // tp_missing
        request += EncodeFloat(28, 59350.0f);       // lot_usd_value
        request += EncodeFloat(29, 0.15f);          // exposure_to_balance_ratio
        request += EncodeFloat(30, 0.0f);           // rapid_entry_exit
        request += EncodeFloat(31, 0.25f);          // abuse_risk_score
        request += EncodeFloat(32, 365.0f);         // trader_tenure_days
        request += EncodeFloat(33, 1.67f);          // deposit_to_withdraw_ratio
        request += EncodeFloat(34, 1.0f);           // education_known
        request += EncodeFloat(35, 1.0f);           // occupation_known
        request += EncodeFloat(36, 0.01f);          // lot_to_balance_ratio
        request += EncodeFloat(37, 0.07f);          // deposit_density
        request += EncodeFloat(38, 0.008f);         // withdrawal_density
        request += EncodeFloat(39, 333.0f);         // turnover_per_trade
        
        // String fields
        request += EncodeString(40, "NZDUSD");      // symbol
        request += EncodeString(41, "FX_MAJORS");   // inst_group
        request += EncodeString(42, "MEDIUM");      // frequency
        request += EncodeString(43, "STANDARD");    // trading_group
        request += EncodeString(44, "RETAIL");      // licence
        request += EncodeString(45, "MT4");         // platform
        request += EncodeString(46, "UNIVERSITY");  // LEVEL_OF_EDUCATION
        request += EncodeString(47, "EMPLOYED");    // OCCUPATION
        request += EncodeString(48, "SALARY");      // SOURCE_OF_WEALTH
        request += EncodeString(49, "50K-100K");    // ANNUAL_DISPOSABLE_INCOME
        request += EncodeString(50, "WEEKLY");      // AVERAGE_FREQUENCY_OF_TRADES
        request += EncodeString(51, "EMPLOYED");    // EMPLOYMENT_STATUS
        request += EncodeString(52, "US");          // country_code
        request += EncodeString(53, "ORGANIC");     // utm_medium
        
        LogWithTime("ScoringRequest created with " + std::to_string(request.length()) + " bytes");
        return request;
    }
    
public:
    void TestProtobufRequest() {
        LogWithTime("=== ML SERVICE PROTOBUF TEST ===");
        LogWithTime("Target: " + ml_ip + ":" + std::to_string(ml_port));
        LogWithTime("Testing protobuf ScoringRequest format");
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
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(ml_port);
            inet_pton(AF_INET, ml_ip.c_str(), &serverAddr.sin_addr);
            
            // Connect
            LogWithTime("Step 4: Connecting to ML service...");
            if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                int error_code = WSAGetLastError();
                LogWithTime("❌ Connection failed (WSA error: " + std::to_string(error_code) + ")");
                closesocket(sock);
                WSACleanup();
                return;
            }
            LogWithTime("✅ Connected successfully");
            
            // Test different protobuf approaches
            TestProtobufFormats(sock);
            
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
        }
    }
    
    void TestProtobufFormats(SOCKET sock) {
        LogWithTime("Step 5: Testing protobuf request formats...");
        LogWithTime("");
        
        // Test 1: Raw protobuf-like binary
        LogWithTime("Test 1: Raw protobuf binary format");
        std::string protobuf_request = CreateScoringRequest();
        TestSingleRequest(sock, protobuf_request, "Binary protobuf");
        
        // Test 2: gRPC-like with length prefix
        LogWithTime("Test 2: gRPC-like format with length prefix");
        std::string grpc_request;
        grpc_request += (char)0; // Compression flag
        uint32_t length = protobuf_request.length();
        grpc_request += (char)((length >> 24) & 0xFF);
        grpc_request += (char)((length >> 16) & 0xFF);
        grpc_request += (char)((length >> 8) & 0xFF);
        grpc_request += (char)(length & 0xFF);
        grpc_request += protobuf_request;
        TestSingleRequest(sock, grpc_request, "gRPC with length prefix");
        
        // Test 3: HTTP/2 gRPC format simulation
        LogWithTime("Test 3: HTTP/2 gRPC simulation");
        std::string http2_request = 
            "POST /scoring.ScoringService/GetScore HTTP/1.1\r\n"
            "Host: " + ml_ip + ":" + std::to_string(ml_port) + "\r\n"
            "Content-Type: application/grpc\r\n"
            "Content-Length: " + std::to_string(protobuf_request.length()) + "\r\n"
            "\r\n" + protobuf_request;
        TestSingleRequest(sock, http2_request, "HTTP/2 gRPC simulation");
    }
    
    void TestSingleRequest(SOCKET sock, const std::string& request, const std::string& format_name) {
        LogWithTime("Testing: " + format_name);
        LogWithTime("Request size: " + std::to_string(request.length()) + " bytes");
        
        // Send request
        if (send(sock, request.c_str(), request.length(), 0) == SOCKET_ERROR) {
            int error_code = WSAGetLastError();
            LogWithTime("❌ Failed to send request (WSA error: " + std::to_string(error_code) + ")");
            return;
        }
        LogWithTime("✅ Request sent successfully");
        
        // Receive response
        char response[4096];
        memset(response, 0, sizeof(response));
        
        auto start_time = std::chrono::high_resolution_clock::now();
        int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if (bytes_received > 0) {
            response[bytes_received] = '\0';
            LogWithTime("✅ Response received in " + std::to_string(duration.count()) + "ms:");
            LogWithTime("Response size: " + std::to_string(bytes_received) + " bytes");
            
            // Display response as hex and text
            LogWithTime("Response (hex): ");
            int display_bytes = (bytes_received < 64) ? bytes_received : 64;
            for (int i = 0; i < display_bytes; i++) {
                printf("%02X ", (unsigned char)response[i]);
                if ((i + 1) % 16 == 0) printf("\n");
            }
            printf("\n");
            
            LogWithTime("Response (text): [" + std::string(response, bytes_received) + "]");
            
            // Try to parse as protobuf response
            ParsePotentialScore(response, bytes_received);
            LogWithTime("");
            return; // Stop after first successful response
            
        } else if (bytes_received == 0) {
            LogWithTime("❌ Connection closed by server after " + std::to_string(duration.count()) + "ms");
        } else {
            int error_code = WSAGetLastError();
            if (error_code == WSAETIMEDOUT) {
                LogWithTime("❌ Response timeout after " + std::to_string(duration.count()) + "ms");
            } else {
                LogWithTime("❌ Failed to receive response (WSA error: " + std::to_string(error_code) + ")");
            }
        }
        LogWithTime("");
    }
    
    void ParsePotentialScore(const char* data, int length) {
        // Look for float values that could be scores (0.0 - 1.0)
        LogWithTime("Analyzing response for potential scores...");
        
        for (int i = 0; i <= length - 4; i++) {
            float potential_score;
            memcpy(&potential_score, data + i, 4);
            
            if (potential_score >= 0.0f && potential_score <= 1.0f && 
                potential_score != 0.0f) { // Exclude obvious zeros
                LogWithTime("Potential score found at offset " + std::to_string(i) + ": " + std::to_string(potential_score));
            }
        }
    }
};

int main() {
    std::cout << "MT4 A/B-book Plugin - Protobuf ML Service Test\n";
    std::cout << "===============================================\n\n";
    
    ProtobufMLTester tester;
    tester.TestProtobufRequest();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 