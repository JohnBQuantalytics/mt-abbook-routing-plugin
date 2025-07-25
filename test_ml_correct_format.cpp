//+------------------------------------------------------------------+
//| ML Service Correct Format Test - Based on Work Requirements     |
//| Tests the exact format specified in the work requirements       |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

class CorrectMLTester {
private:
    std::string ml_ip = "188.245.254.12";
    int ml_port = 50051;
    int timeout_ms = 15000;
    
    void LogWithTime(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);
        
        printf("[%02d:%02d:%02d] %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, message.c_str());
    }
    
    // Protobuf wire format encoding
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
        result += EncodeVarint(field_tag); // Encode field tag as varint
        char* bytes = (char*)&value;
        for (int i = 0; i < 4; i++) {
            result += bytes[i];
        }
        return result;
    }
    
    std::string EncodeUInt32(int field_number, uint32_t value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 0; // Wire type 0 for varint
        result += EncodeVarint(field_tag); // Encode field tag as varint
        result += EncodeVarint(value);
        return result;
    }
    
    std::string EncodeInt32(int field_number, int32_t value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 0; // Wire type 0 for varint
        result += EncodeVarint(field_tag); // Encode field tag as varint
        result += EncodeVarint((uint64_t)value);
        return result;
    }
    
    std::string EncodeString(int field_number, const std::string& value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 2; // Wire type 2 for length-delimited
        result += EncodeVarint(field_tag); // Encode field tag as varint
        result += EncodeVarint(value.length());
        result += value;
        return result;
    }
    
    std::string CreateCorrectScoringRequest() {
        std::string request;
        
        LogWithTime("Creating ScoringRequest per work requirements...");
        
        // Numeric fields from NZDUSD SELL trade simulation
        request += EncodeFloat(1, 0.59350f);        // open_price
        request += EncodeFloat(2, 0.59000f);        // sl (stop loss)
        request += EncodeFloat(3, 0.59700f);        // tp (take profit)
        request += EncodeUInt32(4, 1);              // deal_type (1 = sell/short)
        request += EncodeFloat(5, 1.0f);            // lot_volume (1.0 lots)
        request += EncodeInt32(6, 0);               // is_bonus (0 = real funds)
        request += EncodeFloat(7, 59350.0f);        // turnover_usd
        request += EncodeFloat(8, 10000.0f);        // opening_balance
        request += EncodeInt32(9, 2);               // concurrent_positions
        request += EncodeFloat(10, 0.59f);          // sl_perc
        request += EncodeFloat(11, 0.59f);          // tp_perc
        request += EncodeInt32(12, 1);              // has_sl (1 = has stop loss)
        
        // String fields
        request += EncodeString(42, "MT4");                     // platform
        request += EncodeString(43, "bachelor");                // LEVEL_OF_EDUCATION
        request += EncodeString(44, "engineer");                // OCCUPATION
        request += EncodeString(45, "salary");                  // SOURCE_OF_WEALTH
        request += EncodeString(46, "50k-100k");                // ANNUAL_DISPOSABLE_INCOME
        request += EncodeString(47, "weekly");                  // AVERAGE_FREQUENCY_OF_TRADES
        request += EncodeString(48, "employed");                // EMPLOYMENT_STATUS
        request += EncodeString(49, "US");                      // country_code
        request += EncodeString(50, "cpc");                     // utm_medium
        request += EncodeString(51, "16813");                   // user_id
        
        LogWithTime("ScoringRequest created with " + std::to_string(request.length()) + " bytes");
        LogWithTime("Fields: 12 numeric + 21 string = 21 total fields");
        
        // DEBUG: Show exact bytes for user_id field
        std::string user_id_debug = EncodeString(51, "16813");
        std::string hex_output = "user_id field bytes: ";
        for (size_t i = 0; i < user_id_debug.length(); i++) {
            char hex_buf[4];
            sprintf(hex_buf, "%02X ", (unsigned char)user_id_debug[i]);
            hex_output += hex_buf;
        }
        LogWithTime(hex_output);
        
        // Verify field 51 calculation
        int expected_byte = (51 << 3) | 2; // Field 51, wire type 2
        char calc_info[100];
        sprintf(calc_info, "Field 51 wire type 2 should be: 0x%02X (decimal %d)", expected_byte, expected_byte);
        LogWithTime(calc_info);
        
        return request;
    }
    
    std::string CreateLengthPrefixedMessage(const std::string& protobuf_body) {
        std::string message;
        uint32_t length = protobuf_body.length();
        
        LogWithTime("Creating length-prefixed message format: [length][protobuf_body]");
        LogWithTime("Protobuf body length: " + std::to_string(length) + " bytes");
        
        // Length prefix (4 bytes, network byte order)
        message += (char)((length >> 24) & 0xFF);
        message += (char)((length >> 16) & 0xFF);
        message += (char)((length >> 8) & 0xFF);
        message += (char)(length & 0xFF);
        
        // Protobuf body
        message += protobuf_body;
        
        LogWithTime("Total message length: " + std::to_string(message.length()) + " bytes");
        return message;
    }
    
