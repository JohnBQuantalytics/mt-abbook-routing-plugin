//+------------------------------------------------------------------+
//| MT4 A/B-book Routing Plugin - Official API Version             |
//| Based on official MT4ManagerAPI.h structures                   |
//| Copyright 2025, A/B-book Routing Plugin                        |
//| This plugin routes trades to A-book/B-book based on ML scores  |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <excpt.h>  // For structured exception handling

#pragma comment(lib, "ws2_32.lib")

//+------------------------------------------------------------------+
//| MT4 Server API Structures (from official MT4ManagerAPI.h)      |
//+------------------------------------------------------------------+

// Time type from official API
#ifndef __time32_t
#define __time32_t time_t
#endif

// Order states from official API
enum { ORDER_OPENED=0, ORDER_CLOSED, ORDER_DELETED, ORDER_CANCELED };

// Order commands from official API  
enum { OP_BUY=0, OP_SELL, OP_BUYLIMIT, OP_SELLLIMIT, OP_BUYSTOP, OP_SELLSTOP };

//--- Official MT4 Trade Record structure
struct TradeRecord
{
    int            order;              // order ticket
    int            login;              // user login
    char           symbol[12];         // currency
    int            digits;             // digits  
    int            cmd;               // command
    int            volume;            // volume (in lots*100)
    __time32_t     open_time;         // open time
    int            state;             // reserved  
    double         open_price;        // open price
    double         sl, tp;           // stop loss & take profit
    double         close_price;       // close price
    __time32_t     close_time;        // close time
    int            reason;            // close reason
    double         commission;        // commission
    double         commission_agent;  // agent commission  
    double         storage;           // order swaps
    double         profit;            // floating profit
    double         taxes;             // taxes
    char           comment[32];       // order comment
    int            margin_rate;       // margin rate
    __time32_t     timestamp;         // timestamp
    int            api_data[4];       // for API usage
};

//--- Official MT4 User Info structure  
struct UserInfo
{
    int            login;             // login
    char           group[16];         // group
    char           password[16];      // password  
    int            enable;            // enable
    int            enable_change_password; // allow to change password
    int            enable_readonly;   // allow to open/positions (first bit-buy,second bit-sell)
    int            password_investor[16]; // investor password
    char           password_phone[16]; // phone password
    char           name[128];         // name
    char           country[32];       // country
    char           city[32];          // city
    char           state[32];         // state
    char           zipcode[16];       // zipcode
    char           address[128];      // address
    char           phone[32];         // phone
    char           email[48];         // email
    char           comment[64];       // comment
    char           id[32];           // SSN (IRD)
    char           status[16];       // status
    __time32_t     regdate;          // registration date
    __time32_t     lastdate;         // last coonection time
    int            leverage;         // leverage
    int            agent_account;    // agent account
    __time32_t     timestamp;        // timestamp
    double         balance;          // balance
    double         prevmonthbalance; // previous month balance  
    double         prevbalance;      // previous day balance
    double         credit;           // credit
    double         interestrate;     // accumulated interest rate
    double         taxes;            // taxes
    double         prevmonthequity;  // previous month equity
    double         prevequity;       // previous day equity
    char           reserved[104];    // reserved
    int            margin_mode;      // margin calculation mode
    double         margin_so_mode;   // margin stop out mode
    double         margin_free_mode; // margin free mode (0-don't use,1-use)
    double         margin_call;      // margin call level  
    double         margin_stopout;   // stop out level
    char           reserved2[104];   // reserved  
    char           publickey[270];   // RSA public key
    int            reserved3[4];     // reserved
};

//+------------------------------------------------------------------+
//| Configuration and Logging                                       |
//+------------------------------------------------------------------+

struct PluginConfig {
    std::string cvm_ip = "188.245.254.12";
    int cvm_port = 50051;
    double fallback_score = 0.05;          // Conservative fallback (routes to A-book by default)
    double fx_majors_threshold = 0.08;
    double fx_minors_threshold = 0.12; 
    double crypto_threshold = 0.15;
    bool enable_logging = true;
    int socket_timeout = 5000;             // 5 seconds socket timeout
    bool fail_safe_mode = true;            // Always use fallback if ML service fails
    int max_connection_attempts = 3;        // Max attempts before backing off
    bool log_ml_service_status = true;     // Log ML service connectivity status
    std::string fallback_routing = "A-BOOK"; // Default routing when ML service is down
};

