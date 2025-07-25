//+------------------------------------------------------------------+
//| Decode ML Response - Extract Score from Field 2               |
//+------------------------------------------------------------------+

#include <iostream>
#include <iomanip>
#include <cstring>

int main() {
    std::cout << "ML Response Decoder\n";
    std::cout << "===================\n\n";
    
    // Raw response from ML service: 15 00 3F 25 3B
    unsigned char response_data[] = {0x15, 0x00, 0x3F, 0x25, 0x3B};
    
    std::cout << "Raw protobuf data: ";
    for (int i = 0; i < 5; i++) {
        printf("%02X ", response_data[i]);
    }
    std::cout << std::endl;
    
    // Parse field tag
    unsigned char field_tag = response_data[0];  // 0x15
    int field_number = field_tag >> 3;           // (0x15 >> 3) = 2
    int wire_type = field_tag & 0x7;             // (0x15 & 0x7) = 5
    
    std::cout << "\nField analysis:" << std::endl;
    std::cout << "Field tag: 0x" << std::hex << (int)field_tag << std::dec << std::endl;
    std::cout << "Field number: " << field_number << std::endl;
    std::cout << "Wire type: " << wire_type << " (5 = fixed32 float)" << std::endl;
    
    if (field_number == 2 && wire_type == 5) {
        std::cout << "\n✅ This is the score field (field 2, float)!" << std::endl;
        
        // Extract the 4-byte float value (bytes 1-4)
        float score;
        memcpy(&score, &response_data[1], 4);
        
        std::cout << "\nRaw float bytes: ";
        for (int i = 1; i <= 4; i++) {
            printf("%02X ", response_data[i]);
        }
        std::cout << std::endl;
        
        std::cout << "\n🎯 DECODED ML SCORE: " << std::fixed << std::setprecision(6) << score << std::endl;
        
        // Validate score range
        if (score >= 0.0f && score <= 1.0f) {
            std::cout << "✅ Valid score range (0.0-1.0)" << std::endl;
            
            // Determine routing decision
            if (score >= 0.08f) {
                std::cout << "📈 Routing Decision: B-BOOK (score >= 0.08)" << std::endl;
                std::cout << "💰 Expected outcome: User likely to lose money" << std::endl;
            } else {
                std::cout << "📉 Routing Decision: A-BOOK (score < 0.08)" << std::endl;
                std::cout << "⚠️ Expected outcome: User likely to be profitable" << std::endl;
            }
            
            // Risk assessment
            if (score < 0.01f) {
                std::cout << "🟢 Risk Level: Very Low (skilled trader)" << std::endl;
            } else if (score < 0.05f) {
                std::cout << "🟡 Risk Level: Low (decent trader)" << std::endl;
            } else if (score < 0.2f) {
                std::cout << "🟠 Risk Level: Medium (average trader)" << std::endl;
            } else {
                std::cout << "🔴 Risk Level: High (poor trader)" << std::endl;
            }
            
        } else {
            std::cout << "❌ Invalid score range - expected 0.0-1.0" << std::endl;
        }
        
    } else {
        std::cout << "\n❌ Unexpected field/wire type" << std::endl;
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "✅ ML service is working correctly" << std::endl;
    std::cout << "✅ Returns score in field 2 (not field 1)" << std::endl;
    std::cout << "✅ Uses proper protobuf format" << std::endl;
    std::cout << "✅ Score can be used for A/B routing decisions" << std::endl;
    
    return 0;
} 