public:
    void TestCorrectFormat() {
        LogWithTime("=== ML SERVICE CORRECT FORMAT TEST ===");
        LogWithTime("Based on actual work requirements specification");
        LogWithTime("Target: " + ml_ip + ":" + std::to_string(ml_port));
        LogWithTime("Format: Raw TCP with [length][protobuf_body]");
        LogWithTime("");
        
        WSADATA wsaData;
        SOCKET sock = INVALID_SOCKET;
        
        try {
            // Initialize Winsock
            LogWithTime("Step 1: Initializing Winsock...");
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            LogWithTime("✅ Winsock initialized");
            
            // Create socket
            LogWithTime("Step 2: Creating TCP socket...");
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            LogWithTime("✅ Socket created");
            
            // Set timeouts
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            LogWithTime("✅ Timeouts set to " + std::to_string(timeout_ms / 1000) + " seconds");
            
            // Connect
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(ml_port);
            inet_pton(AF_INET, ml_ip.c_str(), &serverAddr.sin_addr);
            
            LogWithTime("Step 3: Connecting to ML service...");
            if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                int error_code = WSAGetLastError();
                LogWithTime("❌ Connection failed (WSA error: " + std::to_string(error_code) + ")");
                return;
            }
            LogWithTime("✅ Connected successfully");
            
            // Create and send correct format
            LogWithTime("Step 4: Creating ScoringRequest...");
            std::string protobuf_request = CreateCorrectScoringRequest();
            std::string full_message = CreateLengthPrefixedMessage(protobuf_request);
            
            LogWithTime("Step 5: Sending length-prefixed protobuf message...");
            if (send(sock, full_message.c_str(), full_message.length(), 0) == SOCKET_ERROR) {
                int error_code = WSAGetLastError();
                LogWithTime("❌ Send failed (WSA error: " + std::to_string(error_code) + ")");
                return;
            }
            LogWithTime("✅ Message sent successfully");
            
            // Receive response
            LogWithTime("Step 6: Waiting for ScoringResponse...");
            char response[4096];
            memset(response, 0, sizeof(response));
            
            auto start_time = std::chrono::high_resolution_clock::now();
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            if (bytes_received > 0) {
                LogWithTime("✅ Response received in " + std::to_string(duration.count()) + "ms!");
                LogWithTime("Response size: " + std::to_string(bytes_received) + " bytes");
                
                // Parse length-prefixed response
                if (bytes_received >= 4) {
                    uint32_t response_length = 
                        ((unsigned char)response[0] << 24) |
                        ((unsigned char)response[1] << 16) |
                        ((unsigned char)response[2] << 8) |
                        ((unsigned char)response[3]);
                    
                    LogWithTime("Response length prefix: " + std::to_string(response_length) + " bytes");
                    
                    if (bytes_received >= 4 + response_length) {
                        LogWithTime("✅ Complete response received");
                        
                        // Display protobuf body as hex
                        LogWithTime("Protobuf response (hex):");
                        for (int i = 4; i < bytes_received && i < 68; i++) {
                            printf("%02X ", (unsigned char)response[i]);
                            if ((i - 3) % 16 == 0) printf("\n");
                        }
                        printf("\n");
                        
                        // Try to parse score (field 1, wire type 5 for float)
                        ParseScore(response + 4, response_length);
                    } else {
                        LogWithTime("⚠️ Incomplete response received");
                    }
                } else {
                    LogWithTime("⚠️ Response too short for length prefix");
                }
                
            } else if (bytes_received == 0) {
                LogWithTime("❌ Connection closed by server");
            } else {
                int error_code = WSAGetLastError();
                LogWithTime("❌ Receive failed (WSA error: " + std::to_string(error_code) + ")");
            }
            
            closesocket(sock);
            WSACleanup();
            LogWithTime("✅ Connection closed");
            
        } catch (const std::exception& e) {
            LogWithTime("EXCEPTION: " + std::string(e.what()));
            if (sock != INVALID_SOCKET) closesocket(sock);
            WSACleanup();
        }
    }
    
    void ParseScore(const char* protobuf_data, int length) {
        LogWithTime("Parsing ScoringResponse for score field...");
        
        for (int i = 0; i < length - 5; i++) {
            // Look for field 1, wire type 5 (float): 0x0D
            if ((unsigned char)protobuf_data[i] == 0x0D) {
                float score;
                memcpy(&score, protobuf_data + i + 1, 4);
                
                if (score >= 0.0f && score <= 1.0f) {
                    LogWithTime("🎯 SCORE FOUND: " + std::to_string(score));
                    LogWithTime("Score analysis:");
                    LogWithTime("  - Raw value: " + std::to_string(score));
                    LogWithTime("  - In range [0,1]: ✅");
                    std::string routing_decision = (score >= 0.08f) ? "B-BOOK" : "A-BOOK";
                    LogWithTime("  - Routing: " + routing_decision);
                    return;
                }
            }
        }
        
        LogWithTime("⚠️ No valid score found in response");
    }
};

int main() {
    std::cout << "MT4 A/B-book Plugin - Correct ML Service Format Test\n";
    std::cout << "====================================================\n";
    std::cout << "Based on actual work requirements specification\n\n";
    
    CorrectMLTester tester;
    tester.TestCorrectFormat();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 