/*
 * MT4_ABBook_Plugin.cpp
 * Server-side C++ plugin for real-time scoring-based A/B-book routing
 * Enhanced with comprehensive logging for debugging
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

// MT Server integration structures
struct TradeRequest {
    int login;
    char symbol[16];
    int type;          // 0=buy, 1=sell
    double volume;
    double price;
    double sl;
    double tp;
    char comment[64];
};

struct TradeResult {
    int routing;       // 0=A-book, 1=B-book
    int retcode;       // 0=success
    char reason[128];
};

// Return codes
#define MT_RET_OK           0
#define MT_RET_ERROR        1

// Routing decisions
#define ROUTE_A_BOOK        0
#define ROUTE_B_BOOK        1

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
            auto age = std::chrono::steady_clock::now() - it->second.timestamp;
            if (age < ttl) {
                score = it->second.score;
                return true;
            }
            cache.erase(it); // Remove expired entry
        }
        return false;
    }
    
    void SetCachedScore(const std::string& key, float score) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        cache[key] = {score, std::chrono::steady_clock::now()};
        
        // Cleanup old entries if cache gets too large
        if (cache.size() > 1000) {
            auto now = std::chrono::steady_clock::now();
            auto it = cache.begin();
            while (it != cache.end()) {
                if (now - it->second.timestamp > ttl) {
                    it = cache.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    
    void SetTTL(int milliseconds) {
        ttl = std::chrono::milliseconds(milliseconds);
    }
};

// Global variables
static PluginConfig g_config;
static std::mutex g_config_mutex;
static ScoreCache g_score_cache;

// Generate cache key from scoring request
std::string GenerateRequestHash(const ScoringRequest& req) {
    std::stringstream ss;
    ss << req.user_id << "|" << req.symbol << "|" << req.lot_volume 
       << "|" << static_cast<int>(req.open_price * 100000) // Price to 5 decimals
       << "|" << req.deal_type << "|" << static_cast<int>(req.opening_balance);
    return ss.str();
}

// Simple CVM communication
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
        // Create socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            return (float)g_config.fallback_score;
        }
        
        // Set timeout
        DWORD timeout = g_config.connection_timeout;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        
        // Connect to CVM
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(g_config.cvm_port);
        inet_pton(AF_INET, g_config.cvm_ip, &addr.sin_addr);
        
        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
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
        
        // Send length-prefixed message
        uint32_t length = (uint32_t)json.length();
        send(sock, (char*)&length, sizeof(length), 0);
        send(sock, json.c_str(), (int)json.length(), 0);
        
        // Receive response
        uint32_t response_length;
        if (recv(sock, (char*)&response_length, sizeof(response_length), 0) <= 0) {
            closesocket(sock);
            return (float)g_config.fallback_score;
        }
        
        if (recv(sock, buffer, response_length, 0) <= 0) {
            closesocket(sock);
            return (float)g_config.fallback_score;
        }
        
        buffer[response_length] = '\0';
        closesocket(sock);
        
        // Parse JSON response (simplified)
        const char* score_start = strstr(buffer, "\"score\":");
        if (score_start) {
            return (float)atof(score_start + 8);
        }
        
        return (float)g_config.fallback_score;
    }
};

// Configuration management
bool LoadConfiguration() {
    g_logger.LogInfo("Starting configuration loading...");
    
    try {
        // Set defaults
        g_logger.LogInfo("Setting default configuration values...");
        strcpy_s(g_config.cvm_ip, "127.0.0.1");
        g_config.cvm_port = 8080;
        g_config.connection_timeout = 5000;
        g_config.fallback_score = 0.0;
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
        
        g_logger.LogInfo("Default configuration values set successfully");
        
        std::ifstream config_file("ABBook_Config.ini");
        if (!config_file.is_open()) {
            g_logger.LogWarning("Configuration file ABBook_Config.ini not found, using defaults");
            return true; // Use defaults
        }
        
        g_logger.LogInfo("Configuration file opened successfully");
        
        std::string line;
        int line_count = 0;
        while (std::getline(config_file, line)) {
            line_count++;
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') {
                continue;
            }
            
            g_logger.LogDebug("Processing config line " + std::to_string(line_count) + ": " + line);
            
            try {
                if (line.find("CVM_IP=") == 0) {
                    std::string ip = line.substr(7);
                    strcpy_s(g_config.cvm_ip, ip.c_str());
                    g_logger.LogInfo("CVM_IP set to: " + ip);
                } else if (line.find("CVM_Port=") == 0) {
                    g_config.cvm_port = std::stoi(line.substr(9));
                    g_logger.LogInfo("CVM_Port set to: " + std::to_string(g_config.cvm_port));
                } else if (line.find("ConnectionTimeout=") == 0) {
                    g_config.connection_timeout = std::stoi(line.substr(18));
                    g_logger.LogInfo("ConnectionTimeout set to: " + std::to_string(g_config.connection_timeout));
                } else if (line.find("FallbackScore=") == 0) {
                    g_config.fallback_score = std::stod(line.substr(14));
                    g_logger.LogInfo("FallbackScore set to: " + std::to_string(g_config.fallback_score));
                } else if (line.find("EnableCache=") == 0) {
                    g_config.enable_cache = (line.substr(12) == "true");
                    g_logger.LogInfo("EnableCache set to: " + std::string(g_config.enable_cache ? "true" : "false"));
                } else if (line.find("CacheTTL=") == 0) {
                    g_config.cache_ttl = std::stoi(line.substr(9));
                    g_logger.LogInfo("CacheTTL set to: " + std::to_string(g_config.cache_ttl));
                } else if (line.find("ForceABook=") == 0) {
                    g_config.force_a_book = (line.substr(11) == "true");
                    g_logger.LogInfo("ForceABook set to: " + std::string(g_config.force_a_book ? "true" : "false"));
                } else if (line.find("ForceBBook=") == 0) {
                    g_config.force_b_book = (line.substr(11) == "true");
                    g_logger.LogInfo("ForceBBook set to: " + std::string(g_config.force_b_book ? "true" : "false"));
                } else if (line.find("Threshold_FXMajors=") == 0) {
                    g_config.thresholds[0] = std::stod(line.substr(19));
                    g_logger.LogInfo("Threshold_FXMajors set to: " + std::to_string(g_config.thresholds[0]));
                } else if (line.find("Threshold_Crypto=") == 0) {
                    g_config.thresholds[1] = std::stod(line.substr(17));
                    g_logger.LogInfo("Threshold_Crypto set to: " + std::to_string(g_config.thresholds[1]));
                } else if (line.find("Threshold_Metals=") == 0) {
                    g_config.thresholds[2] = std::stod(line.substr(17));
                    g_logger.LogInfo("Threshold_Metals set to: " + std::to_string(g_config.thresholds[2]));
                } else if (line.find("Threshold_Energy=") == 0) {
                    g_config.thresholds[3] = std::stod(line.substr(17));
                    g_logger.LogInfo("Threshold_Energy set to: " + std::to_string(g_config.thresholds[3]));
                } else if (line.find("Threshold_Indices=") == 0) {
                    g_config.thresholds[4] = std::stod(line.substr(18));
                    g_logger.LogInfo("Threshold_Indices set to: " + std::to_string(g_config.thresholds[4]));
                } else if (line.find("Threshold_Other=") == 0) {
                    g_config.thresholds[5] = std::stod(line.substr(16));
                    g_logger.LogInfo("Threshold_Other set to: " + std::to_string(g_config.thresholds[5]));
                } else {
                    g_logger.LogWarning("Unknown configuration line: " + line);
                }
            } catch (const std::exception& e) {
                g_logger.LogError("Error parsing config line " + std::to_string(line_count) + ": " + e.what());
            }
        }
        
        g_logger.LogInfo("Configuration file parsed successfully, processed " + std::to_string(line_count) + " lines");
        
        // Configure cache with loaded settings
        g_score_cache.SetTTL(g_config.cache_ttl);
        g_logger.LogInfo("Cache configured with TTL: " + std::to_string(g_config.cache_ttl));
        
        // Log final configuration
        g_logger.LogInfo("Final configuration:");
        g_logger.LogInfo("  CVM_IP: " + std::string(g_config.cvm_ip));
        g_logger.LogInfo("  CVM_Port: " + std::to_string(g_config.cvm_port));
        g_logger.LogInfo("  Connection Timeout: " + std::to_string(g_config.connection_timeout));
        g_logger.LogInfo("  Fallback Score: " + std::to_string(g_config.fallback_score));
        g_logger.LogInfo("  Force A-Book: " + std::string(g_config.force_a_book ? "true" : "false"));
        g_logger.LogInfo("  Force B-Book: " + std::string(g_config.force_b_book ? "true" : "false"));
        g_logger.LogInfo("  Cache Enabled: " + std::string(g_config.enable_cache ? "true" : "false"));
        
        g_logger.LogInfo("Configuration loading completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        g_logger.LogError("Exception in LoadConfiguration: " + std::string(e.what()));
        return false;
    } catch (...) {
        g_logger.LogError("Unknown exception in LoadConfiguration");
        return false;
    }
}

// Utility functions
const char* GetInstrumentGroup(const char* symbol) {
    if (strstr(symbol, "EUR") || strstr(symbol, "GBP") || strstr(symbol, "USD") || 
        strstr(symbol, "JPY") || strstr(symbol, "CHF") || strstr(symbol, "AUD")) {
        return "FXMajors";
    } else if (strstr(symbol, "BTC") || strstr(symbol, "ETH") || strstr(symbol, "LTC")) {
        return "Crypto";
    } else if (strstr(symbol, "GOLD") || strstr(symbol, "XAU") || strstr(symbol, "SILVER")) {
        return "Metals";
    } else if (strstr(symbol, "OIL") || strstr(symbol, "WTI") || strstr(symbol, "BRENT")) {
        return "Energy";
    } else if (strstr(symbol, "SPX") || strstr(symbol, "NDX") || strstr(symbol, "DAX")) {
        return "Indices";
    }
    return "Other";
}

double GetThresholdForGroup(const char* group) {
    if (strcmp(group, "FXMajors") == 0) return g_config.thresholds[0];
    if (strcmp(group, "Crypto") == 0) return g_config.thresholds[1];
    if (strcmp(group, "Metals") == 0) return g_config.thresholds[2];
    if (strcmp(group, "Energy") == 0) return g_config.thresholds[3];
    if (strcmp(group, "Indices") == 0) return g_config.thresholds[4];
    return g_config.thresholds[5]; // Other
}

void BuildScoringRequest(const TradeRequest* trade, ScoringRequest* request) {
    // Basic trade data
    sprintf_s(request->user_id, "%d", trade->login);
    request->open_price = (float)trade->price;
    request->sl = (float)trade->sl;
    request->tp = (float)trade->tp;
    request->deal_type = (float)trade->type;
    request->lot_volume = (float)trade->volume;
    strcpy_s(request->symbol, trade->symbol);
    strcpy_s(request->inst_group, GetInstrumentGroup(trade->symbol));
    
    // Calculated fields
    request->has_sl = (trade->sl > 0) ? 1.0f : 0.0f;
    request->has_tp = (trade->tp > 0) ? 1.0f : 0.0f;
    
    // Account data (would come from MT server API)
    request->opening_balance = 10000.0f;    // Placeholder
    request->concurrent_positions = 3.0f;   // Placeholder
}

void LogDecision(const TradeRequest* trade, float score, double threshold, int routing) {
    std::ofstream log_file("ABBook_Plugin.log", std::ios::app);
    if (log_file.is_open()) {
        auto now = std::time(nullptr);
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        
        log_file << timestamp << " - ";
        log_file << "Login:" << trade->login << " ";
        log_file << "Symbol:" << trade->symbol << " ";
        log_file << "Score:" << score << " ";
        log_file << "Threshold:" << threshold << " ";
        log_file << "Decision:" << (routing == 0 ? "A-BOOK" : "B-BOOK") << std::endl;
        
        log_file.close();
    }
}

// Main trade processing function
int ProcessTradeRouting(const TradeRequest* trade, TradeResult* result) {
    try {
        // Check global overrides
        if (g_config.force_a_book) {
            result->routing = ROUTE_A_BOOK;
            result->retcode = MT_RET_OK;
            strcpy_s(result->reason, "FORCED_A_BOOK");
            LogDecision(trade, 0.0f, 0.0, 0);
            return MT_RET_OK;
        }
        
        if (g_config.force_b_book) {
            result->routing = ROUTE_B_BOOK;
            result->retcode = MT_RET_OK;
            strcpy_s(result->reason, "FORCED_B_BOOK");
            LogDecision(trade, 1.0f, 0.0, 1);
            return MT_RET_OK;
        }
        
        // Build scoring request
        ScoringRequest scoring_req;
        BuildScoringRequest(trade, &scoring_req);
        
        // Try to get cached score first (if caching is enabled)
        float score = (float)g_config.fallback_score;
        
        if (g_config.enable_cache) {
            std::string cache_key = GenerateRequestHash(scoring_req);
            if (!g_score_cache.GetCachedScore(cache_key, score)) {
                // Cache miss - get score from CVM
                CVMClient cvm_client;
                score = cvm_client.GetScore(scoring_req);
                
                // Cache the result for future requests
                g_score_cache.SetCachedScore(cache_key, score);
            }
        } else {
            // Caching disabled - get score directly
            CVMClient cvm_client;
            score = cvm_client.GetScore(scoring_req);
        }
        
        // Get threshold
        const char* inst_group = GetInstrumentGroup(trade->symbol);
        double threshold = GetThresholdForGroup(inst_group);
        
        // Make routing decision
        if (score >= threshold) {
            result->routing = ROUTE_B_BOOK;
            strcpy_s(result->reason, "SCORE_ABOVE_THRESHOLD");
        } else {
            result->routing = ROUTE_A_BOOK;
            strcpy_s(result->reason, "SCORE_BELOW_THRESHOLD");
        }
        
        result->retcode = MT_RET_OK;
        
        // Log decision
        LogDecision(trade, score, threshold, result->routing);
        
        return MT_RET_OK;
        
    } catch (...) {
        result->routing = ROUTE_A_BOOK; // Default to A-book on error
        result->retcode = MT_RET_ERROR;
        strcpy_s(result->reason, "ERROR_FALLBACK");
        return MT_RET_ERROR;
    }
}

// Plugin API exports
extern "C" {
    __declspec(dllexport) int __stdcall OnTradeRequest(TradeRequest* request, TradeResult* result, void* server_context) {
        g_logger.LogInfo("=== OnTradeRequest() called ===");
        
        try {
            // Validate input parameters
            if (!request) {
                g_logger.LogError("OnTradeRequest: request parameter is NULL");
                return MT_RET_ERROR;
            }
            
            if (!result) {
                g_logger.LogError("OnTradeRequest: result parameter is NULL");
                return MT_RET_ERROR;
            }
            
            // Log trade request details
            g_logger.LogInfo("Trade request details:");
            g_logger.LogInfo("  Login: " + std::to_string(request->login));
            g_logger.LogInfo("  Symbol: " + std::string(request->symbol));
            g_logger.LogInfo("  Type: " + std::string(request->type == 0 ? "BUY" : "SELL"));
            g_logger.LogInfo("  Volume: " + std::to_string(request->volume));
            g_logger.LogInfo("  Price: " + std::to_string(request->price));
            g_logger.LogInfo("  SL: " + std::to_string(request->sl));
            g_logger.LogInfo("  TP: " + std::to_string(request->tp));
            g_logger.LogInfo("  Comment: " + std::string(request->comment));
            
            // Log server context
            g_logger.LogInfo("Server context: " + std::to_string(reinterpret_cast<uintptr_t>(server_context)));
            
            // Process the trade
            int routing_result = ProcessTradeRouting(request, result);
            
            // Log result
            g_logger.LogInfo("Trade routing result:");
            g_logger.LogInfo("  Routing: " + std::string(result->routing == ROUTE_A_BOOK ? "A-BOOK" : "B-BOOK"));
            g_logger.LogInfo("  Return code: " + std::to_string(result->retcode));
            g_logger.LogInfo("  Reason: " + std::string(result->reason));
            
            g_logger.LogInfo("OnTradeRequest completed with result: " + std::to_string(routing_result));
            return routing_result;
            
        } catch (const std::exception& e) {
            g_logger.LogError("Exception in OnTradeRequest: " + std::string(e.what()));
            if (result) {
                result->routing = ROUTE_A_BOOK;
                result->retcode = MT_RET_ERROR;
                strcpy_s(result->reason, "EXCEPTION_ERROR");
            }
            return MT_RET_ERROR;
        } catch (...) {
            g_logger.LogError("Unknown exception in OnTradeRequest");
            if (result) {
                result->routing = ROUTE_A_BOOK;
                result->retcode = MT_RET_ERROR;
                strcpy_s(result->reason, "UNKNOWN_ERROR");
            }
            return MT_RET_ERROR;
        }
    }
    
    // Trade close handler
    __declspec(dllexport) int __stdcall OnTradeClose(int login, int ticket, double volume, double price) {
        g_logger.LogInfo("=== OnTradeClose() called ===");
        
        try {
            // Log trade close details
            g_logger.LogInfo("Trade close details:");
            g_logger.LogInfo("  Login: " + std::to_string(login));
            g_logger.LogInfo("  Ticket: " + std::to_string(ticket));
            g_logger.LogInfo("  Volume: " + std::to_string(volume));
            g_logger.LogInfo("  Price: " + std::to_string(price));
            
            // Also log to the original log file for compatibility
            std::ofstream log_file("ABBook_Plugin.log", std::ios::app);
            if (log_file.is_open()) {
                auto now = std::time(nullptr);
                char timestamp[32];
                std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
                
                log_file << timestamp << " - CLOSE: Login:" << login << " Ticket:" << ticket << std::endl;
                log_file.close();
                g_logger.LogInfo("Trade close logged to ABBook_Plugin.log");
            } else {
                g_logger.LogError("Failed to write to ABBook_Plugin.log");
            }
            
            g_logger.LogInfo("OnTradeClose completed successfully");
            return MT_RET_OK;
            
        } catch (const std::exception& e) {
            g_logger.LogError("Exception in OnTradeClose: " + std::string(e.what()));
            return MT_RET_ERROR;
        } catch (...) {
            g_logger.LogError("Unknown exception in OnTradeClose");
            return MT_RET_ERROR;
        }
    }
    
    // Configuration reload
    __declspec(dllexport) void __stdcall OnConfigUpdate() {
        g_logger.LogInfo("=== OnConfigUpdate() called ===");
        
        try {
            std::lock_guard<std::mutex> lock(g_config_mutex);
            g_logger.LogInfo("Configuration mutex acquired");
            
            g_logger.LogInfo("Reloading configuration...");
            if (LoadConfiguration()) {
                g_logger.LogInfo("Configuration reloaded successfully");
            } else {
                g_logger.LogError("Configuration reload failed");
            }
            
            g_logger.LogInfo("OnConfigUpdate completed");
            
        } catch (const std::exception& e) {
            g_logger.LogError("Exception in OnConfigUpdate: " + std::string(e.what()));
        } catch (...) {
            g_logger.LogError("Unknown exception in OnConfigUpdate");
        }
    }
    
    // Plugin initialization
    __declspec(dllexport) int __stdcall PluginInit() {
        g_logger.LogInfo("=== PluginInit() called ===");
        
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
            
            // Test basic functionality
            g_logger.LogInfo("Testing basic plugin functionality...");
            
            // Test file writing permissions
            std::ofstream test_file("ABBook_Plugin.log", std::ios::app);
            if (test_file.is_open()) {
                auto now = std::time(nullptr);
                char timestamp[32];
                std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
                test_file << timestamp << " - Plugin initialized with enhanced logging" << std::endl;
                test_file.close();
                g_logger.LogInfo("Log file write test successful");
            } else {
                g_logger.LogError("Failed to write to log file");
            }
            
            // Test socket creation
            g_logger.LogInfo("Testing socket creation...");
            SOCKET test_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (test_sock == INVALID_SOCKET) {
                g_logger.LogSocketError("Test socket creation");
            } else {
                g_logger.LogInfo("Socket creation test successful");
                closesocket(test_sock);
            }
            
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
            g_logger.LogInfo("=== PluginInit() completed with success ===");
            
            return 0; // Success
            
        } catch (const std::exception& e) {
            g_logger.LogError("Exception in PluginInit: " + std::string(e.what()));
            return 1;
        } catch (...) {
            g_logger.LogError("Unknown exception in PluginInit");
            return 1;
        }
    }
    
    // Plugin cleanup
    __declspec(dllexport) void __stdcall PluginCleanup() {
        g_logger.LogInfo("=== PluginCleanup() called ===");
        
        try {
            g_logger.LogInfo("Cleaning up Winsock...");
            WSACleanup();
            g_logger.LogInfo("Winsock cleanup completed");
            
            g_logger.LogInfo("Plugin cleanup completed successfully");
            
        } catch (const std::exception& e) {
            g_logger.LogError("Exception in PluginCleanup: " + std::string(e.what()));
        } catch (...) {
            g_logger.LogError("Unknown exception in PluginCleanup");
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