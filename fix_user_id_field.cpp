//+------------------------------------------------------------------+
//| Fix user_id Field Number - Use Working Field Number           |
//+------------------------------------------------------------------+

#include <iostream>
#include <cstdint> // Required for uint8_t
#include <cmath>   // Required for pow
#include <cstring> // Required for memset
#include <iomanip> // Required for std::hex and std::dec
#include <cassert> // Required for assert

// Assuming std::bitset is available, otherwise define it or use a library
// For this example, we'll use a placeholder or assume it's available.
// If not, you might need to define it or use a different library.
// For simplicity, we'll assume it's available or define a minimal version.
// If not, you'll get a compilation error.
// #include <cstdint> // Required for uint8_t
// #include <cmath>   // Required for pow
// #include <cstring> // Required for memset
// #include <iomanip> // Required for std::hex and std::dec
// #include <cassert> // Required for assert

// Placeholder for std::bitset if not available
// This is a simplified version and might not be fully functional
// depending on your compiler's standard library.
// For a real project, you'd use a proper bitset library.
class Bitset {
public:
    Bitset(size_t size) : size_(size), data_(new bool[size]) {
        memset(data_, 0, size);
    }
    ~Bitset() { delete[] data_; }

    void set(size_t pos) {
        if (pos < size_) {
            data_[pos] = true;
        }
    }

    bool get(size_t pos) const {
        if (pos < size_) {
            return data_[pos];
        }
        return false;
    }

    size_t size() const { return size_; }

    // Overload << for easy printing
    friend std::ostream& operator<<(std::ostream& os, const Bitset& bs) {
        for (size_t i = 0; i < bs.size_; ++i) {
            os << (bs.get(i) ? '1' : '0');
        }
        return os;
    }

private:
    size_t size_;
    bool* data_;
};

int main() {
    // From our debug, the "working" version used F2 03
    // Let's decode this properly:
    
    unsigned char byte1 = 0xF2;  // 11110010
    unsigned char byte2 = 0x03;  // 00000011
    
    std::cout << "Decoding F2 03 varint:" << std::endl;
    std::cout << "Byte 1: 0x" << std::hex << (int)byte1 << " = " << Bitset(8).set(0).set(1).set(2).set(3).set(4).set(5).set(6).set(7) << std::endl;
    std::cout << "Byte 2: 0x" << std::hex << (int)byte2 << " = " << Bitset(8).set(0).set(1).set(2).set(3).set(4).set(5).set(6).set(7) << std::endl;
    
    // Varint decoding:
    // Byte 1 has continuation bit (bit 7 = 1), take bits 0-6: 1110010 = 0x72 = 114
    // Byte 2 has no continuation bit (bit 7 = 0), take bits 0-6: 0000011 = 3
    // Combined: 114 + (3 << 7) = 114 + 384 = 498
    
    int lower_bits = byte1 & 0x7F;  // Remove continuation bit: 0x72 = 114
    int upper_bits = byte2 & 0x7F;  // Remove continuation bit: 0x03 = 3
    int field_tag = lower_bits + (upper_bits << 7);
    
    std::cout << "Lower bits: " << lower_bits << std::endl;
    std::cout << "Upper bits: " << upper_bits << std::endl;
    std::cout << "Combined field tag: " << field_tag << std::endl;
    
    int field_number = field_tag >> 3;
    int wire_type = field_tag & 0x7;
    
    std::cout << "Field number: " << field_number << std::endl;
    std::cout << "Wire type: " << wire_type << std::endl;
    
    // So the working version actually used field 62, not 60!
    std::cout << "\n🔍 DISCOVERY: The working version used field " << field_number << " (not 60)!" << std::endl;
    std::cout << "✅ SOLUTION: Change user_id from field 60 to field " << field_number << std::endl;
    
    return 0;
} 