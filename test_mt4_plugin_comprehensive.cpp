/*
 * test_mt4_plugin_comprehensive.cpp
 * Comprehensive test for MT4 ABBook Plugin with ML scoring service
 * Tests connection to 188.245.254.12:50051 and simulates trade scenarios
 */

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <ctime>

// Import plugin functions
typedef int (*MtSrvStartupFunc)(void* server_interface);
typedef int (*MtSrvTradeTransactionFunc)(void* trade, void* user, void* reserved);
typedef void (*MtSrvConfigUpdateFunc)();
typedef void (*MtSrvCleanupFunc)();

// Test structures matching plugin
struct TestTradeRecord {
    int order;
    int login;
    char symbol[12];
    int digits;
    int cmd;
    int volume;
    time_t open_time;
    int state;
    double open_price;
    double sl, tp;
    time_t close_time;
    int gw_volume;
    time_t expiration;
    char reason;
    char conv_rates[2];
    double commission;
    double commission_agent;
    double storage;
    double close_price;
    double profit;
    double taxes;
    int magic;
    char comment[32];
    int gw_order;
    int activation;
    short gw_open_price;
    short gw_close_price;
    int margin_rate;
    time_t timestamp;
    int api_data[4];
};

struct TestUserRecord {
    int login;
    char group[16];
    char password[16];
    int enable;
    int enable_change_password;
    int enable_read_only;
    char name[128];
    char country[32];
    char city[32];
    char state[32];
    char zipcode[16];
    char address[128];
    char phone[32];
    char email[64];
    char comment[64];
    char id[32];
    char status[16];
    time_t regdate;
    time_t lastdate;
    int leverage;
    int agent_account;
    time_t timestamp;
    double balance;
    double prevmonthbalance;
    double prevbalance;
    double credit;
    double interestrate;
    double taxes;
    double prevmonthequity;
    double prevequity;
    int reserved2[2];
    char publickey[270];
    int reserved[7];
};

class TestLogger {
private:
    std::string log_file;
    
public:
    TestLogger() : log_file("ABBook_Test_" + GetTimestamp() + ".log") {}
    
    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", std::localtime(&time_t));
        return std::string(buffer);
    }
    
    void Log(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        char timestamp[100];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
        
        std::string log_msg = "[" + std::string(timestamp) + "] " + message;
        
        std::cout << log_msg << std::endl;
        
        std::ofstream file(log_file, std::ios::app);
        if (file.is_open()) {
            file << log_msg << std::endl;
            file.close();
        }
    }
    
    void LogError(const std::string& message) { Log("ERROR: " + message); }
    void LogInfo(const std::string& message) { Log("INFO: " + message); }
    void LogSuccess(const std::string& message) { Log("SUCCESS: " + message); }
};

TestLogger test_logger;

// Test trade scenarios
std::vector<TestTradeRecord> CreateTestTrades() {
    std::vector<TestTradeRecord> trades;
    
    // Test trade 1: EURUSD Buy - FX Major
    TestTradeRecord trade1 = {};
    trade1.order = 1001;
    trade1.login = 12345;
    strcpy_s(trade1.symbol, "EURUSD");
    trade1.digits = 5;
    trade1.cmd = 0; // Buy
    trade1.volume = 100; // 1.0 lot
    trade1.open_time = time(nullptr);
    trade1.open_price = 1.1234;
    trade1.sl = 1.1200;
    trade1.tp = 1.1300;
    strcpy_s(trade1.comment, "Test FX Major Buy");
    trades.push_back(trade1);
    
    // Test trade 2: BTCUSD Sell - Crypto
    TestTradeRecord trade2 = {};
    trade2.order = 1002;
    trade2.login = 12346;
    strcpy_s(trade2.symbol, "BTCUSD");
    trade2.digits = 2;
    trade2.cmd = 1; // Sell
    trade2.volume = 10; // 0.1 lot
    trade2.open_time = time(nullptr);
    trade2.open_price = 45000.0;
    trade2.sl = 46000.0;
    trade2.tp = 44000.0;
    strcpy_s(trade2.comment, "Test Crypto Sell");
    trades.push_back(trade2);
    
    // Test trade 3: XAUUSD Buy - Metals
    TestTradeRecord trade3 = {};
    trade3.order = 1003;
    trade3.login = 12347;
    strcpy_s(trade3.symbol, "XAUUSD");
    trade3.digits = 2;
    trade3.cmd = 0; // Buy
    trade3.volume = 50; // 0.5 lot
    trade3.open_time = time(nullptr);
    trade3.open_price = 1850.0;
    trade3.sl = 1840.0;
    trade3.tp = 1870.0;
    strcpy_s(trade3.comment, "Test Metal Buy");
    trades.push_back(trade3);
    
    // Test trade 4: CRUDE Buy - Energy
    TestTradeRecord trade4 = {};
    trade4.order = 1004;
    trade4.login = 12348;
    strcpy_s(trade4.symbol, "CRUDE");
    trade4.digits = 3;
    trade4.cmd = 0; // Buy
    trade4.volume = 100; // 1.0 lot
    trade4.open_time = time(nullptr);
    trade4.open_price = 75.50;
    trade4.sl = 74.00;
    trade4.tp = 78.00;
    strcpy_s(trade4.comment, "Test Energy Buy");
    trades.push_back(trade4);
    
    // Test trade 5: SPX500 Sell - Indices
    TestTradeRecord trade5 = {};
    trade5.order = 1005;
    trade5.login = 12349;
    strcpy_s(trade5.symbol, "SPX500");
    trade5.digits = 1;
    trade5.cmd = 1; // Sell
    trade5.volume = 100; // 1.0 lot
    trade5.open_time = time(nullptr);
    trade5.open_price = 4500.0;
    trade5.sl = 4520.0;
    trade5.tp = 4480.0;
    strcpy_s(trade5.comment, "Test Index Sell");
    trades.push_back(trade5);
    
    return trades;
}

