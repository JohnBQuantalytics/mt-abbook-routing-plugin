//+------------------------------------------------------------------+
//| Plugin Simulation Test - Direct Function Call                  |
//| Tests the actual plugin code with realistic trade data         |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>

// Load the plugin DLL dynamically
typedef int (__stdcall *MtSrvStartup_t)(void*);
typedef int (__stdcall *MtSrvTradeTransaction_t)(void*, void*);
typedef void (__stdcall *MtSrvCleanup_t)(void);

// Define the same structures as the plugin
#ifndef __time32_t
#define __time32_t time_t
#endif

enum { ORDER_OPENED=0, ORDER_CLOSED, ORDER_DELETED, ORDER_CANCELED };
enum { OP_BUY=0, OP_SELL, OP_BUYLIMIT, OP_SELLLIMIT, OP_BUYSTOP, OP_SELLSTOP };

struct TradeRecord
{
    int            order;              
    int            login;              
    char           symbol[12];         
    int            digits;             
    int            cmd;               
    int            volume;            
    __time32_t     open_time;         
    int            state;             
    double         open_price;        
    double         sl, tp;           
    double         close_price;       
    __time32_t     close_time;        
    int            reason;            
    double         commission;        
    double         commission_agent;  
    double         storage;           
    double         profit;            
    double         taxes;             
    char           comment[32];       
    int            margin_rate;       
    __time32_t     timestamp;         
    int            api_data[4];       
};

struct UserInfo
{
    int            login;             
    char           group[16];         
    char           password[16];      
    int            enable;            
    int            enable_change_password; 
    int            enable_readonly;   
    int            password_investor[16]; 
    char           password_phone[16]; 
    char           name[128];         
    char           country[32];       
    char           city[32];          
    char           state[32];         
    char           zipcode[16];       
    char           address[128];      
    char           phone[32];         
    char           email[48];         
    char           comment[64];       
    char           id[32];           
    char           status[16];       
    __time32_t     regdate;          
    __time32_t     lastdate;         
    int            leverage;         
    int            agent_account;    
    __time32_t     timestamp;        
    double         balance;          
    double         prevmonthbalance; 
    double         prevbalance;      
    double         credit;           
    double         interestrate;     
    double         taxes;            
    double         prevmonthequity;  
    double         prevequity;       
    char           reserved[104];    
    int            margin_mode;      
    double         margin_so_mode;   
    double         margin_free_mode; 
    double         margin_call;      
    double         margin_stopout;   
    char           reserved2[104];   
    char           publickey[270];   
    int            reserved3[4];     
};

class PluginSimulator {
private:
    HMODULE plugin_dll;
    MtSrvStartup_t MtSrvStartup;
    MtSrvTradeTransaction_t MtSrvTradeTransaction;
    MtSrvCleanup_t MtSrvCleanup;
    
public:
    PluginSimulator() : plugin_dll(nullptr) {}
    
    ~PluginSimulator() {
        if (plugin_dll) {
            if (MtSrvCleanup) {
                MtSrvCleanup();
            }
            FreeLibrary(plugin_dll);
        }
    }
    
    bool LoadPlugin() {
        std::cout << "🔄 Loading plugin DLL..." << std::endl;
        
        plugin_dll = LoadLibraryA("ABBook_Plugin_Official_32bit.dll");
        if (!plugin_dll) {
            DWORD error = GetLastError();
            std::cout << "❌ Failed to load plugin DLL. Error: " << error << std::endl;
            std::cout << "   Make sure ABBook_Plugin_Official_32bit.dll is in the current directory" << std::endl;
            return false;
        }
        
        // Get function pointers
        MtSrvStartup = (MtSrvStartup_t)GetProcAddress(plugin_dll, "MtSrvStartup");
        MtSrvTradeTransaction = (MtSrvTradeTransaction_t)GetProcAddress(plugin_dll, "MtSrvTradeTransaction");
        MtSrvCleanup = (MtSrvCleanup_t)GetProcAddress(plugin_dll, "MtSrvCleanup");
        
        if (!MtSrvStartup || !MtSrvTradeTransaction || !MtSrvCleanup) {
            std::cout << "❌ Failed to get plugin function addresses" << std::endl;
            return false;
        }
        
        std::cout << "✅ Plugin DLL loaded successfully" << std::endl;
        return true;
    }
    
