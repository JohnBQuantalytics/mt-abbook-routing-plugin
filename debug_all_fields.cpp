//+------------------------------------------------------------------+
//| Complete Field Analysis - Find ALL Wire Type Issues           |
//+------------------------------------------------------------------+

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

class CompleteFieldAnalyzer {
private:
    std::string ml_ip = "188.245.254.12";
    int ml_port = 50051;
    
    struct FieldInfo {
        int number;
        std::string name;
        std::string type;
        std::string value;
    };
    
    std::string EncodeVarint(uint64_t value) {
        std::string result;
        while (value >= 0x80) {
            result += (char)((value & 0x7F) | 0x80);
            value >>= 7;
        }
        result += (char)(value & 0x7F);
        return result;
    }
    
    std::string EncodeFloat(int field_number, float value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 5;
        result += EncodeVarint(field_tag);
        char* bytes = (char*)&value;
        for (int i = 0; i < 4; i++) {
            result += bytes[i];
        }
        return result;
    }
    
    std::string EncodeUInt32(int field_number, uint32_t value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 0;
        result += EncodeVarint(field_tag);
        result += EncodeVarint(value);
        return result;
    }
    
    std::string EncodeInt32(int field_number, int32_t value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 0;
        result += EncodeVarint(field_tag);
        result += EncodeVarint((uint64_t)value);
        return result;
    }
    
    std::string EncodeString(int field_number, const std::string& value) {
        std::string result;
        uint32_t field_tag = (field_number << 3) | 2;
        result += EncodeVarint(field_tag);
        result += EncodeVarint(value.length());
        result += value;
        return result;
    }
    
    void AnalyzeField(const std::string& data, const FieldInfo& info) {
        std::cout << "Field " << info.number << " (" << info.name << "):" << std::endl;
        std::cout << "  Type: " << info.type << ", Value: " << info.value << std::endl;
        std::cout << "  Bytes: ";
        for (size_t i = 0; i < data.length() && i < 16; i++) {
            printf("%02X ", (unsigned char)data[i]);
        }
        if (data.length() > 16) std::cout << "...";
        std::cout << " (" << data.length() << " bytes)" << std::endl;
        
        // Decode field tag
        if (data.length() > 0) {
            size_t pos = 0;
            uint32_t field_tag = 0;
            int shift = 0;
            while (pos < data.length()) {
                unsigned char byte = data[pos++];
                field_tag |= (byte & 0x7F) << shift;
                if ((byte & 0x80) == 0) break;
                shift += 7;
            }
            
            int field_number = field_tag >> 3;
            int wire_type = field_tag & 0x07;
            
            std::cout << "  Decoded: Field " << field_number << ", wire type " << wire_type;
            
            bool error = false;
            if (info.type == "float" && wire_type != 5) {
                std::cout << " ❌ SHOULD BE 5 (float)";
                error = true;
            } else if (info.type == "uint32" && wire_type != 0) {
                std::cout << " ❌ SHOULD BE 0 (varint)";
                error = true;
            } else if (info.type == "int32" && wire_type != 0) {
                std::cout << " ❌ SHOULD BE 0 (varint)";
                error = true;
            } else if (info.type == "string" && wire_type != 2) {
                std::cout << " ❌ SHOULD BE 2 (string)";
                error = true;
            } else {
                std::cout << " ✅ CORRECT";
            }
            
            if (error) {
                std::cout << " <-- THIS IS THE BUG!" << std::endl;
            }
        }
        std::cout << std::endl;
    }
    