std::vector<TestUserRecord> CreateTestUsers() {
    std::vector<TestUserRecord> users;
    
    // Create test users for each trade
    for (int i = 0; i < 5; i++) {
        TestUserRecord user = {};
        user.login = 12345 + i;
        strcpy_s(user.group, "FXMajors");
        strcpy_s(user.name, ("Test User " + std::to_string(i + 1)).c_str());
        strcpy_s(user.country, "US");
        strcpy_s(user.email, ("test" + std::to_string(i + 1) + "@example.com").c_str());
        user.leverage = 100;
        user.balance = 10000.0 + (i * 1000.0);
        user.regdate = time(nullptr) - (86400 * 30); // 30 days ago
        user.lastdate = time(nullptr);
        users.push_back(user);
    }
    
    return users;
}

void TestConnectionToMLService() {
    test_logger.LogInfo("Testing direct connection to ML scoring service...");
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        test_logger.LogError("WSAStartup failed");
        return;
    }
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        test_logger.LogError("Socket creation failed");
        WSACleanup();
        return;
    }
    
    // Set timeout
    DWORD timeout = 5000; // 5 seconds
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(50051);
    inet_pton(AF_INET, "188.245.254.12", &addr.sin_addr);
    
    test_logger.LogInfo("Attempting connection to 188.245.254.12:50051...");
    
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0) {
        test_logger.LogSuccess("Successfully connected to ML scoring service");
        
        // Send test message
        std::string test_msg = "{\"test\":\"connection\"}";
        uint32_t length = test_msg.length();
        
        if (send(sock, (char*)&length, sizeof(length), 0) == sizeof(length)) {
            if (send(sock, test_msg.c_str(), length, 0) == (int)length) {
                test_logger.LogSuccess("Test message sent successfully");
                
                // Try to receive response
                uint32_t response_length;
                if (recv(sock, (char*)&response_length, sizeof(response_length), 0) == sizeof(response_length)) {
                    if (response_length > 0 && response_length < 8192) {
                        char buffer[8192];
                        int bytes_received = recv(sock, buffer, response_length, 0);
                        if (bytes_received > 0) {
                            buffer[bytes_received] = '\0';
                            test_logger.LogSuccess("Received response: " + std::string(buffer));
                        }
                    }
                }
            }
        }
    } else {
        test_logger.LogError("Failed to connect to ML scoring service");
        int error = WSAGetLastError();
        test_logger.LogError("WSA Error: " + std::to_string(error));
    }
    
    closesocket(sock);
    WSACleanup();
}

