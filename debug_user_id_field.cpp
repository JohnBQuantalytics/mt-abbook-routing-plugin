//+------------------------------------------------------------------+
//| Debug User ID Field Encoding - Check Wire Type Issue           |
//+------------------------------------------------------------------+

#include <iostream>
#include <string>
#include <iomanip>

class ProtobufDebugger {
private:
    std::string EncodeVarint(uint64_t value) {
        std::string result;
        while (value >= 0x80) {
            result += (char)((value & 0x7F) | 0x80);
            value >>= 7;
        }
        result += (char)(value & 0x7F);
        return result;
    }
    
    std::string EncodeString(int field_number, const std::string& value) {
        std::string result;
        result += (char)((field_number << 3) | 2); // Wire type 2 for length-delimited
        result += EncodeVarint(value.length());
        result += value;
        return result;
    }
    
    std::string EncodeFloat(int field_number, float value) {
        std::string result;
        result += (char)((field_number << 3) | 5); // Wire type 5 for fixed32
        char* bytes = (char*)&value;
        for (int i = 0; i < 4; i++) {
            result += bytes[i];
        }
        return result;
    }
    
    void PrintHex(const std::string& data, const std::string& label) {
        std::cout << label << ": ";
        for (size_t i = 0; i < data.length(); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << (unsigned char)data[i] << " ";
        }
        std::cout << std::dec << " (" << data.length() << " bytes)" << std::endl;
    }
    
public:
    void DebugUserIdField() {
        std::cout << "=== USER ID FIELD ENCODING DEBUG ===" << std::endl;
        std::cout << "Testing field 51 (user_id) encoding" << std::endl;
        std::cout << std::endl;
        
        // Test user_id as string (correct)
        std::string user_id_string = EncodeString(51, "16813");
        PrintHex(user_id_string, "Field 51 as STRING");
        
        // Test what happens if we accidentally encode as float
        std::string user_id_float = EncodeFloat(51, 16813.0f);
        PrintHex(user_id_float, "Field 51 as FLOAT (wrong)");
        
        std::cout << std::endl;
        std::cout << "Wire Type Analysis:" << std::endl;
        std::cout << "- STRING (correct): First byte = 0x" << std::hex 
                  << (unsigned char)user_id_string[0] << std::dec 
                  << " = field 51, wire type 2 (LengthDelimited)" << std::endl;
        std::cout << "- FLOAT (wrong): First byte = 0x" << std::hex 
                  << (unsigned char)user_id_float[0] << std::dec 
                  << " = field 51, wire type 5 (ThirtyTwoBit)" << std::endl;
        
        // Check field number calculation
        int field_51_wire_2 = (51 << 3) | 2; // String
        int field_51_wire_5 = (51 << 3) | 5; // Float
        
        std::cout << std::endl;
        std::cout << "Expected bytes:" << std::endl;
        std::cout << "- Field 51, wire type 2 (string): 0x" << std::hex << field_51_wire_2 << std::dec << std::endl;
        std::cout << "- Field 51, wire type 5 (float): 0x" << std::hex << field_51_wire_5 << std::dec << std::endl;
    }
    
    void TestCompleteMessage() {
        std::cout << std::endl;
        std::cout << "=== COMPLETE MESSAGE FIELD SCAN ===" << std::endl;
        
        // Create a minimal message with just key fields
        std::string message;
        
        message += EncodeFloat(1, 0.59350f);        // open_price
        message += EncodeString(51, "16813");       // user_id
        
        std::cout << "Minimal message with open_price + user_id:" << std::endl;
        PrintHex(message, "Complete message");
        
        // Break down byte by byte
        std::cout << std::endl;
        std::cout << "Byte-by-byte analysis:" << std::endl;
        for (size_t i = 0; i < message.length(); i++) {
            unsigned char byte = message[i];
            int field_num = byte >> 3;
            int wire_type = byte & 0x07;
            
            if (field_num > 0) {
                std::cout << "Byte " << i << ": 0x" << std::hex << (int)byte << std::dec 
                          << " = Field " << field_num << ", Wire type " << wire_type;
                
                if (field_num == 1 && wire_type == 5) std::cout << " (open_price FLOAT - OK)";
                if (field_num == 51 && wire_type == 2) std::cout << " (user_id STRING - OK)";
                if (field_num == 51 && wire_type == 5) std::cout << " (user_id FLOAT - ERROR!)";
                
                std::cout << std::endl;
            }
        }
    }
};

int main() {
    std::cout << "MT4 A/B-book Plugin - User ID Field Debug" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Investigating wire type issue reported by ML service" << std::endl;
    std::cout << std::endl;
    
    ProtobufDebugger debugger;
    debugger.DebugUserIdField();
    debugger.TestCompleteMessage();
    
    std::cout << std::endl;
    std::cout << "Press any key to exit...";
    std::cin.get();
    return 0;
} 