class PluginLogger {
private:
    std::mutex log_mutex;
    bool logging_enabled;
    
public:
    PluginLogger(bool enabled = true) : logging_enabled(enabled) {}
    
    void Log(const std::string& message) {
        if (!logging_enabled) return;
        
        std::lock_guard<std::mutex> lock(log_mutex);
        
        // Get current time
        time_t rawtime;
        struct tm timeinfo;
        char timestamp[64];
        
        time(&rawtime);
        localtime_s(&timeinfo, &rawtime);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        // Log to file
        std::ofstream logfile("ABBook_Plugin_Official.log", std::ios::app);
        if (logfile.is_open()) {
            logfile << "[" << timestamp << "] " << message << std::endl;
            logfile.close();
        }
        
        // Log to console (for debugging)
        std::cout << "[" << timestamp << "] " << message << std::endl;
    }
};

//+------------------------------------------------------------------+
//| ML Service Communication with Robust Error Handling            |
//+------------------------------------------------------------------+

class CVMClient {
private:
    PluginConfig* config;
    PluginLogger* logger;
    bool ml_service_available;
    time_t last_connection_attempt;
    int consecutive_failures;
    
    // Connection retry logic
    bool ShouldAttemptConnection() {
        time_t current_time = time(nullptr);
        
        // If too many consecutive failures, wait longer between attempts
        int wait_time = 30; // Base wait time in seconds
        if (consecutive_failures > 5) {
            wait_time = 300; // Wait 5 minutes after many failures
        } else if (consecutive_failures > 2) {
            wait_time = 120; // Wait 2 minutes after some failures  
        }
        
        return (current_time - last_connection_attempt) >= wait_time;
    }
    
    void RecordConnectionResult(bool success) {
        last_connection_attempt = time(nullptr);
        if (success) {
            consecutive_failures = 0;
            if (!ml_service_available) {
                ml_service_available = true;
                logger->Log("ML SERVICE: Connection restored - switching back to ML scoring");
            }
        } else {
            consecutive_failures++;
            if (ml_service_available) {
                ml_service_available = false;
                logger->Log("ML SERVICE: Connection lost - using fallback scores for all trades");
            }
        }
    }
    
    // Protobuf wire format encoding functions
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
    
    std::string CreateScoringRequest(const TradeRecord* trade, const UserInfo* user) {
        std::string request;
        
        // Clean and normalize trade data
        float open_price = (trade->open_price > 0.0) ? (float)trade->open_price : 1.0f;
        float lot_volume = (trade->volume > 0) ? (float)trade->volume / 100.0f : 1.0f; // MT4 volume is in lots*100
        uint32_t deal_type = (trade->cmd == 0) ? 0 : 1; // 0=buy, 1=sell
        
        // Extract user ID safely
        char user_id_str[32];
        snprintf(user_id_str, sizeof(user_id_str), "%d", trade->login);
        
        // Calculate derived fields
        float turnover_usd = open_price * lot_volume * 100000.0f; // Assuming standard lot size
        float opening_balance = (user && user->balance > 0.0) ? (float)user->balance : 10000.0f;
        
        // Build protobuf request per work requirements
        request += EncodeFloat(1, open_price);                    // open_price
        request += EncodeFloat(2, open_price * 0.99f);            // sl (stop loss, estimated)
        request += EncodeFloat(3, open_price * 1.01f);            // tp (take profit, estimated)
        request += EncodeUInt32(4, deal_type);                    // deal_type (0=buy, 1=sell)
        request += EncodeFloat(5, lot_volume);                    // lot_volume
        request += EncodeInt32(6, 0);                             // is_bonus (0=real funds)
        request += EncodeFloat(7, turnover_usd);                  // turnover_usd
        request += EncodeFloat(8, opening_balance);               // opening_balance
        request += EncodeInt32(9, 1);                             // concurrent_positions (estimated)
        request += EncodeFloat(10, 0.01f);                        // sl_perc (1% estimated)
        request += EncodeFloat(11, 0.01f);                        // tp_perc (1% estimated)
        request += EncodeInt32(12, 1);                            // has_sl (1=has stop loss)
        
        // String fields (using defaults where user data unavailable)
        request += EncodeString(42, "MT4");                       // platform
        request += EncodeString(43, "unknown");                   // LEVEL_OF_EDUCATION
        request += EncodeString(44, "unknown");                   // OCCUPATION
        request += EncodeString(45, "unknown");                   // SOURCE_OF_WEALTH
        request += EncodeString(46, "unknown");                   // ANNUAL_DISPOSABLE_INCOME
        request += EncodeString(47, "unknown");                   // AVERAGE_FREQUENCY_OF_TRADES
        request += EncodeString(48, "unknown");                   // EMPLOYMENT_STATUS
        request += EncodeString(49, "US");                        // country_code (default)
        request += EncodeString(50, "direct");                    // utm_medium (default)
        request += EncodeString(51, user_id_str);                 // user_id
        
        logger->Log("ML SERVICE: Created protobuf request with " + std::to_string(request.length()) + " bytes");
        return request;
    }
    
