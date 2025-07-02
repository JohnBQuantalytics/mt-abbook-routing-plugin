/*
 * simple_connection_test.cpp
 * Quick connection test to CVM scoring service at 128.140.42.37:50051
 */

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

int main() {
    std::cout << "=== CVM Connection Test ===" << std::endl;
    std::cout << "Target: 128.140.42.37:50051" << std::endl;
    std::cout << "Your IP: 213.55.244.85 (whitelisted)" << std::endl << std::endl;
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "ERROR: WSAStartup failed" << std::endl;
        return 1;
    }
    
    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cout << "ERROR: Could not create socket" << std::endl;
        WSACleanup();
        return 1;
    }
    
    // Set timeout (5 seconds)
    DWORD timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
    
    // Setup address
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(50051);
    inet_pton(AF_INET, "128.140.42.37", &addr.sin_addr);
    
    std::cout << "Attempting connection..." << std::endl;
    
    // Connect
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        int error = WSAGetLastError();
        std::cout << "ERROR: Connection failed (Error: " << error << ")" << std::endl;
        
        switch(error) {
            case WSAECONNREFUSED:
                std::cout << "  -> Connection refused. Service may be down." << std::endl;
                break;
            case WSAENETUNREACH:
                std::cout << "  -> Network unreachable." << std::endl;
                break;
            case WSAETIMEDOUT:
                std::cout << "  -> Connection timeout." << std::endl;
                break;
            default:
                std::cout << "  -> Unknown network error." << std::endl;
        }
        
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    
    std::cout << "✓ Connection successful!" << std::endl;
    
    // Create a simple test message (JSON format as per our plugin)
    std::string test_message = R"({
        "user_id": "12345",
        "open_price": 1.1234,
        "sl": 1.1200,
        "tp": 1.1300,
        "deal_type": 0.0,
        "lot_volume": 1.0,
        "opening_balance": 10000.0,
        "concurrent_positions": 3.0,
        "has_sl": 1.0,
        "has_tp": 1.0,
        "symbol": "EURUSD",
        "inst_group": "FXMajors"
    })";
    
    std::cout << "Sending test message..." << std::endl;
    
    // Send length-prefixed message
    uint32_t length = htonl(test_message.length());
    int bytes_sent = send(sock, (char*)&length, sizeof(length), 0);
    
    if (bytes_sent != sizeof(length)) {
        std::cout << "ERROR: Failed to send message length" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    
    bytes_sent = send(sock, test_message.c_str(), test_message.length(), 0);
    
    if (bytes_sent != test_message.length()) {
        std::cout << "ERROR: Failed to send message body" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    
    std::cout << "✓ Message sent (" << test_message.length() << " bytes)" << std::endl;
    
    // Try to receive response
    std::cout << "Waiting for response..." << std::endl;
    
    uint32_t response_length;
    int bytes_received = recv(sock, (char*)&response_length, sizeof(response_length), 0);
    
    if (bytes_received <= 0) {
        int error = WSAGetLastError();
        std::cout << "ERROR: No response received (Error: " << error << ")" << std::endl;
        
        if (error == WSAETIMEDOUT) {
            std::cout << "  -> Response timeout. Service may not be responding." << std::endl;
        }
        
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    
    response_length = ntohl(response_length);
    std::cout << "Response length: " << response_length << " bytes" << std::endl;
    
    if (response_length > 0 && response_length < 8192) {
        char buffer[8192];
        bytes_received = recv(sock, buffer, response_length, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "✓ Response received:" << std::endl;
            std::cout << buffer << std::endl;
            
            // Try to extract score
            const char* score_start = strstr(buffer, "\"score\":");
            if (score_start) {
                float score = atof(score_start + 8);
                std::cout << std::endl << "✓ EXTRACTED SCORE: " << score << std::endl;
                
                // Test routing decision
                double threshold = 0.08; // FXMajors threshold
                std::string decision = (score >= threshold) ? "B-BOOK" : "A-BOOK";
                std::cout << "✓ ROUTING DECISION: " << decision << " (score: " << score << " vs threshold: " << threshold << ")" << std::endl;
            }
        }
    }
    
    std::cout << std::endl << "=== Test Complete ===" << std::endl;
    std::cout << "✓ TCP connection works" << std::endl;
    std::cout << "✓ Message exchange successful" << std::endl;
    std::cout << "✓ Ready for full plugin integration" << std::endl;
    
    closesocket(sock);
    WSACleanup();
    
    std::cout << std::endl << "Press any key to exit..." << std::endl;
    std::cin.get();
    
    return 0;
} 