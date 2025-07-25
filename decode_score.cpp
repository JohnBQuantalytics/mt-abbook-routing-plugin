#include <iostream>
#include <cstring>

int main() {
    // The response bytes from ML service
    unsigned char response_bytes[] = {0x00, 0xA2, 0x52, 0x3B};
    
    // Interpret as float (little-endian)
    float score;
    memcpy(&score, response_bytes, 4);
    
    std::cout << "ML Service Response Analysis:" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Raw bytes: 00 A2 52 3B" << std::endl;
    std::cout << "Decoded score: " << score << std::endl;
    std::cout << "Score in range [0,1]: " << (score >= 0.0f && score <= 1.0f ? "YES" : "NO") << std::endl;
    
    if (score >= 0.0f && score <= 1.0f) {
        std::cout << "Routing decision: " << (score >= 0.08f ? "B-BOOK" : "A-BOOK") << std::endl;
        std::cout << "🎯 ML SERVICE IS WORKING!" << std::endl;
    } else {
        std::cout << "Score seems invalid - might be error code or different format" << std::endl;
    }
    
    return 0;
} 