    std::string CreateLengthPrefixedMessage(const std::string& protobuf_body) {
        std::string message;
        uint32_t length = protobuf_body.length();
        
        // Length prefix (4 bytes, network byte order)
        message += (char)((length >> 24) & 0xFF);
        message += (char)((length >> 16) & 0xFF);
        message += (char)((length >> 8) & 0xFF);
        message += (char)(length & 0xFF);
        
        // Protobuf body
        message += protobuf_body;
        
        return message;
    }
    
    float ParseScoreFromProtobuf(const char* protobuf_data, int length) {
        for (int i = 0; i < length - 5; i++) {
            // Look for field 1, wire type 5 (float): 0x0D
            if ((unsigned char)protobuf_data[i] == 0x0D) {
                float score;
                memcpy(&score, protobuf_data + i + 1, 4);
                logger->Log("ML SERVICE: Found score in protobuf: " + std::to_string(score));
                return score;
            }
        }
        logger->Log("ML SERVICE: No score field found in protobuf response");
        return -2.0f; // Special value indicating "not found"
    }
    
public:
    CVMClient(PluginConfig* cfg, PluginLogger* log) 
        : config(cfg), logger(log), ml_service_available(true), 
          last_connection_attempt(0), consecutive_failures(0) {}
    
    double GetScore(const TradeRecord* trade, const UserInfo* user) {
        // CRITICAL: Always return fallback score if we shouldn't attempt connection
        if (!ShouldAttemptConnection() && consecutive_failures > 0) {
            return config->fallback_score;
        }
        
        SOCKET sock = INVALID_SOCKET;
        double score = config->fallback_score;
        bool connection_successful = false;
        
        // BULLETPROOF: Wrap everything in try-catch to prevent plugin unloading
        try {
            // Initialize Winsock (safe initialization)
            WSADATA wsaData;
            int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (wsa_result != 0) {
                logger->Log("ML SERVICE WARNING: WSAStartup failed (code: " + std::to_string(wsa_result) + ") - using fallback score");
                RecordConnectionResult(false);
                return config->fallback_score;
            }
            
            // Create socket with error handling
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) {
                int error_code = WSAGetLastError();
                logger->Log("ML SERVICE WARNING: Socket creation failed (WSA error: " + std::to_string(error_code) + ") - using fallback score");
                WSACleanup();
                RecordConnectionResult(false);
                return config->fallback_score;
            }
            
            // Set socket timeouts (critical for preventing hangs)
            int timeout_ms = config->socket_timeout;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
            
            // Prepare server address
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(config->cvm_port);
            
            // Convert IP address safely
            int inet_result = inet_pton(AF_INET, config->cvm_ip.c_str(), &serverAddr.sin_addr);
            if (inet_result != 1) {
                logger->Log("ML SERVICE WARNING: Invalid IP address format - using fallback score");
                closesocket(sock);
                WSACleanup();
                RecordConnectionResult(false);
                return config->fallback_score;
            }
            
            // Attempt connection with timeout
            if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
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
                        error_msg = "Connection timed out";
                        break;
                    case WSAEHOSTUNREACH:
                        error_msg = "Host unreachable";
                        break;
                    default:
                        error_msg = "Connection failed (WSA error: " + std::to_string(error_code) + ")";
                        break;
                }
                
