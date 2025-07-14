/*
 * MT4_ABBook_Plugin.cpp
 * Server-side C++ plugin for real-time scoring-based A/B-book routing
 * Updated with MT4-compatible function signatures
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>
#include <atomic>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

// Enhanced logging system
class PluginLogger {
private:
    std::mutex log_mutex;
    std::string log_file;
    
public:
    PluginLogger() : log_file("ABBook_Plugin_Debug.log") {
        // Clear log file on startup
        std::ofstream file(log_file, std::ios::trunc);
        if (file.is_open()) {
            file << "=== ABBook Plugin Debug Log Started ===" << std::endl;
            file << "Timestamp: " << GetTimestamp() << std::endl;
            file << "============================================" << std::endl;
            file.close();
        }
    }
    
    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
            
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
        
        std::ostringstream oss;
        oss << buffer << "." << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
    
    void Log(const std::string& level, const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream file(log_file, std::ios::app);
        if (file.is_open()) {
            file << "[" << GetTimestamp() << "] [" << level << "] " << message << std::endl;
            file.close();
        }
        
        // Also log to console if available
        std::cout << "[" << GetTimestamp() << "] [" << level << "] " << message << std::endl;
    }
    
    void LogError(const std::string& message) { Log("ERROR", message); }
    void LogWarning(const std::string& message) { Log("WARN", message); }
    void LogInfo(const std::string& message) { Log("INFO", message); }
    void LogDebug(const std::string& message) { Log("DEBUG", message); }
    
    void LogWinError(const std::string& operation) {
        DWORD error = GetLastError();
        char* errorMsg = nullptr;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, error, 0, (LPSTR)&errorMsg, 0, NULL);
        
        std::string msg = operation + " failed with error " + std::to_string(error);
        if (errorMsg) {
            msg += ": " + std::string(errorMsg);
            LocalFree(errorMsg);
        }
        LogError(msg);
    }
    
    void LogSocketError(const std::string& operation) {
        int error = WSAGetLastError();
        std::string msg = operation + " failed with WSA error " + std::to_string(error);
        LogError(msg);
    }
};

// Global logger instance
PluginLogger g_logger;

// MT4-compatible structures based on MT4ManagerAPI.h patterns
struct MT4TradeRecord {
    int order;           // order ticket
    int login;           // client login
    char symbol[12];     // security
    int digits;          // security precision
    int cmd;             // trade command
    int volume;          // volume (in lots*100)
    __time32_t open_time; // open time
    int state;           // reserved
    double open_price;   // open price
    double sl, tp;       // stop loss and take profit
    __time32_t close_time; // close time
    int gw_volume;       // gateway volume
    __time32_t expiration; // pending order expiration time
    char reason;         // trade reason
    char conv_rates[2];  // currency conversion rates
    double commission;   // commission
    double commission_agent; // agent commission
    double storage;      // order swaps
    double close_price;  // close price
    double profit;       // profit
    double taxes;        // taxes
    int magic;           // special value used by client experts
    char comment[32];    // comment
    int gw_order;        // gateway order ticket
    int activation;      // used by MT Manager
    short gw_open_price; // gateway open price (relative)
    short gw_close_price; // gateway close price (relative)
    int margin_rate;     // margin conversion rate
    __time32_t timestamp; // timestamp
    int api_data[4];     // for API usage
};

struct MT4UserRecord {
    int login;           // login
    char group[16];      // group
    char password[16];   // password
    int enable;          // enable
    int enable_change_password; // allow to change password
    int enable_read_only; // allow to open/close orders
    char name[128];      // name
    char country[32];    // country
    char city[32];       // city
    char state[32];      // state
    char zipcode[16];    // zipcode
    char address[128];   // address
    char phone[32];      // phone
    char email[64];      // email
    char comment[64];    // comment
    char id[32];         // SSN
    char status[16];     // status
    __time32_t regdate; // registration date
    __time32_t lastdate; // last visit date
    int leverage;        // leverage
    int agent_account;   // agent account
    __time32_t timestamp; // timestamp
    double balance;      // balance
    double prevmonthbalance; // previous month balance
    double prevbalance;  // previous balance
    double credit;       // credit
    double interestrate; // accumulated interest rate
    double taxes;        // taxes
    double prevmonthequity; // previous month equity
    double prevequity;   // previous equity
    int reserved2[2];    // reserved fields
    char publickey[270]; // rsa public key
    int reserved[7];     // reserved fields
};

// Plugin configuration
struct PluginConfig {
    char cvm_ip[64];
    int cvm_port;
    int connection_timeout;
    double fallback_score;
    bool force_a_book;
    bool force_b_book;
    double thresholds[6];  // FXMajors, Crypto, Metals, Energy, Indices, Other
    // Cache settings
    bool enable_cache;
    int cache_ttl;
    int max_cache_size;
};

// Global configuration
PluginConfig g_config;
std::mutex g_config_mutex;

// Simplified scoring request
struct ScoringRequest {
    char user_id[32];
    float open_price;
    float sl;
    float tp;
    float deal_type;
    float lot_volume;
    float opening_balance;
    float concurrent_positions;
    float has_sl;
    float has_tp;
    char symbol[16];
    char inst_group[32];
};

// Score caching system for high-frequency trading
struct CachedScore {
    float score;
    std::chrono::steady_clock::time_point timestamp;
};

class ScoreCache {
private:
    std::unordered_map<std::string, CachedScore> cache;
    std::mutex cache_mutex;
    std::chrono::milliseconds ttl{300}; // 300ms TTL
    
public:
    bool GetCachedScore(const std::string& key, float& score) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = cache.find(key);
        if (it != cache.end()) {
            auto now = std::chrono::steady_clock::now();
            if (now - it->second.timestamp < ttl) {
                score = it->second.score;
                return true;
            } else {
                cache.erase(it);
            }
        }
        return false;
    }
    
    void SetCachedScore(const std::string& key, float score) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        cache[key] = {score, std::chrono::steady_clock::now()};
        
        // Cleanup expired entries periodically
        if (cache.size() > 1000) {
            auto now = std::chrono::steady_clock::now();
            auto it = cache.begin();
            while (it != cache.end()) {
                if (now - it->second.timestamp >= ttl) {
                    it = cache.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    
    void SetTTL(int milliseconds) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        ttl = std::chrono::milliseconds(milliseconds);
    }
};

// Global cache instance
ScoreCache g_score_cache;

// Generate hash for caching
std::string GenerateRequestHash(const ScoringRequest& req) {
    std::string hash = std::string(req.user_id) + "_" + 
                      std::string(req.symbol) + "_" + 
                      std::to_string(req.open_price) + "_" + 
                      std::to_string(req.lot_volume);
    return hash;
}

class CVMClient {
private:
    SOCKET sock;
    char buffer[8192];
    
public:
    CVMClient() : sock(INVALID_SOCKET) {}
    
    ~CVMClient() {
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
        }
    }
    
    float GetScore(const ScoringRequest& request) {
        g_logger.LogInfo("CVMClient::GetScore() called");
        
        // Check cache first
        if (g_config.enable_cache) {
            std::string cache_key = GenerateRequestHash(request);
            float cached_score;
            if (g_score_cache.GetCachedScore(cache_key, cached_score)) {
                g_logger.LogInfo("Using cached score: " + std::to_string(cached_score));
                return cached_score;
            }
        }
        
        // Create socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            g_logger.LogSocketError("Socket creation");
            return (float)g_config.fallback_score;
        }
        
        // Set timeout
        DWORD timeout = g_config.connection_timeout;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
        
        // Connect to CVM
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(g_config.cvm_port);
        inet_pton(AF_INET, g_config.cvm_ip, &addr.sin_addr);
        
        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
            g_logger.LogSocketError("Connection to CVM");
            closesocket(sock);
            return (float)g_config.fallback_score;
        }
        
        // Serialize request to JSON (simplified)
        std::string json = "{";
        json += "\"user_id\":\"" + std::string(request.user_id) + "\",";
        json += "\"open_price\":" + std::to_string(request.open_price) + ",";
        json += "\"sl\":" + std::to_string(request.sl) + ",";
        json += "\"tp\":" + std::to_string(request.tp) + ",";
        json += "\"deal_type\":" + std::to_string(request.deal_type) + ",";
        json += "\"lot_volume\":" + std::to_string(request.lot_volume) + ",";
        json += "\"opening_balance\":" + std::to_string(request.opening_balance) + ",";
        json += "\"concurrent_positions\":" + std::to_string(request.concurrent_positions) + ",";
        json += "\"has_sl\":" + std::to_string(request.has_sl) + ",";
        json += "\"has_tp\":" + std::to_string(request.has_tp) + ",";
        json += "\"symbol\":\"" + std::string(request.symbol) + "\",";
        json += "\"inst_group\":\"" + std::string(request.inst_group) + "\"";
        json += "}";
        
        g_logger.LogInfo("Sending request to CVM: " + json);
        
        // Send length-prefixed message
        uint32_t length = json.length();
        if (send(sock, (char*)&length, sizeof(length), 0) != sizeof(length)) {
            g_logger.LogSocketError("Send length");
            closesocket(sock);
            return (float)g_config.fallback_score;
        }
        
        if (send(sock, json.c_str(), length, 0) != (int)length) {
            g_logger.LogSocketError("Send data");
            closesocket(sock);
            return (float)g_config.fallback_score;
        }
        
        // Receive response
        uint32_t response_length;
        if (recv(sock, (char*)&response_length, sizeof(response_length), 0) != sizeof(response_length)) {
            g_logger.LogSocketError("Receive length");
            closesocket(sock);
            return (float)g_config.fallback_score;
        }
        
        if (response_length > sizeof(buffer) - 1) {
            g_logger.LogError("Response too large: " + std::to_string(response_length));
            closesocket(sock);
            return (float)g_config.fallback_score;
        }
        
        if (recv(sock, buffer, response_length, 0) != (int)response_length) {
            g_logger.LogSocketError("Receive data");
            closesocket(sock);
            return (float)g_config.fallback_score;
        }
        
        buffer[response_length] = '\0';
        closesocket(sock);
        
        // Parse response (simplified JSON parsing)
        std::string response(buffer);
        g_logger.LogInfo("Received response: " + response);
        
        // Extract score from JSON response
        size_t score_pos = response.find("\"score\":");
        if (score_pos != std::string::npos) {
            size_t start = response.find(":", score_pos) + 1;
            size_t end = response.find(",", start);
            if (end == std::string::npos) end = response.find("}", start);
            
            if (start != std::string::npos && end != std::string::npos) {
                std::string score_str = response.substr(start, end - start);
                float score = std::stof(score_str);
                
                // Cache the score
                if (g_config.enable_cache) {
                    std::string cache_key = GenerateRequestHash(request);
                    g_score_cache.SetCachedScore(cache_key, score);
                }
                
                g_logger.LogInfo("Parsed score: " + std::to_string(score));
                return score;
            }
        }
        
        g_logger.LogError("Failed to parse score from response");
        return (float)g_config.fallback_score;
    }
};

// Configuration loading
bool LoadConfiguration() {
    g_logger.LogInfo("Loading configuration from ABBook_Config.ini");
    
    std::lock_guard<std::mutex> lock(g_config_mutex);
    
    // Default values
    strcpy_s(g_config.cvm_ip, "127.0.0.1");
    g_config.cvm_port = 8080;
    g_config.connection_timeout = 5000;
    g_config.fallback_score = 0.05;
    g_config.force_a_book = false;
    g_config.force_b_book = false;
    g_config.enable_cache = true;
    g_config.cache_ttl = 300;
    g_config.max_cache_size = 1000;
    
    // Default thresholds
    g_config.thresholds[0] = 0.08; // FXMajors
    g_config.thresholds[1] = 0.12; // Crypto
    g_config.thresholds[2] = 0.06; // Metals
    g_config.thresholds[3] = 0.10; // Energy
    g_config.thresholds[4] = 0.07; // Indices
    g_config.thresholds[5] = 0.05; // Other
    
    std::ifstream config_file("ABBook_Config.ini");
    if (!config_file.is_open()) {
        g_logger.LogError("Could not open ABBook_Config.ini");
        return false;
    }
    
    std::string line;
    int line_number = 0;
    
    while (std::getline(config_file, line)) {
        line_number++;
        g_logger.LogInfo("Processing line " + std::to_string(line_number) + ": " + line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') {
            continue;
        }
        
        // Parse key=value pairs
        size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, equals_pos);
        std::string value = line.substr(equals_pos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        g_logger.LogInfo("Config: " + key + " = " + value);
        
        // Process configuration values
        if (key == "CVM_IP") {
            strcpy_s(g_config.cvm_ip, value.c_str());
        } else if (key == "CVM_Port") {
            g_config.cvm_port = std::stoi(value);
        } else if (key == "ConnectionTimeout") {
            g_config.connection_timeout = std::stoi(value);
        } else if (key == "FallbackScore") {
            g_config.fallback_score = std::stod(value);
        } else if (key == "ForceABook") {
            g_config.force_a_book = (value == "true" || value == "1");
        } else if (key == "ForceBBook") {
            g_config.force_b_book = (value == "true" || value == "1");
        } else if (key == "EnableCache") {
            g_config.enable_cache = (value == "true" || value == "1");
        } else if (key == "CacheTTL") {
            g_config.cache_ttl = std::stoi(value);
            g_score_cache.SetTTL(g_config.cache_ttl);
        } else if (key == "MaxCacheSize") {
            g_config.max_cache_size = std::stoi(value);
        } else if (key == "Threshold_FXMajors") {
            g_config.thresholds[0] = std::stod(value);
        } else if (key == "Threshold_Crypto") {
            g_config.thresholds[1] = std::stod(value);
        } else if (key == "Threshold_Metals") {
            g_config.thresholds[2] = std::stod(value);
        } else if (key == "Threshold_Energy") {
            g_config.thresholds[3] = std::stod(value);
        } else if (key == "Threshold_Indices") {
            g_config.thresholds[4] = std::stod(value);
        } else if (key == "Threshold_Other") {
            g_config.thresholds[5] = std::stod(value);
        }
    }
    
    config_file.close();
    
    g_logger.LogInfo("Configuration loaded successfully");
    g_logger.LogInfo("CVM_IP: " + std::string(g_config.cvm_ip));
    g_logger.LogInfo("CVM_Port: " + std::to_string(g_config.cvm_port));
    g_logger.LogInfo("ConnectionTimeout: " + std::to_string(g_config.connection_timeout));
    g_logger.LogInfo("FallbackScore: " + std::to_string(g_config.fallback_score));
    g_logger.LogInfo("EnableCache: " + std::string(g_config.enable_cache ? "true" : "false"));
    
    return true;
}

double GetThresholdForGroup(const char* group) {
    if (strstr(group, "FXMajors") || strstr(group, "EURUSD") || strstr(group, "GBPUSD")) {
        return g_config.thresholds[0];
    } else if (strstr(group, "Crypto") || strstr(group, "BTC") || strstr(group, "ETH")) {
        return g_config.thresholds[1];
    } else if (strstr(group, "Metals") || strstr(group, "Gold") || strstr(group, "Silver")) {
        return g_config.thresholds[2];
    } else if (strstr(group, "Energy") || strstr(group, "Oil") || strstr(group, "WTI")) {
        return g_config.thresholds[3];
    } else if (strstr(group, "Indices") || strstr(group, "SPX") || strstr(group, "NDX")) {
        return g_config.thresholds[4];
    } else {
        return g_config.thresholds[5]; // Other
    }
}

void BuildScoringRequest(const MT4TradeRecord* trade, const MT4UserRecord* user, ScoringRequest* request) {
    g_logger.LogInfo("Building scoring request for trade");
    
    // User ID
    sprintf_s(request->user_id, "%d", trade->login);
    
    // Trade data
    request->open_price = (float)trade->open_price;
    request->sl = (float)trade->sl;
    request->tp = (float)trade->tp;
    request->deal_type = (float)trade->cmd;
    request->lot_volume = (float)trade->volume / 100.0f; // Convert to lots
    
    // User data
    request->opening_balance = (float)user->balance;
    request->concurrent_positions = 1; // Simplified
    request->has_sl = (trade->sl > 0) ? 1.0f : 0.0f;
    request->has_tp = (trade->tp > 0) ? 1.0f : 0.0f;
    
    // Symbol and group
    strcpy_s(request->symbol, trade->symbol);
    strcpy_s(request->inst_group, user->group);
    
    g_logger.LogInfo("Scoring request built successfully");
}

void LogDecision(const MT4TradeRecord* trade, float score, double threshold, const char* routing) {
    g_logger.LogInfo("=== ROUTING DECISION ===");
    g_logger.LogInfo("Login: " + std::to_string(trade->login));
    g_logger.LogInfo("Symbol: " + std::string(trade->symbol));
    g_logger.LogInfo("Volume: " + std::to_string(trade->volume));
    g_logger.LogInfo("Price: " + std::to_string(trade->open_price));
    g_logger.LogInfo("Score: " + std::to_string(score));
    g_logger.LogInfo("Threshold: " + std::to_string(threshold));
    g_logger.LogInfo("Routing: " + std::string(routing));
    g_logger.LogInfo("========================");
    
    // Also log to the original log file for compatibility
    std::ofstream log_file("ABBook_Plugin.log", std::ios::app);
    if (log_file.is_open()) {
        auto now = std::time(nullptr);
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        
        log_file << timestamp << " - Login:" << trade->login << " Symbol:" << trade->symbol 
                << " Score:" << score << " Threshold:" << threshold << " Decision:" << routing << std::endl;
        log_file.close();
    }
}

// MT4-Compatible Plugin Interface
// Based on common MT4 plugin patterns
extern "C" {
    // Plugin info structure
    struct PluginInfo {
        int version;
        char name[64];
        char copyright[128];
        char web[128];
        char email[64];
    };
    
    // Plugin initialization
    __declspec(dllexport) int __stdcall MtSrvStartup(void* server_interface) {
        g_logger.LogInfo("=== MtSrvStartup() called ===");
        
        try {
            // Initialize Winsock
            g_logger.LogInfo("Initializing Winsock...");
            WSADATA wsaData;
            int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (wsa_result != 0) {
                g_logger.LogError("WSAStartup failed with error: " + std::to_string(wsa_result));
                g_logger.LogWinError("WSAStartup");
                return 1; // Failed
            }
            
            g_logger.LogInfo("Winsock initialized successfully");
            g_logger.LogInfo("Winsock version: " + std::to_string(wsaData.wVersion));
            g_logger.LogInfo("Winsock high version: " + std::to_string(wsaData.wHighVersion));
            
            // Load configuration
            g_logger.LogInfo("Loading plugin configuration...");
            if (!LoadConfiguration()) {
                g_logger.LogError("Configuration loading failed");
                return 1; // Failed
            }
            
            g_logger.LogInfo("Configuration loaded successfully");
            
            // Log system information
            g_logger.LogInfo("System information:");
            char computer_name[256];
            DWORD size = sizeof(computer_name);
            if (GetComputerNameA(computer_name, &size)) {
                g_logger.LogInfo("Computer name: " + std::string(computer_name));
            }
            
            // Log process information
            g_logger.LogInfo("Process ID: " + std::to_string(GetCurrentProcessId()));
            g_logger.LogInfo("Thread ID: " + std::to_string(GetCurrentThreadId()));
            
            // Log working directory
            char working_dir[MAX_PATH];
            if (GetCurrentDirectoryA(MAX_PATH, working_dir)) {
                g_logger.LogInfo("Working directory: " + std::string(working_dir));
            }
            
            // Log plugin module information
            HMODULE hModule = GetModuleHandleA("ABBook_Plugin_32bit.dll");
            if (hModule) {
                char module_path[MAX_PATH];
                if (GetModuleFileNameA(hModule, module_path, MAX_PATH)) {
                    g_logger.LogInfo("Plugin module path: " + std::string(module_path));
                }
            }
            
            g_logger.LogInfo("Plugin initialization completed successfully");
            g_logger.LogInfo("=== MtSrvStartup() completed with success ===");
            
            return 0; // Success
            
        } catch (const std::exception& e) {
            g_logger.LogError("Exception in MtSrvStartup: " + std::string(e.what()));
            return 1;
        } catch (...) {
            g_logger.LogError("Unknown exception in MtSrvStartup");
            return 1;
        }
    }
    
    // Plugin cleanup
    __declspec(dllexport) void __stdcall MtSrvCleanup() {
        g_logger.LogInfo("=== MtSrvCleanup() called ===");
        
        try {
            g_logger.LogInfo("Cleaning up Winsock...");
            WSACleanup();
            g_logger.LogInfo("Winsock cleanup completed");
            
            g_logger.LogInfo("Plugin cleanup completed successfully");
            
        } catch (const std::exception& e) {
            g_logger.LogError("Exception in MtSrvCleanup: " + std::string(e.what()));
        } catch (...) {
            g_logger.LogError("Unknown exception in MtSrvCleanup");
        }
    }
    
    // Plugin information
    __declspec(dllexport) PluginInfo* __stdcall MtSrvAbout() {
        static PluginInfo info = {
            100,                                    // version
            "ABBook Router v1.0",                 // name
            "Copyright 2024 ABBook Systems",      // copyright
            "https://github.com/abbook/plugin",   // web
            "support@abbook.com"                  // email
        };
        return &info;
    }
    
    // Trade processing hook (if supported)
    __declspec(dllexport) int __stdcall MtSrvTradeTransaction(MT4TradeRecord* trade, MT4UserRecord* user) {
        g_logger.LogInfo("=== MtSrvTradeTransaction() called ===");
        
        try {
            if (!trade || !user) {
                g_logger.LogError("Invalid parameters: trade or user is NULL");
                return 0; // Continue processing
            }
            
            g_logger.LogInfo("Processing trade transaction:");
            g_logger.LogInfo("  Order: " + std::to_string(trade->order));
            g_logger.LogInfo("  Login: " + std::to_string(trade->login));
            g_logger.LogInfo("  Symbol: " + std::string(trade->symbol));
            g_logger.LogInfo("  Command: " + std::to_string(trade->cmd));
            g_logger.LogInfo("  Volume: " + std::to_string(trade->volume));
            g_logger.LogInfo("  Price: " + std::to_string(trade->open_price));
            g_logger.LogInfo("  User Group: " + std::string(user->group));
            g_logger.LogInfo("  User Balance: " + std::to_string(user->balance));
            
            // Check for override flags
            if (g_config.force_a_book) {
                g_logger.LogInfo("Force A-book enabled - routing to A-book");
                LogDecision(trade, 0.0f, 0.0f, "A-BOOK (FORCED)");
                return 0; // Continue processing
            }
            
            if (g_config.force_b_book) {
                g_logger.LogInfo("Force B-book enabled - routing to B-book");
                LogDecision(trade, 1.0f, 0.0f, "B-BOOK (FORCED)");
                return 0; // Continue processing
            }
            
            // Build scoring request
            ScoringRequest request;
            BuildScoringRequest(trade, user, &request);
            
            // Get score from CVM
            CVMClient cvm_client;
            float score = cvm_client.GetScore(request);
            
            // Get threshold for this instrument group
            double threshold = GetThresholdForGroup(user->group);
            
            // Make routing decision
            if (score < threshold) {
                LogDecision(trade, score, threshold, "A-BOOK");
                // Route to A-book (external liquidity)
                // NOTE: Actual routing implementation depends on broker's specific API
            } else {
                LogDecision(trade, score, threshold, "B-BOOK");
                // Route to B-book (internal)
                // NOTE: Actual routing implementation depends on broker's specific API
            }
            
            g_logger.LogInfo("Trade transaction processed successfully");
            return 0; // Continue processing
            
        } catch (const std::exception& e) {
            g_logger.LogError("Exception in MtSrvTradeTransaction: " + std::string(e.what()));
            return 0; // Continue processing
        } catch (...) {
            g_logger.LogError("Unknown exception in MtSrvTradeTransaction");
            return 0; // Continue processing
        }
    }
    
    // Configuration update hook
    __declspec(dllexport) void __stdcall MtSrvConfigUpdate() {
        g_logger.LogInfo("=== MtSrvConfigUpdate() called ===");
        
        try {
            g_logger.LogInfo("Reloading configuration...");
            if (LoadConfiguration()) {
                g_logger.LogInfo("Configuration reloaded successfully");
            } else {
                g_logger.LogError("Configuration reload failed");
            }
            
        } catch (const std::exception& e) {
            g_logger.LogError("Exception in MtSrvConfigUpdate: " + std::string(e.what()));
        } catch (...) {
            g_logger.LogError("Unknown exception in MtSrvConfigUpdate");
        }
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    try {
        switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            {
                g_logger.LogInfo("=== DLL_PROCESS_ATTACH ===");
                g_logger.LogInfo("DLL is being loaded into process");
                g_logger.LogInfo("Module handle: " + std::to_string(reinterpret_cast<uintptr_t>(hModule)));
                
                // Log process information
                g_logger.LogInfo("Process ID: " + std::to_string(GetCurrentProcessId()));
                g_logger.LogInfo("Thread ID: " + std::to_string(GetCurrentThreadId()));
                
                // Get process name
                char process_name[MAX_PATH];
                if (GetModuleFileNameA(NULL, process_name, MAX_PATH)) {
                    g_logger.LogInfo("Process name: " + std::string(process_name));
                }
                
                // Get DLL path
                char dll_path[MAX_PATH];
                if (GetModuleFileNameA(hModule, dll_path, MAX_PATH)) {
                    g_logger.LogInfo("DLL path: " + std::string(dll_path));
                }
                
                // Log if this is called from MT4 server
                std::string proc_name = process_name;
                if (proc_name.find("terminal") != std::string::npos || 
                    proc_name.find("mt4") != std::string::npos ||
                    proc_name.find("mt5") != std::string::npos) {
                    g_logger.LogInfo("Detected MT4/MT5 process");
                }
                
                g_logger.LogInfo("DLL_PROCESS_ATTACH completed successfully");
                break;
            }
            
        case DLL_THREAD_ATTACH:
            g_logger.LogDebug("DLL_THREAD_ATTACH - Thread ID: " + std::to_string(GetCurrentThreadId()));
            break;
            
        case DLL_THREAD_DETACH:
            g_logger.LogDebug("DLL_THREAD_DETACH - Thread ID: " + std::to_string(GetCurrentThreadId()));
            break;
            
        case DLL_PROCESS_DETACH:
            {
                g_logger.LogInfo("=== DLL_PROCESS_DETACH ===");
                g_logger.LogInfo("DLL is being unloaded from process");
                g_logger.LogInfo("Process ID: " + std::to_string(GetCurrentProcessId()));
                
                if (lpReserved != NULL) {
                    g_logger.LogInfo("Process is terminating");
                } else {
                    g_logger.LogInfo("DLL is being unloaded via FreeLibrary");
                }
                
                g_logger.LogInfo("DLL_PROCESS_DETACH completed");
                break;
            }
        }
        
        return TRUE;
        
    } catch (const std::exception& e) {
        // Cannot use logger here as it might not be initialized
        std::ofstream emergency_log("ABBook_Plugin_Emergency.log", std::ios::app);
        if (emergency_log.is_open()) {
            emergency_log << "DllMain exception: " << e.what() << std::endl;
            emergency_log.close();
        }
        return FALSE;
    } catch (...) {
        // Cannot use logger here as it might not be initialized
        std::ofstream emergency_log("ABBook_Plugin_Emergency.log", std::ios::app);
        if (emergency_log.is_open()) {
            emergency_log << "DllMain unknown exception" << std::endl;
            emergency_log.close();
        }
        return FALSE;
    }
} 