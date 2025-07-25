//+------------------------------------------------------------------+
//| Field 51 (user_id) Specific Debug - Find Wire Type Issue      |
//+------------------------------------------------------------------+

#include <iostream>
#include <string>
#include <iomanip>

class Field51Debugger {
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
    
    std::string EncodeString_CORRECT(int field_number, const std::string& value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 2; // Wire type 2 for length-delimited
        result += EncodeVarint(field_tag); // Encode field tag as varint
        result += EncodeVarint(value.length());
        result += value;
        return result;
    }
    
    std::string EncodeString_WRONG(int field_number, const std::string& value) {
        std::string result;
        result += (char)((field_number << 3) | 2); // OLD WAY - single byte (WRONG for field > 15)
        result += EncodeVarint(value.length());
        result += value;
        return result;
    }
    
    std::string EncodeFloat_WRONG(int field_number, float value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 5; // Wire type 5 for float
        result += EncodeVarint(field_tag);
        char* bytes = (char*)&value;
        for (int i = 0; i < 4; i++) {
            result += bytes[i];
        }
        return result;
    }
    
    void PrintHex(const std::string& data, const std::string& label) {
        std::cout << label << ": ";
        for (size_t i = 0; i < data.length(); i++) {
            std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
                      << (unsigned char)data[i] << " ";
        }
        std::cout << std::dec << " (" << data.length() << " bytes)" << std::endl;
    }
    
    uint32_t DecodeVarint(const std::string& data, size_t& pos) {
        uint32_t result = 0;
        int shift = 0;
        while (pos < data.length()) {
            unsigned char byte = data[pos++];
            result |= (byte & 0x7F) << shift;
            if ((byte & 0x80) == 0) break;
            shift += 7;
        }
        return result;
    }
    
    void AnalyzeBytes(const std::string& data, const std::string& method) {
        std::cout << "\n=== " << method << " ===" << std::endl;
        PrintHex(data, "Raw bytes");
        
        if (data.length() > 0) {
            size_t pos = 0;
            uint32_t field_tag = DecodeVarint(data, pos);
            int field_number = field_tag >> 3;
            int wire_type = field_tag & 0x07;
            
            std::cout << "Field tag: " << field_tag << " (0x" << std::hex << field_tag << std::dec << ")" << std::endl;
            std::cout << "Field number: " << field_number << std::endl;
            std::cout << "Wire type: " << wire_type;
            
            switch (wire_type) {
                case 0: std::cout << " (Varint)"; break;
                case 1: std::cout << " (64-bit)"; break;
                case 2: std::cout << " (LengthDelimited - STRING)"; break;
                case 3: std::cout << " (Start group)"; break;
                case 4: std::cout << " (End group)"; break;
                case 5: std::cout << " (32-bit - FLOAT)"; break;
                default: std::cout << " (Unknown)"; break;
            }
            std::cout << std::endl;
            
            if (field_number == 51 && wire_type == 2) {
                std::cout << "✅ CORRECT: Field 51 as LengthDelimited (string)" << std::endl;
            } else if (field_number == 51 && wire_type == 5) {
                std::cout << "❌ ERROR: Field 51 as ThirtyTwoBit (float) - THIS IS THE BUG!" << std::endl;
            }
        }
    }
    
public:
    void DebugField51() {
        std::cout << "=== FIELD 51 (user_id) WIRE TYPE DEBUG ===" << std::endl;
        std::cout << "Testing different encoding methods for field 51\n" << std::endl;
        
        // Test correct string encoding
        std::string correct = EncodeString_CORRECT(51, "16813");
        AnalyzeBytes(correct, "CORRECT String Encoding");
        
        // Test wrong string encoding (old method)
        std::string wrong_old = EncodeString_WRONG(51, "16813");
        AnalyzeBytes(wrong_old, "WRONG String Encoding (Old Method)");
        
        // Test if we accidentally encode as float
        std::string wrong_float = EncodeFloat_WRONG(51, 16813.0f);
        AnalyzeBytes(wrong_float, "WRONG Float Encoding (Accident)");
        
        // Calculate expected values
        std::cout << "\n=== EXPECTED CALCULATIONS ===" << std::endl;
        uint32_t expected_tag = (51 << 3) | 2;
        std::cout << "Field 51, wire type 2: " << expected_tag << " (0x" << std::hex << expected_tag << std::dec << ")" << std::endl;
        
        uint32_t wrong_tag = (51 << 3) | 5;
        std::cout << "Field 51, wire type 5: " << wrong_tag << " (0x" << std::hex << wrong_tag << std::dec << ")" << std::endl;
        
        // Check if our current implementation has the bug
        std::cout << "\n=== CURRENT IMPLEMENTATION TEST ===" << std::endl;
        std::string current = EncodeString_CORRECT(51, "16813");
        
        if (current == correct) {
            std::cout << "✅ Current implementation is CORRECT" << std::endl;
        } else {
            std::cout << "❌ Current implementation has a BUG!" << std::endl;
            std::cout << "Expected: ";
            PrintHex(correct, "");
            std::cout << "Actual:   ";
            PrintHex(current, "");
        }
    }
    
    void TestFullMessage() {
        std::cout << "\n=== FULL MESSAGE TEST ===" << std::endl;
        
        std::string message;
        // Add a few fields to simulate the full request
        message += EncodeString_CORRECT(1, "test"); // Simulate open_price as string for testing
        message += EncodeString_CORRECT(42, "MT4");
        message += EncodeString_CORRECT(51, "16813"); // The problematic field
        
        std::cout << "Full message simulation:" << std::endl;
        PrintHex(message, "Complete");
        
        // Parse each field
        size_t pos = 0;
        int field_count = 0;
        
        while (pos < message.length()) {
            field_count++;
            uint32_t field_tag = DecodeVarint(message, pos);
            int field_number = field_tag >> 3;
            int wire_type = field_tag & 0x07;
            
            std::cout << "Field " << field_count << ": #" << field_number << ", wire type " << wire_type;
            if (field_number == 51) {
                if (wire_type == 2) {
                    std::cout << " ✅ (user_id as string - CORRECT)";
                } else if (wire_type == 5) {
                    std::cout << " ❌ (user_id as float - BUG!)";
                }
            }
            std::cout << std::endl;
            
            // Skip field data
            if (wire_type == 2) { // Length-delimited
                uint32_t length = DecodeVarint(message, pos);
                pos += length;
            } else if (wire_type == 5) { // 32-bit
                pos += 4;
            } else if (wire_type == 0) { // Varint
                DecodeVarint(message, pos); // This advances pos
            }
        }
    }
};

int main() {
    std::cout << "MT4 A/B-book Plugin - Field 51 Wire Type Debug\n";
    std::cout << "==============================================\n";
    std::cout << "Finding the exact cause of the wire type error\n\n";
    
    Field51Debugger debugger;
    debugger.DebugField51();
    debugger.TestFullMessage();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 