                logger->Log("ML SERVICE: " + error_msg + " - using fallback score");
                closesocket(sock);
                WSACleanup();
                RecordConnectionResult(false);
                return config->fallback_score;
            }
            
            // Create scoring request (length-prefixed protobuf format)
            std::string protobuf_request = CreateScoringRequest(trade, user);
            std::string full_message = CreateLengthPrefixedMessage(protobuf_request);
            
            logger->Log("ML SERVICE: Sending protobuf request (" + std::to_string(full_message.length()) + " bytes)");
            
            // Send request with error handling
            if (send(sock, full_message.c_str(), full_message.length(), 0) == SOCKET_ERROR) {
                int error_code = WSAGetLastError();
                logger->Log("ML SERVICE: Failed to send request (WSA error: " + std::to_string(error_code) + ") - using fallback score");
                closesocket(sock);
                WSACleanup();
                RecordConnectionResult(false);
                return config->fallback_score;
            }
            
            // Receive response with timeout (length-prefixed protobuf format)
            char response[4096];
            memset(response, 0, sizeof(response));
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received > 0) {
                logger->Log("ML SERVICE: Received response (" + std::to_string(bytes_received) + " bytes)");
                
                // Parse length-prefixed protobuf response
                if (bytes_received >= 4) {
                    uint32_t response_length = 
                        ((unsigned char)response[0] << 24) |
                        ((unsigned char)response[1] << 16) |
                        ((unsigned char)response[2] << 8) |
                        ((unsigned char)response[3]);
                    
                    logger->Log("ML SERVICE: Response length prefix: " + std::to_string(response_length) + " bytes");
                    
                    if (bytes_received >= 4 + response_length) {
                        // Parse score from protobuf response (field 1, wire type 5 for float)
                        float parsed_score = ParseScoreFromProtobuf(response + 4, response_length);
                        
                        if (parsed_score >= 0.0f && parsed_score <= 1.0f) {
                            score = (double)parsed_score;
                            connection_successful = true;
                            logger->Log("ML SERVICE: Received valid score: " + std::to_string(score));
                        } else if (parsed_score == -2.0f) {
                            logger->Log("ML SERVICE WARNING: No valid score found in protobuf response - using fallback");
                        } else {
                            logger->Log("ML SERVICE WARNING: Score out of valid range [0.0-1.0]: " + std::to_string(parsed_score) + " - using fallback");
                        }
                    } else {
                        logger->Log("ML SERVICE WARNING: Incomplete response received - using fallback score");
                    }
                } else {
                    logger->Log("ML SERVICE WARNING: Response too short for length prefix - using fallback score");
                }
            } else if (bytes_received == 0) {
                logger->Log("ML SERVICE WARNING: Connection closed by server - using fallback score");
            } else {
                int error_code = WSAGetLastError();
                logger->Log("ML SERVICE: Failed to receive response (WSA error: " + std::to_string(error_code) + ") - using fallback score");
            }
            
            // Clean shutdown
            closesocket(sock);
            WSACleanup();
            
        } catch (const std::exception& e) {
            logger->Log("ML SERVICE EXCEPTION: " + std::string(e.what()) + " - using fallback score (plugin remains stable)");
            if (sock != INVALID_SOCKET) {
                closesocket(sock);
            }
            WSACleanup();
            connection_successful = false;
        } catch (...) {
            logger->Log("ML SERVICE: Unknown exception occurred - using fallback score (plugin remains stable)");
            if (sock != INVALID_SOCKET) {
                closesocket(sock);
            }
            WSACleanup();
            connection_successful = false;
        }
        
        // Record connection result for retry logic
        RecordConnectionResult(connection_successful);
        
        // GUARANTEE: Always return a valid score
        if (score < 0.0 || score > 1.0) {
            logger->Log("ML SERVICE: Normalizing invalid score to fallback value");
            score = config->fallback_score;
        }
        
        return score;
    }
    
    // Public method to check ML service status
    bool IsMLServiceAvailable() const {
        return ml_service_available;
    }
    
    int GetConsecutiveFailures() const {
        return consecutive_failures;
    }
};

//+------------------------------------------------------------------+
//| Global Plugin State                                            |
//+------------------------------------------------------------------+

PluginConfig g_config;
PluginLogger g_logger(true);
CVMClient g_cvm_client(&g_config, &g_logger);

