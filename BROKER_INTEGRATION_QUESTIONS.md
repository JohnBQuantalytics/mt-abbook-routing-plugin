# Broker Integration Questions - Based on PRD

## 🎯 **Essential Questions for Broker Meeting**

### **1. Trade Data Access**
- **"Can we access trade data through OnTradeRequest() hook before execution?"**
- **"What trade data fields are available in your MT4/MT5 server?"**
- **"Can you provide ALL 51 fields required by our ML scoring model?"**

**Show them this exact list and ask what's available:**

#### **Critical Trade Fields (1-5)**
- `open_price` - Execution price at which the trade will open
- `sl` - Stop-loss price level the trader sets
- `tp` - Take-profit price level the trader sets
- `deal_type` - Trade direction: 0 = buy (long), 1 = sell (short)
- `lot_volume` - Position size in standard lots

#### **Account & Trading History (6-36)**
- `is_bonus` - Flag: 1 if bonus credit will be used, 0 if real funds
- `turnover_usd` - Total notional (USD) = open_price * lot_volume * contract_size
- `opening_balance` - Account equity immediately before opening this trade
- `concurrent_positions` - Number of other positions open at placement
- `sl_perc` - Stop-loss distance fraction = |open_price - sl| / open_price
- `tp_perc` - Take-profit distance fraction = |tp - open_price| / open_price
- `has_sl` - Indicator: 1 if a stop-loss is set, else 0
- `has_tp` - Indicator: 1 if a take-profit is set, else 0
- `profitable_ratio` - Proportion of past closed trades that were profitable
- `num_open_trades` - Number of trades currently open
- `num_closed_trades` - Total number of historical trades closed
- `age` - Trader's age in years
- `days_since_reg` - Days since registration
- `deposit_lifetime` - Total deposited over account lifetime
- `deposit_count` - Number of deposit transactions
- `withdraw_lifetime` - Total withdrawn over account lifetime
- `withdraw_count` - Number of withdrawal transactions
- `vip` - VIP status: 1 = VIP, 0 = regular
- `holding_time_sec` - Average duration of trades in the last 1–2h
- `lot_usd_value` - USD value of one lot
- `exposure_to_balance_ratio` - ratio = turnover_usd / opening_balance
- `rapid_entry_exit` - Count of trades <60s in the last 1–2h
- `abuse_risk_score` - Sum of has_sl and has_tp and not_empy rapid_entry_exit
- `trader_tenure_days` - Days since account opened
- `deposit_to_withdraw_ratio` - deposit_lifetime / max(1, withdraw_lifetime)
- `education_known` - 1 if education provided, else 0
- `occupation_known` - 1 if occupation provided, else 0
- `lot_to_balance_ratio` - (lot_volume * contract_size) / opening_balance
- `deposit_density` - deposit_count / max(1, days_since_reg)
- `withdrawal_density` - withdraw_count / max(1, days_since_reg)
- `turnover_per_trade` - turnover_usd / max(1, num_closed_trades)

#### **Profile & Metadata (37-51)**
- `symbol` - Trading symbol (e.g., "EURUSD")
- `inst_group` - Instrument group (e.g., "FXMajors")
- `frequency` - Trading frequency (e.g., "medium")
- `trading_group` - Product grouping/path code
- `licence` - Licensing jurisdiction (e.g., "CY")
- `platform` - Trading platform (e.g., "MT5")
- `LEVEL_OF_EDUCATION` - Education level (e.g., "bachelor")
- `OCCUPATION` - Occupation (e.g., "engineer")
- `SOURCE_OF_WEALTH` - Source of wealth (e.g., "salary")
- `ANNUAL_DISPOSABLE_INCOME` - Income bracket (e.g., "50k-100k")
- `AVERAGE_FREQUENCY_OF_TRADES` - Trade frequency (e.g., "weekly")
- `EMPLOYMENT_STATUS` - Employment status (e.g., "employed")
- `country_code` - Country code (e.g., "CY")
- `utm_medium` - UTM medium (e.g., "cpc")
- `user_id` - Client-provided ID for external service queries

