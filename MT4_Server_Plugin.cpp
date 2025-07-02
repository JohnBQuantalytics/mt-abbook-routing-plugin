/*
 * MT4_Server_Plugin.cpp
 * Server-side C++ plugin for real-time scoring-based A/B-book routing
 * Hooks into MT4/MT5 server trade pipeline
 */

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>
#include <iostream>
#include <thread>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

// MT Server structures (placeholders - need actual SDK)
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

// Configuration
struct PluginConfig {
    char cvm_ip[64];
    int cvm_port;
    int timeout;
    double fallback_score;
    bool force_a_book;
    bool force_b_book;
    double thresholds[6];  // FXMajors, Crypto, Metals, Energy, Indices, Other
} g_config;

// Protobuf structures (simplified)
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

struct ScoringResponse {
    float score;
    char warnings[256];
};

// CVM Communication
class CVMClient {
private:
    SOCKET sock;
    
public:
    float GetScore(const ScoringRequest& request) {
        // Create socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            return g_config.fallback_score;
        }
        
        // Set timeout
        DWORD timeout = g_config.timeout;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        
        // Connect to CVM
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(g_config.cvm_port);
        inet_pton(AF_INET, g_config.cvm_ip, &addr.sin_addr);
        
        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
            closesocket(sock);
            return g_config.fallback_score;
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
        uint32_t length = htonl(json.length());
        send(sock, (char*)&length, sizeof(length), 0);
        send(sock, json.c_str(), json.length(), 0);
        
        // Receive response
        uint32_t response_length;
        if (recv(sock, (char*)&response_length, sizeof(response_length), 0) <= 0) {
            closesocket(sock);
            return g_config.fallback_score;
        }
        
        response_length = ntohl(response_length);
        char buffer[1024];
        if (recv(sock, buffer, response_length, 0) <= 0) {
            closesocket(sock);
            return g_config.fallback_score;
        }
        
        buffer[response_length] = '\0';
        closesocket(sock);
        
        // Parse JSON response (simplified)
        const char* score_start = strstr(buffer, "\"score\":");
        if (score_start) {
            return atof(score_start + 8);
        }
        
        return g_config.fallback_score;
    }
};

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

bool LoadConfiguration() {
    std::ifstream config_file("ABBook_Config.ini");
    if (!config_file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(config_file, line)) {
        if (line.find("CVM_IP=") == 0) {
            strcpy_s(g_config.cvm_ip, line.substr(7).c_str());
        } else if (line.find("CVM_Port=") == 0) {
            g_config.cvm_port = std::stoi(line.substr(9));
        } else if (line.find("ConnectionTimeout=") == 0) {
            g_config.timeout = std::stoi(line.substr(18));
        } else if (line.find("FallbackScore=") == 0) {
            g_config.fallback_score = std::stod(line.substr(14));
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
    
    config_file.close();
    return true;
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

// Main processing function
int ProcessTradeRouting(const TradeRequest* trade, TradeResult* result) {
    try {
        // Check global overrides
        if (g_config.force_a_book) {
            result->routing = 0; // A-book
            result->retcode = 0; // Success
            strcpy_s(result->reason, "FORCED_A_BOOK");
            LogDecision(trade, 0.0f, 0.0, 0);
            return 0;
        }
        
        if (g_config.force_b_book) {
            result->routing = 1; // B-book
            result->retcode = 0; // Success
            strcpy_s(result->reason, "FORCED_B_BOOK");
            LogDecision(trade, 1.0f, 0.0, 1);
            return 0;
        }
        
        // Build scoring request
        ScoringRequest scoring_req;
        BuildScoringRequest(trade, &scoring_req);
        
        // Get score from CVM
        CVMClient cvm_client;
        float score = cvm_client.GetScore(scoring_req);
        
        // Get threshold for instrument group
        double threshold = GetThresholdForGroup(scoring_req.inst_group);
        
        // Make routing decision
        if (score >= threshold) {
            result->routing = 1; // B-book
            strcpy_s(result->reason, "SCORE_ABOVE_THRESHOLD");
        } else {
            result->routing = 0; // A-book
            strcpy_s(result->reason, "SCORE_BELOW_THRESHOLD");
        }
        
        result->retcode = 0; // Success
        
        // Log decision
        LogDecision(trade, score, threshold, result->routing);
        
        return 0; // Success
        
    } catch (...) {
        result->routing = 0; // Default to A-book on error
        result->retcode = 1; // Error
        strcpy_s(result->reason, "ERROR_FALLBACK");
        return 1;
    }
}

// Plugin API exports
extern "C" {
    
    // Main trade request handler
    __declspec(dllexport) int OnTradeRequest(TradeRequest* request, TradeResult* result, void* server_context) {
        return ProcessTradeRouting(request, result);
    }
    
    // Trade close handler (no scoring needed)
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
        
        return 0;
    }
    
    // Configuration reload
    __declspec(dllexport) void OnConfigUpdate() {
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
            log_file << timestamp << " - Plugin initialized" << std::endl;
            log_file.close();
        }
        
        return 0; // Success
    }
    
    // Plugin cleanup
    __declspec(dllexport) void PluginCleanup() {
        WSACleanup();
    }
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
} 