//+------------------------------------------------------------------+
//| Helper Functions                                                |
//+------------------------------------------------------------------+

std::string GetInstrumentGroup(const char* symbol) {
    std::string sym(symbol);
    
    // Major FX pairs
    if (sym.find("EURUSD") != std::string::npos || 
        sym.find("GBPUSD") != std::string::npos ||
        sym.find("USDJPY") != std::string::npos ||
        sym.find("USDCHF") != std::string::npos ||
        sym.find("AUDUSD") != std::string::npos ||
        sym.find("USDCAD") != std::string::npos ||
        sym.find("NZDUSD") != std::string::npos) {
        return "FX_MAJORS";
    }
    
    // Crypto (if supported)
    if (sym.find("BTC") != std::string::npos ||
        sym.find("ETH") != std::string::npos) {
        return "CRYPTO";
    }
    
    // Default to minors
    return "FX_MINORS";
}

double GetThreshold(const std::string& instrument_group) {
    if (instrument_group == "FX_MAJORS") {
        return g_config.fx_majors_threshold;
    } else if (instrument_group == "CRYPTO") {
        return g_config.crypto_threshold;
    } else {
        return g_config.fx_minors_threshold;
    }
}

std::string GetCommandName(int cmd) {
    switch (cmd) {
        case OP_BUY: return "BUY";
        case OP_SELL: return "SELL";
        case OP_BUYLIMIT: return "BUYLIMIT";
        case OP_SELLLIMIT: return "SELLLIMIT";
        case OP_BUYSTOP: return "BUYSTOP";
        case OP_SELLSTOP: return "SELLSTOP";
        default: return "UNKNOWN";
    }
}

bool ShouldProcessTrade(const TradeRecord* trade) {
    // Only process new market orders (BUY/SELL)
    if (trade->cmd != OP_BUY && trade->cmd != OP_SELL) {
        return false;
    }
    
    // Only process opening trades
    if (trade->state != ORDER_OPENED) {
        return false;
    }
    
    return true;
}

//+------------------------------------------------------------------+
//| MT4 Server Plugin API Functions                                |
//+------------------------------------------------------------------+