### **2. Position Management**
- **"How do we track opening vs closing positions?"**
- **"Can we match closing trades with their original opening trades?"**
- **"What's the data structure for position tracking?"**

### **3. A-Book/B-Book Routing**
- **"How do you currently handle A-book vs B-book routing?"**
- **"What APIs are available for routing trades to:"**
  - A-book (liquidity providers)
  - B-book (internal handling)
- **"Can we programmatically route trades during the OnTradeRequest() hook?"**

### **4. Technical Integration**
- **"What's the exact function signature for trade hooks?"**
- **"Do you support standard MT4 structures or use custom ones?"**
- **"What's the maximum processing time allowed before trade execution?"**

### **5. System Requirements**
- **"What version of MT4/MT5 server are you running?"**
- **"Can our plugin make outbound TCP connections to our CVM?"**
- **"What network security requirements need to be met?"**

## 📊 **Show Them Our Architecture**

**Present this flow:**
```
Client Trade → MT4/MT5 Server → OnTradeRequest() Hook → Our Plugin → 
51-Field Feature Vector → TCP Binary Protobuf → ML Scoring Service → 
Score (float32) → Threshold Comparison → A-Book/B-Book Routing Decision
```

**Key Points:**
- **Opening positions**: Get scored by ML service (all 51 fields)
- **Closing positions**: No scoring, just match to opening position
- **Protocol**: TCP with length-prefixed binary Protobuf packets
- **Routing Logic**: 
  - **score >= threshold → B-book**
  - **score < threshold → A-book**
- **Fallback**: Configurable fallback score if service unavailable

## 🔧 **Technical Requirements**

### **Data Structure We Need**
```cpp
struct TradeData {
    int login;           // Client login ID
    char symbol[12];     // Trading symbol
    int cmd;             // Trade command (0=buy, 1=sell)
    double volume;       // Trade volume
    double open_price;   // Requested price
    double sl;           // Stop loss
    double tp;           // Take profit
    double balance;      // Account balance
    double equity;       // Account equity
    char group[16];      // Client group
    int leverage;        // Account leverage
};
```

### **Routing Functions We Need**
```cpp
// Route to A-book (liquidity providers) - when score < threshold
bool RouteToABook(int ticket, const char* symbol, double volume);

// Route to B-book (internal handling) - when score >= threshold
bool RouteToBBook(int ticket, const char* symbol, double volume);
```

### **Binary Protocol Requirements**
- **TCP connection** with length-prefixed binary packets
- **Message format**: `[4-byte length][protobuf_body]`
- **Protobuf serialization** for compact transmission
- **Request**: 51-field ScoringRequest message
- **Response**: ScoringResponse with float32 score + warnings

## 🎮 **UX Requirements**

**Show them the dashboard requirements:**
- **Risk threshold configuration per asset class**
- **Revenue reporting (B-book baseline vs TDNA recommendations)**
- **Three override buttons: All A-Book, All B-Book, TDNA Scores**

## 📋 **Expected Deliverables**

**By end of meeting, we need:**
- ✅ API documentation for trade hooks
- ✅ Data structure specifications
- ✅ Routing API documentation
- ✅ Test environment access
- ✅ Technical contact information
- ✅ Implementation timeline

## 🚨 **Critical Success Factors**

1. **"Can we intercept trades before execution with sufficient data?"**
2. **"What's the exact process for A-book vs B-book routing?"**
3. **"Can you provide a test environment for integration?"**
4. **"What's your timeline for implementation and testing?"**

---

**References**: 
- [PRODUCT_REQUIREMENTS_DOCUMENT.md](PRODUCT_REQUIREMENTS_DOCUMENT.md) - Business requirements
- [TECHNICAL_SPECIFICATION.md](TECHNICAL_SPECIFICATION.md) - Complete technical specification with all 51 fields 