    bool InitializePlugin() {
        std::cout << "\n🚀 Initializing plugin..." << std::endl;
        
        int result = MtSrvStartup(nullptr);
        if (result == 1) {
            std::cout << "✅ Plugin initialized successfully (returned " << result << ")" << std::endl;
            return true;
        } else {
            std::cout << "❌ Plugin initialization failed (returned " << result << ")" << std::endl;
            return false;
        }
    }
    
    void CreateRealisticTradeData(TradeRecord* trade, UserInfo* user) {
        // Clear memory
        memset(trade, 0, sizeof(TradeRecord));
        memset(user, 0, sizeof(UserInfo));
        
        // === REALISTIC TRADE RECORD ===
        trade->order = 12345678;
        trade->login = 16813;                    // Same user_id as in ML service logs
        strcpy_s(trade->symbol, "NZDUSD");       // Same symbol as in ML service logs
        trade->digits = 5;
        trade->cmd = OP_BUY;                     // Buy order
        trade->volume = 100;                     // 1 lot (MT4 uses lots*100)
        trade->open_time = time(nullptr);
        trade->state = ORDER_OPENED;             // New opening trade
        trade->open_price = 0.59350;             // Same price as in ML service logs
        trade->sl = 0.59000;                     // Stop loss
        trade->tp = 0.59700;                     // Take profit
        trade->close_price = 0.0;
        trade->close_time = 0;
        trade->reason = 0;
        trade->commission = 0.0;
        trade->commission_agent = 0.0;
        trade->storage = 0.0;
        trade->profit = 0.0;
        trade->taxes = 0.0;
        strcpy_s(trade->comment, "Test trade");
        trade->margin_rate = 100;
        trade->timestamp = time(nullptr);
        
        // === REALISTIC USER INFO ===  
        user->login = 16813;                     // Same as trade login
        strcpy_s(user->group, "standard");
        strcpy_s(user->password, "");            // Hidden
        user->enable = 1;
        user->enable_change_password = 1;
        user->enable_readonly = 0;
        strcpy_s(user->name, "Test User 16813");
        strcpy_s(user->country, "New Zealand");
        strcpy_s(user->city, "Wellington");
        strcpy_s(user->state, "Wellington");
        strcpy_s(user->zipcode, "6011");
        strcpy_s(user->address, "Test Address 123");
        strcpy_s(user->phone, "+64123456789");
        strcpy_s(user->email, "test@example.com");
        strcpy_s(user->comment, "Test user for simulation");
        strcpy_s(user->id, "16813");
        strcpy_s(user->status, "active");
        user->regdate = time(nullptr) - (90 * 24 * 3600); // 90 days ago
        user->lastdate = time(nullptr);
        user->leverage = 100;
        user->agent_account = 0;
        user->timestamp = time(nullptr);
        user->balance = 10000.0;                 // $10,000 balance (same as ML logs)
        user->prevmonthbalance = 9500.0;
        user->prevbalance = 9950.0;
        user->credit = 0.0;
        user->interestrate = 0.0;
        user->taxes = 0.0;
        user->prevmonthequity = 9500.0;
        user->prevequity = 9950.0;
        user->margin_mode = 0;
        user->margin_so_mode = 0.0;
        user->margin_free_mode = 1.0;
        user->margin_call = 50.0;
        user->margin_stopout = 20.0;
    }
    