    std::string CreateCompleteRequest() {
        std::string request;
        std::vector<FieldInfo> fields;
        
        // Build all fields exactly as in our plugin
        std::cout << "=== BUILDING COMPLETE SCORING REQUEST ===" << std::endl;
        
        // Numeric fields
        fields.push_back({1, "open_price", "float", "0.59350"});
        request += EncodeFloat(1, 0.59350f);
        
        fields.push_back({2, "sl", "float", "0.59000"});
        request += EncodeFloat(2, 0.59000f);
        
        fields.push_back({3, "tp", "float", "0.59700"});
        request += EncodeFloat(3, 0.59700f);
        
        fields.push_back({4, "deal_type", "uint32", "1"});
        request += EncodeUInt32(4, 1);
        
        fields.push_back({5, "lot_volume", "float", "1.0"});
        request += EncodeFloat(5, 1.0f);
        
        fields.push_back({6, "is_bonus", "int32", "0"});
        request += EncodeInt32(6, 0);
        
        fields.push_back({7, "turnover_usd", "float", "59350.0"});
        request += EncodeFloat(7, 59350.0f);
        
        fields.push_back({8, "opening_balance", "float", "10000.0"});
        request += EncodeFloat(8, 10000.0f);
        
        fields.push_back({9, "concurrent_positions", "int32", "2"});
        request += EncodeInt32(9, 2);
        
        fields.push_back({10, "sl_perc", "float", "0.59"});
        request += EncodeFloat(10, 0.59f);
        
        fields.push_back({11, "tp_perc", "float", "0.59"});
        request += EncodeFloat(11, 0.59f);
        
        fields.push_back({12, "has_sl", "int32", "1"});
        request += EncodeInt32(12, 1);
        
        // String fields
        fields.push_back({42, "platform", "string", "MT4"});
        request += EncodeString(42, "MT4");
        
        fields.push_back({43, "LEVEL_OF_EDUCATION", "string", "bachelor"});
        request += EncodeString(43, "bachelor");
        
        fields.push_back({44, "OCCUPATION", "string", "engineer"});
        request += EncodeString(44, "engineer");
        
        fields.push_back({45, "SOURCE_OF_WEALTH", "string", "salary"});
        request += EncodeString(45, "salary");
        
        fields.push_back({46, "ANNUAL_DISPOSABLE_INCOME", "string", "50k-100k"});
        request += EncodeString(46, "50k-100k");
        
        fields.push_back({47, "AVERAGE_FREQUENCY_OF_TRADES", "string", "weekly"});
        request += EncodeString(47, "weekly");
        
        fields.push_back({48, "EMPLOYMENT_STATUS", "string", "employed"});
        request += EncodeString(48, "employed");
        
        fields.push_back({49, "country_code", "string", "US"});
        request += EncodeString(49, "US");
        
        fields.push_back({50, "utm_medium", "string", "cpc"});
        request += EncodeString(50, "cpc");
        
        fields.push_back({51, "user_id", "string", "16813"});
        request += EncodeString(51, "16813");
        
        // Now analyze each field
        std::cout << "\n=== ANALYZING EACH FIELD ===" << std::endl;
        
        size_t pos = 0;
        for (size_t i = 0; i < fields.size(); i++) {
            // Extract this field's bytes
            size_t field_start = pos;
            
            // Skip field tag
            while (pos < request.length() && (request[pos] & 0x80)) pos++;
            if (pos < request.length()) pos++; // Skip last tag byte
            
            // Skip field data based on wire type
            if (fields[i].type == "float") {
                pos += 4; // 4 bytes for float
            } else if (fields[i].type == "uint32" || fields[i].type == "int32") {
                // Skip varint
                while (pos < request.length() && (request[pos] & 0x80)) pos++;
                if (pos < request.length()) pos++;
            } else if (fields[i].type == "string") {
                // Skip length varint
                size_t len_start = pos;
                while (pos < request.length() && (request[pos] & 0x80)) pos++;
                if (pos < request.length()) pos++;
                
                // Get string length
                size_t len_pos = len_start;
                uint32_t str_len = 0;
                int shift = 0;
                while (len_pos < pos) {
                    unsigned char byte = request[len_pos++];
                    str_len |= (byte & 0x7F) << shift;
                    if ((byte & 0x80) == 0) break;
                    shift += 7;
                }
                
                pos += str_len; // Skip string data
            }
            
            // Extract field bytes
            std::string field_bytes = request.substr(field_start, pos - field_start);
            AnalyzeField(field_bytes, fields[i]);
        }
        
        return request;
    }
    
public:
    void TestCompleteRequest() {
        std::cout << "MT4 A/B-book Plugin - Complete Field Analysis\n";
        std::cout << "=============================================\n";
        std::cout << "Analyzing every field to find wire type issues\n\n";
        
        std::string protobuf_request = CreateCompleteRequest();
        
        std::cout << "=== SUMMARY ===" << std::endl;
        std::cout << "Total request size: " << protobuf_request.length() << " bytes" << std::endl;
        std::cout << "All fields analyzed above. Look for ❌ to find the bug!" << std::endl;
    }
};

int main() {
    CompleteFieldAnalyzer analyzer;
    analyzer.TestCompleteRequest();
    
    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
} 