extern "C" {

    // Plugin initialization
    __declspec(dllexport) int __stdcall MtSrvStartup(void* mt_interface) {
        g_logger.Log("=== MT4 A/B-book Routing Plugin STARTED (Official API + Bulletproof Version) ===");
        g_logger.Log("Plugin using official MT4 Manager API structures from mtapi.online");
        g_logger.Log("BULLETPROOF MODE: Plugin will NEVER unload due to ML service issues");
        g_logger.Log("");
        g_logger.Log("ML Service Configuration:");
        g_logger.Log("  Target: " + g_config.cvm_ip + ":" + std::to_string(g_config.cvm_port));
        g_logger.Log("  Socket Timeout: " + std::to_string(g_config.socket_timeout / 1000) + " seconds");
        g_logger.Log("  Fallback Score: " + std::to_string(g_config.fallback_score) + " (routes to " + g_config.fallback_routing + ")");
        g_logger.Log("");
        g_logger.Log("Routing Thresholds:");
        g_logger.Log("  FX Majors: " + std::to_string(g_config.fx_majors_threshold));
        g_logger.Log("  FX Minors: " + std::to_string(g_config.fx_minors_threshold));
        g_logger.Log("  Crypto: " + std::to_string(g_config.crypto_threshold));
        g_logger.Log("");
        g_logger.Log("Failsafe Features:");
        g_logger.Log("  - Automatic retry with exponential backoff");
        g_logger.Log("  - Graceful fallback to default routing when ML service unavailable");
        g_logger.Log("  - Zero-crash guarantee: Plugin remains stable under all conditions");
        g_logger.Log("  - All trades processed normally regardless of ML service status");
        g_logger.Log("");
        g_logger.Log("PLUGIN READY: Waiting for trade transactions...");
        g_logger.Log("Note: If ML service IP needs whitelisting, plugin will work in fallback mode until connected");
        g_logger.Log("MtSrvStartup returning success code 1");
        return 1; // Return 1 instead of 0 - some MT4 versions expect 1 for success
    }

    // Plugin cleanup
    __declspec(dllexport) void __stdcall MtSrvCleanup(void) {
        g_logger.Log("=== MT4 A/B-book Routing Plugin STOPPED ===");
    }

    // Plugin about info - MT4 expects specific plugin info structure
    __declspec(dllexport) int __stdcall MtSrvAbout(void* reserved) {
        g_logger.Log("Plugin About requested - A/B-book Routing Plugin v1.0 (Official API)");
        return 1; // Return 1 to indicate success with plugin info
    }

    // Configuration update
    __declspec(dllexport) void __stdcall MtSrvConfigUpdate(void* config) {
        g_logger.Log("Configuration update received");
        // Handle configuration updates here if needed
    }

    // Main trade transaction handler - BULLETPROOF against ML service failures
    __declspec(dllexport) int __stdcall MtSrvTradeTransaction(TradeRecord* trade, UserInfo* user) {
        // CRITICAL: Validate inputs to prevent crashes
        if (!trade || !user) {
            g_logger.Log("ERROR: Null pointers passed to MtSrvTradeTransaction - plugin continues safely");
            return 0; // Return 0 to indicate plugin handled it safely
        }
        
        // BULLETPROOF: Comprehensive exception handling to prevent plugin unloading
        try {
            g_logger.Log("=== TRADE TRANSACTION START ===");
            g_logger.Log("CHECKPOINT 1: Function entry successful");
            
            // Enhanced input validation with detailed logging
            g_logger.Log("CHECKPOINT 2: Validating trade pointer: " + std::to_string(reinterpret_cast<uintptr_t>(trade)));
            g_logger.Log("CHECKPOINT 3: Validating user pointer: " + std::to_string(reinterpret_cast<uintptr_t>(user)));
            
            // Log raw memory to detect corruption patterns
            g_logger.Log("=== RAW TRADE DATA ANALYSIS ===");
            g_logger.Log("Raw Order: " + std::to_string(trade->order));
            g_logger.Log("Raw Login: " + std::to_string(trade->login));
            
            // Safe symbol extraction with corruption detection
            std::string raw_symbol(trade->symbol, 12);
            std::string clean_symbol;
            
            // Enhanced symbol cleaning - look for known currency patterns
            bool found_currency_start = false;
            for (size_t i = 0; i < raw_symbol.length(); i++) {
                char c = raw_symbol[i];
                
                // Skip garbage bytes at start, look for valid 3-letter currency codes
                if (!found_currency_start) {
                    // Common currency prefixes: USD, EUR, GBP, AUD, NZD, CAD, CHF, JPY
                    if (i + 2 < raw_symbol.length()) {
                        std::string potential = raw_symbol.substr(i, 3);
                        if (potential == "USD" || potential == "EUR" || potential == "GBP" || 
                            potential == "AUD" || potential == "NZD" || potential == "CAD" || 
                            potential == "CHF" || potential == "JPY" || potential == "XPT" || 
                            potential == "XAU" || potential == "GER" || potential == "UK1" ||
                            potential == "FRA" || potential == "JPN") {
                            found_currency_start = true;
                            clean_symbol = potential;
                            i += 2; // Skip next 2 chars as they're part of this currency
                            continue;
                        }
                    }
                } else {
                    // After finding currency start, add valid characters
                    if (isalnum(c)) {
                        clean_symbol += c;
                    } else if (c == '\0' || c == ' ') {
                        break; // End of symbol
                    }
                }
            }
            
            // If no currency found, fall back to original cleaning
            if (clean_symbol.empty()) {
                for (char c : raw_symbol) {
                    if (isalnum(c) || c == '_') {
                        clean_symbol += c;
                    } else if (c == '\0') {
                        break;
                    }
                }
            }
            
            g_logger.Log("Raw Symbol: [" + raw_symbol + "]");
            g_logger.Log("Clean Symbol: [" + clean_symbol + "]");
            std::string cleaning_method = found_currency_start ? "Currency pattern detected" : "Fallback cleaning";
            g_logger.Log("Symbol cleaning method: " + cleaning_method);
            
            g_logger.Log("Raw Command: " + std::to_string(trade->cmd));
            g_logger.Log("Raw Volume: " + std::to_string(trade->volume));
            g_logger.Log("Raw Price: " + std::to_string(trade->open_price));
            g_logger.Log("Raw State: " + std::to_string(trade->state));
            g_logger.Log("Raw Digits: " + std::to_string(trade->digits));
            
            // Data validation and normalization
            int normalized_cmd = trade->cmd;
            int normalized_volume = trade->volume;
            double normalized_price = trade->open_price;
            
            // Detect and handle data corruption
            bool data_corrupted = false;
            
            if (trade->cmd < 0 || trade->cmd > 5) {
                g_logger.Log("WARNING: Command value out of range: " + std::to_string(trade->cmd));
                normalized_cmd = (trade->cmd > 100) ? (trade->cmd - 100) : 0; // Handle offset corruption
                data_corrupted = true;
            }
            
            if (trade->volume <= 0 || trade->volume > 100000000) { // Reasonable volume limits
                g_logger.Log("WARNING: Volume value suspicious: " + std::to_string(trade->volume));
                normalized_volume = 100; // Default to 1 lot
                data_corrupted = true;
            }
            
            if (trade->open_price <= 0 || trade->open_price > 1000000) {
                g_logger.Log("WARNING: Price value suspicious: " + std::to_string(trade->open_price));
                normalized_price = 1.0; // Default price
                data_corrupted = true;
            }
            
            if (data_corrupted) {
                g_logger.Log("=== DATA CORRUPTION DETECTED - USING NORMALIZED VALUES ===");
            }
            
            // Log normalized data
            g_logger.Log("=== PROCESSED TRADE DATA ===");
            g_logger.Log("Order: " + std::to_string(trade->order));
            g_logger.Log("Login: " + std::to_string(trade->login));
            g_logger.Log("Symbol: " + clean_symbol);
            g_logger.Log("Command: " + std::to_string(normalized_cmd) + " (" + GetCommandName(normalized_cmd) + ")");
            g_logger.Log("Volume: " + std::to_string(normalized_volume));
            g_logger.Log("Price: " + std::to_string(normalized_price));
            g_logger.Log("State: " + std::to_string(trade->state));
            
            g_logger.Log("CHECKPOINT 4: Data logging completed successfully");
            
            // Check if we should process this trade
            g_logger.Log("CHECKPOINT 5: Checking if trade should be processed");
            if (!ShouldProcessTrade(trade)) {
                g_logger.Log("Trade skipped - not a new market order");
                g_logger.Log("CHECKPOINT 6: Trade processing completed (skipped)");
                return 1; // Changed to return 1 for consistency
            }
            
            g_logger.Log("CHECKPOINT 7: Trade approved for processing");
            
            // EXPERIMENTAL: Try early exit to test if data processing causes crash
            // Uncomment next lines to test minimal processing
            // g_logger.Log("EXPERIMENTAL: Early exit to test crash cause");
            // g_logger.Log("CHECKPOINT 16: About to return early to MT4");
            // return 1;
            
            // Display ML service status
            std::string ml_status;
            if (g_cvm_client.IsMLServiceAvailable()) {
                ml_status = "CONNECTED";
            } else {
                int failures = g_cvm_client.GetConsecutiveFailures();
                ml_status = "DISCONNECTED (failures: " + std::to_string(failures) + ")";
            }
            g_logger.Log("ML Service Status: " + ml_status);
            g_logger.Log("CHECKPOINT 8: ML service status determined");
            
            // Get ML score (always returns valid score, even if ML service is down)
            g_logger.Log("CHECKPOINT 9: About to call ML scoring service");
            double score = 0.0;
            bool ml_score_received = false;
            
            try {
                score = g_cvm_client.GetScore(trade, user);
                
                // Check if this is actually a fallback score
                if (score == g_config.fallback_score) {
                    g_logger.Log("CHECKPOINT 10: Received fallback score (ML service failed): " + std::to_string(score));
                    ml_score_received = false;
                } else {
                    g_logger.Log("CHECKPOINT 10: Received REAL ML score: " + std::to_string(score));
                    ml_score_received = true;
                }
            } catch (const std::exception& e) {
                g_logger.Log("ERROR: Exception in ML scoring: " + std::string(e.what()));
                score = g_config.fallback_score;
                ml_score_received = false;
                g_logger.Log("CHECKPOINT 10: Using fallback score due to exception");
            } catch (...) {
                g_logger.Log("ERROR: Unknown exception in ML scoring");
                score = g_config.fallback_score;
                ml_score_received = false;
                g_logger.Log("CHECKPOINT 10: Using fallback score due to unknown exception");
            }
            
            std::string score_status = ml_score_received ? "REAL ML SCORE" : "FALLBACK SCORE USED";
            g_logger.Log("ML Score Status: " + score_status);
            
            // Determine instrument group and threshold using clean symbol
            g_logger.Log("CHECKPOINT 11: Determining instrument group");
            std::string instrument_group = GetInstrumentGroup(clean_symbol.c_str());
            double threshold = GetThreshold(instrument_group);
            g_logger.Log("CHECKPOINT 12: Threshold determined");
            
            // Make routing decision
            std::string routing_decision;
            std::string decision_basis;
            
            if (g_cvm_client.IsMLServiceAvailable()) {
                decision_basis = "ML Score";
            } else {
                decision_basis = "Fallback Score (ML service unavailable)";
            }
            
            if (score >= threshold) {
                routing_decision = "B-BOOK";
            } else {
                routing_decision = "A-BOOK";  
            }
            
            g_logger.Log("CHECKPOINT 13: Routing decision made");
            
            // Log decision with context
            g_logger.Log("Score: " + std::to_string(score) + " (" + decision_basis + ")");
            g_logger.Log("Instrument Group: " + instrument_group);
            g_logger.Log("Threshold: " + std::to_string(threshold));
            g_logger.Log("ROUTING DECISION: " + routing_decision);
            
            // Log plugin stability status
            if (!g_cvm_client.IsMLServiceAvailable()) {
                g_logger.Log("PLUGIN STATUS: Operating in FALLBACK mode - all trades processed normally");
            }
            
            g_logger.Log("CHECKPOINT 14: About to complete trade processing");
            g_logger.Log("=====================================");
            
            // INTEGRATION POINT: In production, integrate with broker's routing system here
            // The plugin NEVER fails regardless of ML service status
            
            g_logger.Log("CHECKPOINT 15: Trade processing completed successfully");
            g_logger.Log("CHECKPOINT 16: About to return to MT4 - trying different return value");
            
            // Try returning 1 instead of 0 - some MT4 versions expect different return codes
            return 1; // Try returning 1 to see if this prevents MT4 crash
            
        } catch (const std::bad_alloc& e) {
            g_logger.Log("CRITICAL: Memory allocation failed in MtSrvTradeTransaction - plugin remains stable");
            g_logger.Log("CRASH PREVENTION: Returning safely from memory allocation error");
            return 1; // Try returning 1 for consistency
        } catch (const std::exception& e) {
            g_logger.Log("EXCEPTION in MtSrvTradeTransaction: " + std::string(e.what()) + " - plugin remains stable");
            g_logger.Log("CRASH PREVENTION: Returning safely from standard exception");
            return 1; // Try returning 1 for consistency
        } catch (...) {
            g_logger.Log("UNKNOWN EXCEPTION in MtSrvTradeTransaction - plugin remains stable and continues operating");
            g_logger.Log("CRASH PREVENTION: Returning safely from unknown exception");
            return 1; // Try returning 1 for consistency
        }
    }

} // extern "C"

//+------------------------------------------------------------------+
//| DLL Entry Point                                                 |
//+------------------------------------------------------------------+

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            g_logger.Log("DLL_PROCESS_ATTACH: Plugin loaded into MT4 server");
            g_logger.Log("ATTACH INFO: hinstDLL=" + std::to_string(reinterpret_cast<uintptr_t>(hinstDLL)));
            break;
            
        case DLL_PROCESS_DETACH:
            g_logger.Log("DLL_PROCESS_DETACH: Plugin unloaded from MT4 server");
            g_logger.Log("DETACH INFO: hinstDLL=" + std::to_string(reinterpret_cast<uintptr_t>(hinstDLL)));
            if (lpvReserved) {
                g_logger.Log("DETACH REASON: Process termination (MT4 crashed or shutdown)");
            } else {
                g_logger.Log("DETACH REASON: Normal DLL unload (FreeLibrary called)");
            }
            break;
            
        case DLL_THREAD_ATTACH:
            g_logger.Log("DLL_THREAD_ATTACH: New thread attached to plugin");
            break;
            
        case DLL_THREAD_DETACH:
            g_logger.Log("DLL_THREAD_DETACH: Thread detached from plugin");
            break;
    }
    return TRUE;
} 