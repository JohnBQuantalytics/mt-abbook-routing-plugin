//+------------------------------------------------------------------+
//| Simple MT4 Plugin Compatibility Test                            |
//| Tests basic plugin loading and function exports                 |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>

// Simple test plugin with minimal MT4 API functions
extern "C" {
    __declspec(dllexport) int __stdcall MtSrvStartup(void* mt_interface) {
        std::cout << "=== TEST PLUGIN STARTUP ===" << std::endl;
        std::cout << "Plugin loaded successfully!" << std::endl;
        return 0; // Success
    }

    __declspec(dllexport) void __stdcall MtSrvCleanup(void) {
        std::cout << "=== TEST PLUGIN CLEANUP ===" << std::endl;
    }

    __declspec(dllexport) int __stdcall MtSrvAbout(void* reserved) {
        std::cout << "=== TEST PLUGIN ABOUT ===" << std::endl;
        return 0;
    }

    // Minimal trade transaction handler for testing
    __declspec(dllexport) int __stdcall MtSrvTradeTransaction(void* trade, void* user) {
        std::cout << "=== TEST TRADE TRANSACTION ===" << std::endl;
        return 0; // Success
    }

    __declspec(dllexport) void __stdcall MtSrvConfigUpdate(void* config) {
        std::cout << "=== TEST CONFIG UPDATE ===" << std::endl;
    }
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            std::cout << "DLL_PROCESS_ATTACH: Test plugin loaded" << std::endl;
            break;
        case DLL_PROCESS_DETACH:
            std::cout << "DLL_PROCESS_DETACH: Test plugin unloaded" << std::endl;
            break;
    }
    return TRUE;
} 