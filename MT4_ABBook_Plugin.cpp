/*
 * MT4_ABBook_Plugin.cpp
 * Server-side C++ plugin for real-time scoring-based A/B-book routing
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

#pragma comment(lib, "ws2_32.lib")

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
    // Set defaults
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
    
    std::ifstream config_file("ABBook_Config.ini");
    if (!config_file.is_open()) {
        return true; // Use defaults
    }
    
    std::string line;
    while (std::getline(config_file, line)) {
        if (line.find("CVM_IP=") == 0) {
            strcpy_s(g_config.cvm_ip, line.substr(7).c_str());
        } else if (line.find("CVM_Port=") == 0) {
            g_config.cvm_port = std::stoi(line.substr(9));
        } else if (line.find("ConnectionTimeout=") == 0) {
            g_config.connection_timeout = std::stoi(line.substr(18));
        } else if (line.find("FallbackScore=") == 0) {
            g_config.fallback_score = std::stod(line.substr(14));
        } else if (line.find("EnableCache=") == 0) {
            g_config.enable_cache = (line.substr(12) == "true");
        } else if (line.find("CacheTTL=") == 0) {
            g_config.cache_ttl = std::stoi(line.substr(9));
        } else if (line.find("ForceABook=") == 0) {
            g_config.force_a_book = (line.substr(11) == "true");
        } else if (line.find("ForceBBook=") == 0) {
            g_config.force_b_book = (line.substr(11) == "true");
        } else if (line.find("Threshold_FXMajors=") == 0) {
            g_config.thresholds[0] = std::stod(line.substr(19));
        } else if (line.find("Threshold_Crypto=") == 0) {
            g_config.thresholds[1] = std::stod(line.substr(17));
        } else if (line.find("Threshold_Metals=") == 0) {
            g_config.thresholds[2] = std::stod(line.substr(17));
        } else if (line.find("Threshold_Energy=") == 0) {
            g_config.thresholds[3] = std::stod(line.substr(17));
        } else if (line.find("Threshold_Indices=") == 0) {
            g_config.thresholds[4] = std::stod(line.substr(18));
        } else if (line.find("Threshold_Other=") == 0) {
            g_config.thresholds[5] = std::stod(line.substr(16));
        }
    }
    
    // Configure cache with loaded settings
    g_score_cache.SetTTL(g_config.cache_ttl);
    
    return true;
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
    
    // Main trade request handler
    __declspec(dllexport) int OnTradeRequest(TradeRequest* request, TradeResult* result, void* server_context) {
        return ProcessTradeRouting(request, result);
    }
    
    // Trade close handler
    __declspec(dllexport) int OnTradeClose(int login, int ticket, double volume, double price) {
        // Just log the close - routing follows original decision
        std::ofstream log_file("ABBook_Plugin.log", std::ios::app);
        if (log_file.is_open()) {
            auto now = std::time(nullptr);
            char timestamp[32];
            std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
            
            log_file << timestamp << " - CLOSE: Login:" << login << " Ticket:" << ticket << std::endl;
            log_file.close();
        }
        
        return MT_RET_OK;
    }
    
    // Configuration reload
    __declspec(dllexport) void OnConfigUpdate() {
        std::lock_guard<std::mutex> lock(g_config_mutex);
        LoadConfiguration();
    }
    
    // Plugin initialization
    __declspec(dllexport) int PluginInit() {
        // Initialize Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return 1; // Failed
        }
        
        // Load configuration
        if (!LoadConfiguration()) {
            return 1; // Failed
        }
        
        // Log startup
        std::ofstream log_file("ABBook_Plugin.log", std::ios::app);
        if (log_file.is_open()) {
            auto now = std::time(nullptr);
            char timestamp[32];
            std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
            log_file << timestamp << " - Plugin initialized with caching enabled" << std::endl;
            log_file.close();
        }
        
        return 0; // Success
    }
    
    // Plugin cleanup
    __declspec(dllexport) void PluginCleanup() {
        WSACleanup();
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
} 