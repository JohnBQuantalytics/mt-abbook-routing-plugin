//+------------------------------------------------------------------+
//| Complete ML Fields Test - All 60 Protobuf Fields               |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class CompleteMlTester {
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
    
    float ParseScore(const std::string& response) {
        if (response.length() < 9) return -1.0f;
        
        const char* protobuf_data = response.c_str() + 4;
        int protobuf_length = response.length() - 4;
        
        // Look for score field (field 1, wire type 5)
        for (int i = 0; i < protobuf_length - 4; i++) {
            if ((unsigned char)protobuf_data[i] == 0x0D) { // Field 1, wire type 5
                float score;
                memcpy(&score, protobuf_data + i + 1, 4);
                return score;
            }
        }
        return -1.0f;
    }
    
public:
    void TestCompleteMLFeatures() {
        std::cout << "=== COMPLETE ML FEATURES TEST (All 60 Fields) ===" << std::endl;
        std::cout << "Building complete ScoringRequest with realistic trading data...\n" << std::endl;
        
        std::string request;
        
        // === CORE TRADE DATA (Fields 1-5) ===
        std::cout << "Adding Core Trade Data (1-5)..." << std::endl;
        request += EncodeFloat(1, 0.59350f);        // open_price
        request += EncodeFloat(2, 0.59000f);        // sl (stop loss)
        request += EncodeFloat(3, 0.59700f);        // tp (take profit)
        request += EncodeInt64(4, 1);               // deal_type (1=sell)
        request += EncodeFloat(5, 1.0f);            // lot_volume
        
        // === ACCOUNT & TRADING HISTORY (Fields 6-36) ===
        std::cout << "Adding Account & Trading History (6-36)..." << std::endl;
        request += EncodeInt64(6, 0);               // is_bonus
        request += EncodeFloat(7, 59350.0f);        // turnover_usd
        request += EncodeFloat(8, 10000.0f);        // opening_balance
        request += EncodeInt64(9, 2);               // concurrent_positions
        request += EncodeFloat(10, 0.0059f);        // sl_perc
        request += EncodeFloat(11, 0.0059f);        // tp_perc
        request += EncodeInt64(12, 1);              // has_sl
        request += EncodeInt64(13, 1);              // has_tp
        request += EncodeFloat(14, 0.65f);          // profitable_ratio
        request += EncodeInt64(15, 3);              // num_open_trades
        request += EncodeInt64(16, 85);             // num_closed_trades
        request += EncodeInt64(17, 32);             // age
        request += EncodeInt64(18, 145);            // days_since_reg
        request += EncodeFloat(19, 15000.0f);       // deposit_lifetime
        request += EncodeInt64(20, 7);              // deposit_count
        request += EncodeFloat(21, 2500.0f);        // withdraw_lifetime
        request += EncodeInt64(22, 3);              // withdraw_count
        request += EncodeInt64(23, 0);              // vip
        request += EncodeInt64(24, 4200);           // holding_time_sec
        request += EncodeFloat(25, 100000.0f);      // lot_usd_value
        request += EncodeFloat(26, -850.0f);        // max_drawdown
        request += EncodeFloat(27, 1200.0f);        // max_runup
        request += EncodeFloat(28, 8.5f);           // volume_24h
        request += EncodeFloat(29, 145.0f);         // trader_tenure_days
        request += EncodeFloat(30, 6.0f);           // deposit_to_withdraw_ratio
        request += EncodeInt64(31, 1);              // education_known
        request += EncodeInt64(32, 1);              // occupation_known
        request += EncodeFloat(33, 0.59f);          // lot_to_balance_ratio
        request += EncodeFloat(34, 0.048f);         // deposit_density
        request += EncodeFloat(35, 0.021f);         // withdrawal_density
        request += EncodeFloat(36, 698.0f);         // turnover_per_trade
        
        // === RECENT PERFORMANCE METRICS (Fields 37-45) ===
        std::cout << "Adding Recent Performance Metrics (37-45)..." << std::endl;
        request += EncodeFloat(37, 0.70f);          // profitable_ratio_24h
        request += EncodeFloat(38, 0.65f);          // profitable_ratio_48h
        request += EncodeFloat(39, 0.62f);          // profitable_ratio_72h
        request += EncodeInt64(40, 12);             // trades_count_24h
        request += EncodeInt64(41, 23);             // trades_count_48h
        request += EncodeInt64(42, 34);             // trades_count_72h
        request += EncodeFloat(43, 85.5f);          // avg_profit_24h
        request += EncodeFloat(44, 72.3f);          // avg_profit_48h
        request += EncodeFloat(45, 68.8f);          // avg_profit_72h
        
        // === CONTEXT & METADATA (Fields 46-60) ===  
        std::cout << "Adding Context & Metadata (46-60)..." << std::endl;
        request += EncodeString(46, "NZDUSD");      // symbol
        request += EncodeString(47, "FXMajors");    // inst_group
        request += EncodeString(48, "medium");      // frequency
        request += EncodeString(49, "standard");    // trading_group
        request += EncodeString(50, "CY");          // licence
        request += EncodeString(51, "MT4");         // platform
        request += EncodeString(52, "bachelor");    // LEVEL_OF_EDUCATION
        request += EncodeString(53, "engineer");    // OCCUPATION
        request += EncodeString(54, "salary");      // SOURCE_OF_WEALTH
        request += EncodeString(55, "50k-100k");    // ANNUAL_DISPOSABLE_INCOME
        request += EncodeString(56, "weekly");      // AVERAGE_FREQUENCY_OF_TRADES
        request += EncodeString(57, "employed");    // EMPLOYMENT_STATUS
        request += EncodeString(58, "CY");          // country_code
        request += EncodeString(59, "cpc");         // utm_medium
        request += EncodeString(60, "16813");       // user_id
        
        std::cout << "\n🎯 Complete protobuf message built (" << request.length() << " bytes)" << std::endl;
        std::cout << "🚀 Sending to ML service..." << std::endl;
        
        // Test the complete request
        std::string full_message = CreateLengthPrefix(request);
        
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
                std::cout << "❌ Connection failed to " << ml_ip << ":" << ml_port << std::endl;
                return;
            }
            
            std::cout << "✅ Connected to ML service" << std::endl;
            
            if (send(sock, full_message.c_str(), full_message.length(), 0) == SOCKET_ERROR) {
                std::cout << "❌ Send failed" << std::endl;
                return;
            }
            
            std::cout << "✅ Sent " << full_message.length() << " bytes" << std::endl;
            
            char response[256];
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received > 0) {
                std::string response_str(response, bytes_received);
                float score = ParseScore(response_str);
                
                if (score >= 0.0f && score <= 1.0f) {
                    std::cout << "\n🎉 SUCCESS! ML service processed complete trading data!" << std::endl;
                    std::cout << "📊 ML Score: " << score << std::endl;
                    std::cout << "🎯 Routing Decision: " << (score >= 0.08f ? "B-BOOK" : "A-BOOK") << std::endl;
                    std::cout << "💡 This proves the ML service works with full feature set!" << std::endl;
                } else {
                    std::cout << "⚠️ Response received but invalid score: " << score << std::endl;
                    std::cout << "Raw response (" << bytes_received << " bytes): ";
                    for (int i = 0; i < bytes_received && i < 20; i++) {
                        printf("%02X ", (unsigned char)response[i]);
                    }
                    std::cout << std::endl;
                }
            } else if (bytes_received == 0) {
                std::cout << "⚠️ Connection closed by server - may indicate format issue" << std::endl;
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
        
        std::cout << "\n=== SUMMARY ===" << std::endl;
        std::cout << "✅ Built complete 60-field ScoringRequest" << std::endl;
        std::cout << "✅ Includes all core trading data (price, volume, SL/TP)" << std::endl;
        std::cout << "✅ Includes account history and performance metrics" << std::endl;
        std::cout << "✅ Includes recent performance data (critical for ML)" << std::endl;
        std::cout << "✅ Includes context metadata (symbol, platform, user profile)" << std::endl;
        std::cout << "🎯 This is the PROPER format for intelligent A/B routing!" << std::endl;
    }
};

int main() {
    CompleteMlTester tester;
    tester.TestCompleteMLFeatures();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 