int main() {
    std::cout << "=== MT4 ABBook Plugin Comprehensive Test ===" << std::endl;
    std::cout << "ML Scoring Service: 188.245.254.12:50051" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    test_logger.LogInfo("Starting comprehensive plugin test");
    
    // Test 1: Direct connection to ML service
    TestConnectionToMLService();
    
    // Test 2: Load plugin DLL
    test_logger.LogInfo("Loading plugin DLL...");
    HMODULE plugin = LoadLibraryA("ABBook_Plugin_32bit.dll");
    if (!plugin) {
        test_logger.LogError("Could not load ABBook_Plugin_32bit.dll");
        test_logger.LogError("Make sure the plugin is compiled and in the same directory");
        return 1;
    }
    
    test_logger.LogSuccess("Plugin DLL loaded successfully");
    
    // Get function pointers
    auto MtSrvStartup = (MtSrvStartupFunc)GetProcAddress(plugin, "MtSrvStartup");
    auto MtSrvTradeTransaction = (MtSrvTradeTransactionFunc)GetProcAddress(plugin, "MtSrvTradeTransaction");
    auto MtSrvConfigUpdate = (MtSrvConfigUpdateFunc)GetProcAddress(plugin, "MtSrvConfigUpdate");
    auto MtSrvCleanup = (MtSrvCleanupFunc)GetProcAddress(plugin, "MtSrvCleanup");
    
    if (!MtSrvStartup || !MtSrvTradeTransaction) {
        test_logger.LogError("Could not find required plugin functions");
        FreeLibrary(plugin);
        return 1;
    }
    
    test_logger.LogSuccess("Plugin functions loaded successfully");
    
    // Test 3: Initialize plugin
    test_logger.LogInfo("Initializing plugin...");
    if (MtSrvStartup(nullptr) != 0) {
        test_logger.LogError("Plugin initialization failed");
        FreeLibrary(plugin);
        return 1;
    }
    
    test_logger.LogSuccess("Plugin initialized successfully");
    
    // Test 4: Test configuration update
    test_logger.LogInfo("Testing configuration reload...");
    if (MtSrvConfigUpdate) {
        MtSrvConfigUpdate();
        test_logger.LogSuccess("Configuration reloaded");
    }
    
    // Test 5: Simulate trade scenarios
    test_logger.LogInfo("Starting trade simulation tests...");
    
    std::vector<TestTradeRecord> test_trades = CreateTestTrades();
    std::vector<TestUserRecord> test_users = CreateTestUsers();
    
    for (size_t i = 0; i < test_trades.size(); i++) {
        TestTradeRecord& trade = test_trades[i];
        TestUserRecord& user = test_users[i];
        
        test_logger.LogInfo("=== Processing Trade " + std::to_string(i + 1) + " ===");
        test_logger.LogInfo("Symbol: " + std::string(trade.symbol));
        test_logger.LogInfo("Login: " + std::to_string(trade.login));
        test_logger.LogInfo("Type: " + std::string(trade.cmd == 0 ? "BUY" : "SELL"));
        test_logger.LogInfo("Volume: " + std::to_string(trade.volume / 100.0) + " lots");
        test_logger.LogInfo("Price: " + std::to_string(trade.open_price));
        test_logger.LogInfo("Balance: " + std::to_string(user.balance));
        
        // Process trade through plugin
        int result = MtSrvTradeTransaction(&trade, &user, nullptr);
        
        if (result == 0) {
            test_logger.LogSuccess("Trade processed successfully");
        } else {
            test_logger.LogError("Trade processing failed with code: " + std::to_string(result));
        }
        
        // Small delay between trades
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Test 6: Monitor logs
    test_logger.LogInfo("Test completed. Checking log files...");
    
    // Check if plugin log files exist
    std::ifstream debug_log("ABBook_Plugin_Debug.log");
    if (debug_log.is_open()) {
        test_logger.LogSuccess("Plugin debug log file found");
        debug_log.close();
    } else {
        test_logger.LogError("Plugin debug log file not found");
    }
    
    std::ifstream plugin_log("ABBook_Plugin.log");
    if (plugin_log.is_open()) {
        test_logger.LogSuccess("Plugin routing decisions log file found");
        plugin_log.close();
    } else {
        test_logger.LogError("Plugin routing decisions log file not found");
    }
    
    // Test 7: Cleanup
    test_logger.LogInfo("Cleaning up...");
    if (MtSrvCleanup) {
        MtSrvCleanup();
        test_logger.LogSuccess("Plugin cleanup completed");
    }
    
    FreeLibrary(plugin);
    test_logger.LogSuccess("Plugin DLL unloaded");
    
    test_logger.LogInfo("=== Test Summary ===");
    test_logger.LogInfo("1. ML Service Connection: Check console output above");
    test_logger.LogInfo("2. Plugin DLL Loading: SUCCESS");
    test_logger.LogInfo("3. Plugin Initialization: SUCCESS");
    test_logger.LogInfo("4. Configuration Reload: SUCCESS");
    test_logger.LogInfo("5. Trade Processing: Check individual trade results above");
    test_logger.LogInfo("6. Log Files: Check ABBook_Plugin_Debug.log and ABBook_Plugin.log");
    test_logger.LogInfo("7. Cleanup: SUCCESS");
    
    std::cout << std::endl << "Test completed. Press any key to exit..." << std::endl;
    std::cin.get();
    
    return 0;
} 