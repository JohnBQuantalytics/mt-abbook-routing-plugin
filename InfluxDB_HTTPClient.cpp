/*
 * InfluxDB_HTTPClient.cpp
 * Simple HTTP client for sending metrics to InfluxDB from MT4/MT5
 * Implements InfluxDB Line Protocol over HTTP
 */

#include <windows.h>
#include <wininet.h>
#include <string>
#include <iostream>
#include <sstream>

#pragma comment(lib, "wininet.lib")

#ifdef __cplusplus
extern "C" {
#endif

// InfluxDB configuration structure
struct InfluxDBConfig {
    char host[256];
    int port;
    char database[64];
    char username[64];
    char password[64];
    bool useSSL;
    char measurement[64];
};

InfluxDBConfig g_config = {0};
bool g_initialized = false;

__declspec(dllexport) bool __stdcall InitializeInfluxDB(
    const char* host,
    int port,
    const char* database,
    const char* username,
    const char* password,
    bool useSSL
) {
    strncpy_s(g_config.host, host, sizeof(g_config.host) - 1);
    g_config.port = port;
    strncpy_s(g_config.database, database, sizeof(g_config.database) - 1);
    strncpy_s(g_config.username, username, sizeof(g_config.username) - 1);
    strncpy_s(g_config.password, password, sizeof(g_config.password) - 1);
    g_config.useSSL = useSSL;
    strcpy_s(g_config.measurement, "abbook_routing");
    
    g_initialized = true;
    return true;
}

__declspec(dllexport) bool __stdcall SendTradeMetric(
    const char* symbol,
    const char* instrument_group,
    const char* routing_decision,
    double score,
    double threshold,
    double volume,
    double price,
    long long timestamp_ns
) {
    if (!g_initialized) {
        return false;
    }
    
    // Build InfluxDB line protocol string
    std::ostringstream lineProtocol;
    lineProtocol << g_config.measurement 
                 << ",symbol=" << symbol
                 << ",group=" << instrument_group
                 << ",decision=" << routing_decision
                 << " score=" << score
                 << ",threshold=" << threshold
                 << ",volume=" << volume
                 << ",price=" << price
                 << " " << timestamp_ns;
    
    std::string data = lineProtocol.str();
    
    // Send HTTP POST request
    return SendHTTPPost(data.c_str());
}

__declspec(dllexport) bool __stdcall SendCustomMetric(
    const char* measurement,
    const char* tags,
    const char* fields,
    long long timestamp_ns
) {
    if (!g_initialized) {
        return false;
    }
    
    // Build line protocol: measurement,tags fields timestamp
    std::ostringstream lineProtocol;
    lineProtocol << measurement;
    
    if (tags && strlen(tags) > 0) {
        lineProtocol << "," << tags;
    }
    
    lineProtocol << " " << fields;
    
    if (timestamp_ns > 0) {
        lineProtocol << " " << timestamp_ns;
    }
    
    std::string data = lineProtocol.str();
    return SendHTTPPost(data.c_str());
}

bool SendHTTPPost(const char* data) {
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    bool success = false;
    
    try {
        // Initialize WinINet
        hInternet = InternetOpenA("MT4/MT5 InfluxDB Client", 
                                 INTERNET_OPEN_TYPE_DIRECT, 
                                 NULL, NULL, 0);
        if (!hInternet) {
            return false;
        }
        
        // Connect to server
        DWORD flags = g_config.useSSL ? INTERNET_FLAG_SECURE : 0;
        hConnect = InternetConnectA(hInternet, 
                                   g_config.host, 
                                   g_config.port,
                                   g_config.username[0] ? g_config.username : NULL,
                                   g_config.password[0] ? g_config.password : NULL,
                                   INTERNET_SERVICE_HTTP, 
                                   flags, 0);
        if (!hConnect) {
            goto cleanup;
        }
        
        // Build URL path
        char url[512];
        sprintf_s(url, "/write?db=%s&precision=ns", g_config.database);
        
        // Create HTTP request
        DWORD requestFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
        if (g_config.useSSL) {
            requestFlags |= INTERNET_FLAG_SECURE;
        }
        
        hRequest = HttpOpenRequestA(hConnect, "POST", url, NULL, NULL, NULL, requestFlags, 0);
        if (!hRequest) {
            goto cleanup;
        }
        
        // Set headers
        const char* headers = "Content-Type: application/x-www-form-urlencoded\r\n";
        if (!HttpAddRequestHeadersA(hRequest, headers, -1, HTTP_ADDREQ_FLAG_ADD)) {
            goto cleanup;
        }
        
        // Send request
        DWORD dataLength = strlen(data);
        if (HttpSendRequestA(hRequest, NULL, 0, (LPVOID)data, dataLength)) {
            // Check response status
            char statusCode[32];
            DWORD statusSize = sizeof(statusCode);
            DWORD index = 0;
            
            if (HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE, statusCode, &statusSize, &index)) {
                int status = atoi(statusCode);
                success = (status >= 200 && status < 300);
            }
        }
        
    } catch (...) {
        success = false;
    }
    
cleanup:
    if (hRequest) InternetCloseHandle(hRequest);
    if (hConnect) InternetCloseHandle(hConnect);
    if (hInternet) InternetCloseHandle(hInternet);
    
    return success;
}

