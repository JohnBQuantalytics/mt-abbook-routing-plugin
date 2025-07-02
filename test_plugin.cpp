/*
 * test_plugin.cpp
 * Test client for MT4/MT5 A/B-Book Server Plugin
 * Simulates trade requests to demonstrate plugin functionality
 */

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

// Import plugin functions
typedef int (*OnTradeRequestFunc)(void* request, void* result, void* context);
typedef int (*OnTradeCloseFunc)(int login, int ticket, double volume, double price);
typedef void (*OnConfigUpdateFunc)();
typedef int (*PluginInitFunc)();
typedef void (*PluginCleanupFunc)();

// Test trade structures (matching plugin)
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

int main() {
    std::cout << "MT4/MT5 A/B-Book Plugin Test Client" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // Load the plugin DLL
    HMODULE plugin = LoadLibraryA("ABBook_Plugin.dll");
    if (!plugin) {
        std::cout << "Error: Could not load ABBook_Plugin.dll" << std::endl;
        std::cout << "Make sure the plugin is compiled and in the same directory" << std::endl;
        return 1;
    }
    
    // Get function pointers
    auto OnTradeRequest = (OnTradeRequestFunc)GetProcAddress(plugin, "OnTradeRequest");
    auto OnTradeClose = (OnTradeCloseFunc)GetProcAddress(plugin, "OnTradeClose");
    auto OnConfigUpdate = (OnConfigUpdateFunc)GetProcAddress(plugin, "OnConfigUpdate");
    auto PluginInit = (PluginInitFunc)GetProcAddress(plugin, "PluginInit");
    auto PluginCleanup = (PluginCleanupFunc)GetProcAddress(plugin, "PluginCleanup");
    
    if (!OnTradeRequest || !PluginInit) {
        std::cout << "Error: Could not find required plugin functions" << std::endl;
        FreeLibrary(plugin);
        return 1;
    }
    
    // Initialize plugin
    std::cout << "Initializing plugin..." << std::endl;
    if (PluginInit() != 0) {
        std::cout << "Error: Plugin initialization failed" << std::endl;
        FreeLibrary(plugin);
        return 1;
    }
    std::cout << "Plugin initialized successfully" << std::endl << std::endl;
    
    // Test trades
    std::vector<TradeRequest> test_trades = {
        {12345, "EURUSD", 0, 1.0, 1.1234, 1.1200, 1.1300, "Test trade 1"},
        {12346, "BTCUSD", 1, 0.1, 45000.0, 44000.0, 0.0, "Test trade 2"},
        {12347, "XAUUSD", 0, 0.5, 1850.0, 1840.0, 1870.0, "Test trade 3"},
        {12348, "GBPUSD", 1, 2.0, 1.2500, 0.0, 1.2400, "Test trade 4"},
        {12349, "CRUDE", 0, 1.0, 75.50, 74.00, 78.00, "Test trade 5"}
    };
    
    std::cout << "Testing trade routing decisions:" << std::endl;
    std::cout << "================================" << std::endl;
    
    for (const auto& trade : test_trades) {
        TradeResult result;
        
        std::cout << "Trade: " << trade.symbol << " | ";
        std::cout << "Login: " << trade.login << " | ";
        std::cout << "Type: " << (trade.type == 0 ? "BUY" : "SELL") << " | ";
        std::cout << "Volume: " << trade.volume << " | ";
        std::cout << "Price: " << trade.price << std::endl;
        
        // Process trade through plugin
        int ret = OnTradeRequest((void*)&trade, (void*)&result, nullptr);
        
        if (ret == 0) {
            std::cout << "  Result: " << (result.routing == 0 ? "A-BOOK" : "B-BOOK");
            std::cout << " | Reason: " << result.reason << std::endl;
        } else {
            std::cout << "  Error: Trade processing failed" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    // Test configuration reload
    std::cout << "Testing configuration reload..." << std::endl;
    if (OnConfigUpdate) {
        OnConfigUpdate();
        std::cout << "Configuration reloaded" << std::endl;
    }
    
    // Test trade close
    std::cout << std::endl << "Testing trade close..." << std::endl;
    if (OnTradeClose) {
        OnTradeClose(12345, 1001, 1.0, 1.1240);
        std::cout << "Trade close processed" << std::endl;
    }
    
    // Cleanup
    std::cout << std::endl << "Cleaning up..." << std::endl;
    if (PluginCleanup) {
        PluginCleanup();
    }
    
    FreeLibrary(plugin);
    
    std::cout << "Test completed. Check ABBook_Plugin.log for detailed logs." << std::endl;
    std::cout << "Press any key to exit..." << std::endl;
    std::cin.get();
    
    return 0;
} 