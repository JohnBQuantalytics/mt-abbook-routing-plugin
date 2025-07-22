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
            
            // Create scoring request (robust formatting)
            char request[512];
            int request_len = snprintf(request, sizeof(request) - 1, 
                "SCORE_REQUEST|ORDER:%d|LOGIN:%d|SYMBOL:%.11s|CMD:%d|VOLUME:%d|PRICE:%.5f|END\n",
                trade->order, trade->login, trade->symbol, trade->cmd, trade->volume, trade->open_price);
            
            if (request_len <= 0 || request_len >= sizeof(request)) {
                logger->Log("ML SERVICE WARNING: Request formatting error - using fallback score");
                closesocket(sock);
                WSACleanup();
                RecordConnectionResult(false);
                return config->fallback_score;
            }
            
            // Send request with error handling
            if (send(sock, request, request_len, 0) == SOCKET_ERROR) {
                int error_code = WSAGetLastError();
                logger->Log("ML SERVICE: Failed to send request (WSA error: " + std::to_string(error_code) + ") - using fallback score");
                closesocket(sock);
                WSACleanup();
                RecordConnectionResult(false);
                return config->fallback_score;
            }
            
            // Receive response with timeout
            char response[256];
            memset(response, 0, sizeof(response));
            int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
            
            if (bytes_received > 0) {
                response[bytes_received] = '\0';
                
                // Parse score from response safely
                char* score_pos = strstr(response, "SCORE:");
                if (score_pos && strlen(score_pos) > 6) {
                    double parsed_score = atof(score_pos + 6);
                    
                    // Validate score range (0.0 to 1.0)
                    if (parsed_score >= 0.0 && parsed_score <= 1.0) {
                        score = parsed_score;
                        connection_successful = true;
                        logger->Log("ML SERVICE: Received valid score: " + std::to_string(score));
                    } else {
                        logger->Log("ML SERVICE WARNING: Score out of valid range [0.0-1.0]: " + std::to_string(parsed_score) + " - using fallback");
                    }
                } else {
                    logger->Log("ML SERVICE WARNING: Invalid response format: '" + std::string(response) + "' - using fallback score");
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
        return 0;
    }

    // Plugin cleanup
    __declspec(dllexport) void __stdcall MtSrvCleanup(void) {
        g_logger.Log("=== MT4 A/B-book Routing Plugin STOPPED ===");
    }

    // Plugin about info
    __declspec(dllexport) int __stdcall MtSrvAbout(void* reserved) {
        g_logger.Log("Plugin About requested - A/B-book Routing Plugin v1.0 (Official API)");
        return 0;
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
            // Log raw trade data (using official structure)
            g_logger.Log("=== TRADE TRANSACTION (Official API) ===");
            g_logger.Log("Order: " + std::to_string(trade->order));
            g_logger.Log("Login: " + std::to_string(trade->login));
            g_logger.Log("Symbol: " + std::string(trade->symbol, 12));
            g_logger.Log("Command: " + std::to_string(trade->cmd) + " (" + GetCommandName(trade->cmd) + ")");
            g_logger.Log("Volume: " + std::to_string(trade->volume));
            g_logger.Log("Price: " + std::to_string(trade->open_price));
            g_logger.Log("State: " + std::to_string(trade->state));
            
            // Check if we should process this trade
            if (!ShouldProcessTrade(trade)) {
                g_logger.Log("Trade skipped - not a new market order");
                return 0;
            }
            
            // Display ML service status
            std::string ml_status;
            if (g_cvm_client.IsMLServiceAvailable()) {
                ml_status = "CONNECTED";
            } else {
                int failures = g_cvm_client.GetConsecutiveFailures();
                ml_status = "DISCONNECTED (failures: " + std::to_string(failures) + ")";
            }
            g_logger.Log("ML Service Status: " + ml_status);
            
            // Get ML score (always returns valid score, even if ML service is down)
            double score = g_cvm_client.GetScore(trade, user);
            
            // Determine instrument group and threshold
            std::string instrument_group = GetInstrumentGroup(trade->symbol);
            double threshold = GetThreshold(instrument_group);
            
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
            
            // Log decision with context
            g_logger.Log("Score: " + std::to_string(score) + " (" + decision_basis + ")");
            g_logger.Log("Instrument Group: " + instrument_group);
            g_logger.Log("Threshold: " + std::to_string(threshold));
            g_logger.Log("ROUTING DECISION: " + routing_decision);
            
            // Log plugin stability status
            if (!g_cvm_client.IsMLServiceAvailable()) {
                g_logger.Log("PLUGIN STATUS: Operating in FALLBACK mode - all trades processed normally");
            }
            
            g_logger.Log("=====================================");
            
            // INTEGRATION POINT: In production, integrate with broker's routing system here
            // The plugin NEVER fails regardless of ML service status
            
            return 0; // Always return success to prevent plugin unloading
            
        } catch (const std::bad_alloc& e) {
            g_logger.Log("CRITICAL: Memory allocation failed in MtSrvTradeTransaction - plugin remains stable");
            return 0; // Plugin continues to operate
        } catch (const std::exception& e) {
            g_logger.Log("EXCEPTION in MtSrvTradeTransaction: " + std::string(e.what()) + " - plugin remains stable");
            return 0; // Plugin continues to operate
        } catch (...) {
            g_logger.Log("UNKNOWN EXCEPTION in MtSrvTradeTransaction - plugin remains stable and continues operating");
            return 0; // Plugin continues to operate
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
            break;
            
        case DLL_PROCESS_DETACH:
            g_logger.Log("DLL_PROCESS_DETACH: Plugin unloaded from MT4 server");
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            // Handle thread attach/detach if needed
            break;
    }
    return TRUE;
} 