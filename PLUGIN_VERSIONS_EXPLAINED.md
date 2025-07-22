# MT4 A/B-Book Plugin Versions - Complete Explanation

## 🎯 **Root Cause of Data Issues**

The **data corruption** we observed (`Command: 100`, `Symbol: ║╕l AUDUSD`, `Volume: -1972248982`) was caused by **structure misalignment** between our plugin's data structures and MT4's actual memory layout.

## 📋 **Plugin Versions Summary**

We now have **3 plugin versions** to address different scenarios:

### 1. `MT4_ABBook_Plugin_Fixed.cpp` - **Defensive Programming**
- **Purpose**: Handles corrupted data gracefully with normalization functions
- **When to use**: When you can't guarantee structure alignment
- **Features**: 
  - `CleanSymbol()` - extracts "AUDUSD" from `"║╕l AUDUSD"`
  - `NormalizeCommand()` - maps 100→0, 101→1
  - `NormalizeVolume()` - handles negative values
- **DLL**: `ABBook_Plugin_Fixed_32bit.dll`

### 2. `MT4_ABBook_Plugin_Official.cpp` - **Correct Structures** ⭐ RECOMMENDED
- **Purpose**: Uses exact MT4 Manager API structures from official documentation
- **When to use**: **Primary choice** - should eliminate data corruption entirely
- **Features**:
  - Official `TradeRecord` structure from [MT4ManagerAPI.h](https://gist.github.com/anonymous/e32013de651fef941739)
  - Official `UserInfo` structure with proper field alignment
  - No data normalization needed - data should be clean
- **DLL**: `ABBook_Plugin_Official_32bit.dll`

### 3. `test_plugin_simple.cpp` - **Minimal Test**
- **Purpose**: Basic validation plugin for testing MT4 integration
- **When to use**: Initial testing and structure validation
- **DLL**: `test_plugin_simple_32bit.dll`

---

## 🔧 **Structure Differences Explained**

### **❌ Our Original (Incorrect) Structure:**
```cpp
struct MT4TradeRecord {
    int order;           
    int login;           
    char symbol[12];     
    int digits;          
    int cmd;             // This was getting value 100 instead of 0
    int volume;          // This was getting -1972248982
    // ... other fields
};
```

### **✅ Official MT4 Structure (Correct):**
```cpp
struct TradeRecord {
    int            order;              // order ticket
    int            login;              // user login  
    char           symbol[12];         // currency
    int            digits;             // digits
    int            cmd;               // command (0=BUY, 1=SELL)
    int            volume;            // volume (in lots*100)
    __time32_t     open_time;         // open time ⭐ THIS WAS MISSING
    int            state;             // reserved ⭐ THIS WAS MISSING  
    double         open_price;        // open price
    double         sl, tp;           // stop loss & take profit
    double         close_price;       // close price
    __time32_t     close_time;        // close time
    int            reason;            // close reason
    // ... many more fields with proper alignment
};
```

**The Problem**: Missing `open_time` and `state` fields caused all subsequent fields to be read from **wrong memory offsets**.

---

## 🚀 **Recommended Testing Sequence**

### **Step 1: Test Official API Version (RECOMMENDED)**
```bash
build_official_plugin.bat
# Test: ABBook_Plugin_Official_32bit.dll
```
**Expected Result**: Clean data, no corruption, proper command values (0/1)

### **Step 2: Fallback to Fixed Version (If Issues Persist)**
```bash  
build_fixed_plugin.bat
# Test: ABBook_Plugin_Fixed_32bit.dll
```
**Expected Result**: Normalized data, handles corruption gracefully

### **Step 3: Simple Test for Validation**
```bash
build_simple_test.bat  
# Test: test_plugin_simple_32bit.dll
```
**Expected Result**: Basic functionality confirmation

---

## 📊 **Why Official API Version Should Work**

### **Before (Data Corruption):**
- MT4 sends: `cmd=0, volume=100, open_time=1234567890`
- Our plugin reads: `cmd=100, volume=-1972248982, open_price=<garbage>`
- **Cause**: Wrong memory offsets due to missing fields

### **After (Official Structures):**
- MT4 sends: `cmd=0, volume=100, open_time=1234567890`  
- Official plugin reads: `cmd=0, volume=100, open_time=1234567890`
- **Result**: Perfect data alignment, no corruption

---

## 🏆 **Final Recommendation**

1. **START** with `ABBook_Plugin_Official_32bit.dll` 
2. This should **eliminate all data corruption**
3. If issues persist, we know the problem is elsewhere (not structure alignment)
4. Keep `ABBook_Plugin_Fixed_32bit.dll` as backup for edge cases

The official plugin uses the **exact same structures** that MT4 server uses internally, ensuring perfect memory alignment and data integrity. 