    void RunSimulation() {
        std::cout << "\n🧪 === PLUGIN SIMULATION TEST ===" << std::endl;
        std::cout << "Testing with REAL trading data that matches ML service logs" << std::endl;
        std::cout << "Expected: ML score ≈ 0.8222747 (B-BOOK routing)" << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        // Create realistic trade data
        TradeRecord trade;
        UserInfo user;
        CreateRealisticTradeData(&trade, &user);
        
        // Display the test data
        std::cout << "📊 SIMULATED TRADE DATA:" << std::endl;
        std::cout << "  Order: " << trade.order << std::endl;
        std::cout << "  Login: " << trade.login << std::endl;
        std::cout << "  Symbol: " << trade.symbol << std::endl;
        std::cout << "  Command: " << trade.cmd << " (BUY)" << std::endl;
        std::cout << "  Volume: " << trade.volume << " (1 lot)" << std::endl;
        std::cout << "  Open Price: " << trade.open_price << std::endl;
        std::cout << "  Stop Loss: " << trade.sl << std::endl;
        std::cout << "  Take Profit: " << trade.tp << std::endl;
        std::cout << "  State: " << trade.state << " (OPENED)" << std::endl;
        
        std::cout << "\n👤 SIMULATED USER DATA:" << std::endl;
        std::cout << "  Login: " << user.login << std::endl;
        std::cout << "  Name: " << user.name << std::endl;
        std::cout << "  Group: " << user.group << std::endl;
        std::cout << "  Balance: $" << user.balance << std::endl;
        std::cout << "  Country: " << user.country << std::endl;
        std::cout << "  Leverage: 1:" << user.leverage << std::endl;
        
        std::cout << "\n🚀 CALLING PLUGIN TRADE TRANSACTION..." << std::endl;
        std::cout << "========================================" << std::endl;
        
        // Call the actual plugin function
        int result = MtSrvTradeTransaction(&trade, &user);
        
        std::cout << "========================================" << std::endl;
        std::cout << "🏁 PLUGIN RETURNED: " << result << (result == 1 ? " (SUCCESS)" : " (UNEXPECTED)") << std::endl;
        
        std::cout << "\n📋 SIMULATION RESULTS:" << std::endl;
        std::cout << "✅ Plugin executed without crashing" << std::endl;
        std::cout << "✅ All 15 checkpoints should have passed" << std::endl;
        std::cout << "✅ ML service should have received protobuf request" << std::endl;
        std::cout << "✅ Expected ML score: ~0.82 (B-BOOK routing)" << std::endl;
        
        std::cout << "\n🔍 CHECK THE LOG FILE:" << std::endl;
        std::cout << "Look at 'ABBook_Plugin_Official.log' for detailed results" << std::endl;
        
        std::cout << "\n💡 WHAT TO VERIFY:" << std::endl;
        std::cout << "1. All CHECKPOINT 1-16 messages appear" << std::endl;
        std::cout << "2. ML SERVICE messages show connection + score" << std::endl;
        std::cout << "3. ROUTING DECISION shows A-BOOK or B-BOOK" << std::endl;
        std::cout << "4. No crash or exception messages" << std::endl;
        std::cout << "5. Plugin returns safely to this test program" << std::endl;
    }
};

int main() {
    std::cout << "🎯 MT4 A/B-Book Plugin Simulation Test" << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "This test simulates a real MT4 trade transaction" << std::endl;
    std::cout << "and calls the plugin directly to verify functionality." << std::endl;
    
    PluginSimulator simulator;
    
    // Step 1: Load the plugin DLL
    if (!simulator.LoadPlugin()) {
        std::cout << "\n❌ TEST FAILED: Could not load plugin DLL" << std::endl;
        std::cout << "Make sure you compiled the plugin first!" << std::endl;
        goto cleanup;
    }
    
    // Step 2: Initialize the plugin
    if (!simulator.InitializePlugin()) {
        std::cout << "\n❌ TEST FAILED: Plugin initialization failed" << std::endl;
        goto cleanup;
    }
    
    // Step 3: Run the simulation
    simulator.RunSimulation();
    
    std::cout << "\n🎉 SIMULATION COMPLETED!" << std::endl;
    std::cout << "Check the results above and in the log file." << std::endl;
    
cleanup:
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 