__declspec(dllexport) bool __stdcall SendBatchMetrics(
    const char** lines,
    int count
) {
    if (!g_initialized || !lines || count <= 0) {
        return false;
    }
    
    // Combine multiple line protocol lines
    std::ostringstream batch;
    for (int i = 0; i < count; i++) {
        if (lines[i] && strlen(lines[i]) > 0) {
            batch << lines[i] << "\n";
        }
    }
    
    std::string data = batch.str();
    if (data.empty()) {
        return false;
    }
    
    return SendHTTPPost(data.c_str());
}

__declspec(dllexport) bool __stdcall TestConnection() {
    if (!g_initialized) {
        return false;
    }
    
    // Send a simple test metric
    long long timestamp = GetCurrentTimeNS();
    return SendCustomMetric("test_connection", "source=mt4_mt5", "value=1", timestamp);
}

long long GetCurrentTimeNS() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    
    // Convert from 100-nanosecond intervals since 1601 to nanoseconds since Unix epoch
    const long long EPOCH_DIFF = 116444736000000000LL; // 100-ns intervals between 1601 and 1970
    long long ns = (uli.QuadPart - EPOCH_DIFF) * 100;
    
    return ns;
}

__declspec(dllexport) void __stdcall SetMeasurementName(const char* measurement) {
    if (measurement && strlen(measurement) > 0) {
        strncpy_s(g_config.measurement, measurement, sizeof(g_config.measurement) - 1);
    }
}

__declspec(dllexport) const char* __stdcall GetLastError() {
    // Simple error reporting (in production, use proper error handling)
    static char errorBuffer[256];
    DWORD error = GetLastError();
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, error, 0, errorBuffer, sizeof(errorBuffer), NULL);
    return errorBuffer;
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_PROCESS_DETACH:
            g_initialized = false;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

#ifdef __cplusplus
}
#endif

/*
 * Usage Example in MT4/MT5:
 * 
 * #import "InfluxDB_HTTPClient.dll"
 *   bool InitializeInfluxDB(string host, int port, string database, string username, string password, bool useSSL);
 *   bool SendTradeMetric(string symbol, string group, string decision, double score, double threshold, double volume, double price, long timestamp);
 *   bool TestConnection();
 * #import
 * 
 * // In OnInit():
 * InitializeInfluxDB("localhost", 8086, "trading", "", "", false);
 * 
 * // When routing trades:
 * long timestamp = TimeCurrent() * 1000000000; // Convert to nanoseconds
 * SendTradeMetric("EURUSD", "FXMajors", "A_BOOK", 0.045, 0.08, 1.0, 1.1234, timestamp);
 */ 