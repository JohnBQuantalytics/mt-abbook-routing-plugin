/*
 * ABBook_ProtobufLib.cpp
 * Advanced Protobuf handling DLL for MT4/MT5 A/B-Book Router
 * 
 * This DLL provides proper protobuf encoding/decoding functionality
 * for production use. The main EA uses simplified JSON for demo purposes.
 */

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

#ifdef __cplusplus
extern "C" {
#endif

// Protobuf wire types
enum WireType {
    VARINT = 0,
    FIXED64 = 1,
    LENGTH_DELIMITED = 2,
    START_GROUP = 3,
    END_GROUP = 4,
    FIXED32 = 5
};

// Structure to hold scoring request data
struct ScoringRequestData {
    float open_price;                   // 1
    float sl;                          // 2  
    float tp;                          // 3
    uint32_t deal_type;                // 4
    float lot_volume;                  // 5
    int32_t is_bonus;                  // 6
    float turnover_usd;                // 7
    float opening_balance;             // 8
    int32_t concurrent_positions;      // 9
    float sl_perc;                     // 10
    float tp_perc;                     // 11
    int32_t has_sl;                    // 12
    int32_t has_tp;                    // 13
    float profitable_ratio;            // 14
    float num_open_trades;             // 15
    float num_closed_trades;           // 16
    float age;                         // 17
    float days_since_reg;              // 18
    float deposit_lifetime;            // 19
    float deposit_count;               // 20
    float withdraw_lifetime;           // 21
    float withdraw_count;              // 22
    float vip;                         // 23
    float holding_time_sec;            // 24
    float lot_usd_value;               // 25
    float exposure_to_balance_ratio;   // 26
    uint32_t rapid_entry_exit;         // 27
    uint32_t abuse_risk_score;         // 28
    float trader_tenure_days;          // 29
    float deposit_to_withdraw_ratio;   // 30
    int64_t education_known;           // 31
    int64_t occupation_known;          // 32
    float lot_to_balance_ratio;        // 33
    float deposit_density;             // 34
    float withdrawal_density;          // 35
    float turnover_per_trade;          // 36
    wchar_t symbol[32];                // 37
    wchar_t inst_group[32];            // 38
    wchar_t frequency[32];             // 39
    wchar_t trading_group[32];         // 40
    wchar_t licence[16];               // 41
    wchar_t platform[16];              // 42
    wchar_t level_of_education[64];    // 43
    wchar_t occupation[64];            // 44
    wchar_t source_of_wealth[32];      // 45
    wchar_t annual_disposable_income[32]; // 46
    wchar_t average_frequency_of_trades[32]; // 47
    wchar_t employment_status[32];     // 48
    wchar_t country_code[8];           // 49
    wchar_t utm_medium[32];            // 50
    wchar_t user_id[64];               // 51
};

// Helper functions for protobuf encoding
void EncodeVarint(std::vector<uint8_t>& buffer, uint64_t value) {
    while (value >= 0x80) {
        buffer.push_back((uint8_t)(value | 0x80));
        value >>= 7;
    }
    buffer.push_back((uint8_t)value);
}

void EncodeTag(std::vector<uint8_t>& buffer, uint32_t field_number, WireType wire_type) {
    uint32_t tag = (field_number << 3) | wire_type;
    EncodeVarint(buffer, tag);
}

void EncodeFloat(std::vector<uint8_t>& buffer, uint32_t field_number, float value) {
    EncodeTag(buffer, field_number, FIXED32);
    uint32_t int_value = *(uint32_t*)&value;
    buffer.push_back((uint8_t)(int_value & 0xFF));
    buffer.push_back((uint8_t)((int_value >> 8) & 0xFF));
    buffer.push_back((uint8_t)((int_value >> 16) & 0xFF));
    buffer.push_back((uint8_t)((int_value >> 24) & 0xFF));
}

void EncodeInt32(std::vector<uint8_t>& buffer, uint32_t field_number, int32_t value) {
    if (value >= 0) {
        EncodeTag(buffer, field_number, VARINT);
        EncodeVarint(buffer, (uint32_t)value);
    } else {
        EncodeTag(buffer, field_number, VARINT);
        EncodeVarint(buffer, (uint64_t)value);
    }
}

void EncodeUInt32(std::vector<uint8_t>& buffer, uint32_t field_number, uint32_t value) {
    EncodeTag(buffer, field_number, VARINT);
    EncodeVarint(buffer, value);
}

void EncodeInt64(std::vector<uint8_t>& buffer, uint32_t field_number, int64_t value) {
    EncodeTag(buffer, field_number, VARINT);
    EncodeVarint(buffer, (uint64_t)value);
}

void EncodeString(std::vector<uint8_t>& buffer, uint32_t field_number, const wchar_t* str) {
    // Convert wide string to UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
    if (utf8_len <= 1) return; // Empty string
    
    std::string utf8_str(utf8_len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, str, -1, &utf8_str[0], utf8_len - 1, nullptr, nullptr);
    
    EncodeTag(buffer, field_number, LENGTH_DELIMITED);
    EncodeVarint(buffer, utf8_str.length());
    buffer.insert(buffer.end(), utf8_str.begin(), utf8_str.end());
}

// Main encoding function
__declspec(dllexport) int __stdcall EncodeProtobufRequest(
    const ScoringRequestData* request,
    uint8_t* output_buffer,
    int buffer_size
) {
    if (!request || !output_buffer || buffer_size <= 0) {
        return -1; // Invalid parameters
    }
    
    std::vector<uint8_t> buffer;
    buffer.reserve(2048); // Reserve space for efficiency
    
    try {
        // Encode all numeric fields
        EncodeFloat(buffer, 1, request->open_price);
        EncodeFloat(buffer, 2, request->sl);
        EncodeFloat(buffer, 3, request->tp);
        EncodeUInt32(buffer, 4, request->deal_type);
        EncodeFloat(buffer, 5, request->lot_volume);
        EncodeInt32(buffer, 6, request->is_bonus);
        EncodeFloat(buffer, 7, request->turnover_usd);
        EncodeFloat(buffer, 8, request->opening_balance);
        EncodeInt32(buffer, 9, request->concurrent_positions);
        EncodeFloat(buffer, 10, request->sl_perc);
        EncodeFloat(buffer, 11, request->tp_perc);
        EncodeInt32(buffer, 12, request->has_sl);
        EncodeInt32(buffer, 13, request->has_tp);
        EncodeFloat(buffer, 14, request->profitable_ratio);
        EncodeFloat(buffer, 15, request->num_open_trades);
        EncodeFloat(buffer, 16, request->num_closed_trades);
        EncodeFloat(buffer, 17, request->age);
        EncodeFloat(buffer, 18, request->days_since_reg);
        EncodeFloat(buffer, 19, request->deposit_lifetime);
        EncodeFloat(buffer, 20, request->deposit_count);
        EncodeFloat(buffer, 21, request->withdraw_lifetime);
        EncodeFloat(buffer, 22, request->withdraw_count);
        EncodeFloat(buffer, 23, request->vip);
        EncodeFloat(buffer, 24, request->holding_time_sec);
        EncodeFloat(buffer, 25, request->lot_usd_value);
        EncodeFloat(buffer, 26, request->exposure_to_balance_ratio);
        EncodeUInt32(buffer, 27, request->rapid_entry_exit);
        EncodeUInt32(buffer, 28, request->abuse_risk_score);
        EncodeFloat(buffer, 29, request->trader_tenure_days);
        EncodeFloat(buffer, 30, request->deposit_to_withdraw_ratio);
        EncodeInt64(buffer, 31, request->education_known);
        EncodeInt64(buffer, 32, request->occupation_known);
        EncodeFloat(buffer, 33, request->lot_to_balance_ratio);
        EncodeFloat(buffer, 34, request->deposit_density);
        EncodeFloat(buffer, 35, request->withdrawal_density);
        EncodeFloat(buffer, 36, request->turnover_per_trade);
        
        // Encode string fields
        EncodeString(buffer, 37, request->symbol);
        EncodeString(buffer, 38, request->inst_group);
        EncodeString(buffer, 39, request->frequency);
        EncodeString(buffer, 40, request->trading_group);
        EncodeString(buffer, 41, request->licence);
        EncodeString(buffer, 42, request->platform);
        EncodeString(buffer, 43, request->level_of_education);
        EncodeString(buffer, 44, request->occupation);
        EncodeString(buffer, 45, request->source_of_wealth);
        EncodeString(buffer, 46, request->annual_disposable_income);
        EncodeString(buffer, 47, request->average_frequency_of_trades);
        EncodeString(buffer, 48, request->employment_status);
        EncodeString(buffer, 49, request->country_code);
        EncodeString(buffer, 50, request->utm_medium);
        EncodeString(buffer, 51, request->user_id);
        
        // Copy to output buffer
        if ((int)buffer.size() > buffer_size) {
            return -2; // Buffer too small
        }
        
        memcpy(output_buffer, buffer.data(), buffer.size());
        return (int)buffer.size();
        
    } catch (...) {
        return -3; // Encoding error
    }
}

// Decode response helper functions
uint64_t DecodeVarint(const uint8_t*& data, const uint8_t* end) {
    uint64_t result = 0;
    int shift = 0;
    
    while (data < end && shift < 64) {
        uint8_t byte = *data++;
        result |= ((uint64_t)(byte & 0x7F)) << shift;
        if ((byte & 0x80) == 0) {
            break;
        }
        shift += 7;
    }
    
    return result;
}

float DecodeFloat(const uint8_t*& data, const uint8_t* end) {
    if (data + 4 > end) return 0.0f;
    
    uint32_t int_value = 0;
    int_value |= (uint32_t)data[0];
    int_value |= (uint32_t)data[1] << 8;
    int_value |= (uint32_t)data[2] << 16;
    int_value |= (uint32_t)data[3] << 24;
    data += 4;
    
    return *(float*)&int_value;
}

// Response decoding function
__declspec(dllexport) int __stdcall DecodeProtobufResponse(
    const uint8_t* input_buffer,
    int buffer_size,
    float* score,
    wchar_t* warnings,
    int warnings_size
) {
    if (!input_buffer || buffer_size <= 0 || !score) {
        return -1; // Invalid parameters
    }
    
    const uint8_t* data = input_buffer;
    const uint8_t* end = input_buffer + buffer_size;
    
    *score = 0.0f;
    if (warnings && warnings_size > 0) {
        warnings[0] = L'\0';
    }
    
    try {
        while (data < end) {
            uint64_t tag = DecodeVarint(data, end);
            uint32_t field_number = (uint32_t)(tag >> 3);
            WireType wire_type = (WireType)(tag & 0x7);
            
            switch (field_number) {
                case 1: // score
                    if (wire_type == FIXED32) {
                        *score = DecodeFloat(data, end);
                    }
                    break;
                    
                case 2: // warnings (repeated string)
                    if (wire_type == LENGTH_DELIMITED) {
                        uint64_t length = DecodeVarint(data, end);
                        if (data + length <= end) {
                            // Convert first warning to wide string
                            if (warnings && warnings_size > 0) {
                                int wide_len = MultiByteToWideChar(CP_UTF8, 0, (char*)data, (int)length, warnings, warnings_size - 1);
                                if (wide_len > 0) {
                                    warnings[wide_len] = L'\0';
                                }
                            }
                            data += length;
                        }
                    }
                    break;
                    
                default:
                    // Skip unknown fields
                    switch (wire_type) {
                        case VARINT:
                            DecodeVarint(data, end);
                            break;
                        case FIXED64:
                            data += 8;
                            break;
                        case LENGTH_DELIMITED: {
                            uint64_t length = DecodeVarint(data, end);
                            data += length;
                            break;
                        }
                        case FIXED32:
                            data += 4;
                            break;
                        default:
                            return -2; // Invalid wire type
                    }
                    break;
            }
        }
        
        return 0; // Success
        
    } catch (...) {
        return -3; // Decoding error
    }
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

#ifdef __cplusplus
}
#endif 