# MT4/MT5 Plug-in for Real-Time Scoring-Based A/B-book Routing - Technical Specification

## 🎯 **Objective**

Develop an MT4/MT5 server-side plug-in that routes trades to A-book or B-book based on real-time scores from an external service. The plug-in must:
- Remain lightweight and logic-free regarding the scoring model itself
- Offload scoring to an external Protobuf-compatible service
- Include locally stored threshold logic to make routing decisions
- Be secure, maintainable, and broker-configurable

## 🔧 **Trade Processing Flow**

### **1. Trade Detection & Vector Construction**
- Detect new trade openings (buy/sell orders)
- Construct a binary-encoded Protobuf-like feature vector from both numeric and string fields available on the MT4/MT5 platform
- Vector includes trade and account metadata
- The field list is fixed, but should be flexible for future expansion

### **2. Send Scoring Request to External Service**
- Open a raw TCP connection to the scoring microservice using a length-prefixed binary packet
- Use Protobuf-style serialisation for compact transmission
- Message format: `[length][protobuf_body]` where body contains multiple fields

### **3. Handle Scoring Response or Fallback**
- Parse the returned Protobuf message to extract a score (float32)
- If the service times out, fails to connect, or responds with invalid data:
  - Use a predefined fallback score (e.g., -1.0) for that trade
  - Log the fallback with error info

### **4. Threshold-Based Decision**
- Each trade belongs to a configurable instrument group (e.g., "FXMajors", "Crypto", etc.)
- The plug-in compares the score (or fallback) to a local threshold for that group
- **Routing logic:**
  - **If score >= threshold**: route to **B-book**
  - **If score < threshold**: route to **A-book**

### **5. Global Routing Overrides**
The broker should be able to toggle:
- Force A-book for all trades
- Force B-book for all trades
These are input parameters or config flags.

### **6. Logging**
For each trade scored and routed, log:
- Trade ticket, timestamp
- Feature vector content (optional, or hashed)
- Score or fallback used
- Threshold used and routing decision
- Errors or timeouts from the scoring service
Logs must be human-readable and optionally exportable.

## 📊 **Configurability**

### **Thresholds**
- Stored per instrument group (e.g., "FXMajors": 0.08, "Crypto": 0.12)
- Can be configured by the broker via a local .ini or .csv file or an input string MQL variable

### **Fallback Score**
- Configurable via input parameter

### **External Scoring Service**
- Target IP and port are input variables

## 🔌 **Message Format**

### **ScoringRequest (51 Fields)**
```protobuf
syntax = "proto3";
package scoring;

message ScoringRequest {
  float open_price                   = 1;  // Execution price at which the trade will open
  float sl                           = 2;  // Stop-loss price level the trader sets
  float tp                           = 3;  // Take-profit price level the trader sets
  uint32 deal_type                   = 4;  // Trade direction: 0 = buy (long), 1 = sell (short)
  float lot_volume                   = 5;  // Position size in standard lots
  int32  is_bonus                    = 6;  // Flag: 1 if bonus credit will be used, 0 if real funds
  float turnover_usd                 = 7;  // Total notional (USD) = open_price * lot_volume * contract_size
  float opening_balance              = 8;  // Account equity immediately before opening this trade
  int32  concurrent_positions        = 9;  // Number of other positions open at placement
  float sl_perc                      = 10; // Stop-loss distance fraction = |open_price - sl| / open_price
  float tp_perc                      = 11; // Take-profit distance fraction = |tp - open_price| / open_price
  int32  has_sl                      = 12; // Indicator: 1 if a stop-loss is set, else 0
  int32  has_tp                      = 13; // Indicator: 1 if a take-profit is set, else 0
  float profitable_ratio             = 14; // Proportion of past closed trades that were profitable
  float num_open_trades              = 15; // Number of trades currently open
  float num_closed_trades            = 16; // Total number of historical trades closed
  float age                          = 17; // Trader's age in years
  float days_since_reg               = 18; // Days since registration
  float deposit_lifetime             = 19; // Total deposited over account lifetime
  float deposit_count                = 20; // Number of deposit transactions
  float withdraw_lifetime            = 21; // Total withdrawn over account lifetime
  float withdraw_count               = 22; // Number of withdrawal transactions
  float vip                          = 23; // VIP status: 1 = VIP, 0 = regular
  float holding_time_sec             = 24; // Average duration of trades in the last 1–2h
  float lot_usd_value                = 25; // USD value of one lot
  float exposure_to_balance_ratio    = 26; // ratio = turnover_usd / opening_balance
  uint32 rapid_entry_exit            = 27; // Count of trades <60s in the last 1–2h
  uint32 abuse_risk_score            = 28; // Sum of has_sl and has_tp and not_empy rapid_entry_exit
  float trader_tenure_days           = 29; // Days since account opened
  float deposit_to_withdraw_ratio    = 30; // deposit_lifetime / max(1, withdraw_lifetime)
  int64  education_known             = 31; // 1 if education provided, else 0
  int64  occupation_known            = 32; // 1 if occupation provided, else 0
  float lot_to_balance_ratio         = 33; // (lot_volume * contract_size) / opening_balance
  float deposit_density              = 34; // deposit_count / max(1, days_since_reg)
  float withdrawal_density           = 35; // withdraw_count / max(1, days_since_reg)
  float turnover_per_trade           = 36; // turnover_usd / max(1, num_closed_trades)
  string symbol                      = 37; // Trading symbol (e.g., "EURUSD")
  string inst_group                  = 38; // Instrument group (e.g., "FXMajors")
  string frequency                   = 39; // Trading frequency (e.g., "medium")
  string trading_group               = 40; // Product grouping/path code
  string licence                     = 41; // Licensing jurisdiction (e.g., "CY")
  string platform                    = 42; // Trading platform (e.g., "MT5")
  string LEVEL_OF_EDUCATION          = 43; // Education level (e.g., "bachelor")
  string OCCUPATION                  = 44; // Occupation (e.g., "engineer")
  string SOURCE_OF_WEALTH            = 45; // Source of wealth (e.g., "salary")
  string ANNUAL_DISPOSABLE_INCOME    = 46; // Income bracket (e.g., "50k-100k")
  string AVERAGE_FREQUENCY_OF_TRADES = 47; // Trade frequency (e.g., "weekly")
  string EMPLOYMENT_STATUS           = 48; // Employment status (e.g., "employed")
  string country_code                = 49; // Country code (e.g., "CY")
  string utm_medium                  = 50; // UTM medium (e.g., "cpc")
  string user_id                     = 51; // Client-provided ID for external service queries
}
```

### **ScoringResponse**
```protobuf
message ScoringResponse {
  float score       = 1; // Model's computed score
  repeated string warnings = 2; // Any warnings produced during scoring
}
```

### **RPC Service Definition**
```protobuf
service ScoringService {
  rpc Score (ScoringRequest) returns (ScoringResponse);
}
```

## 📋 **Influx Logging (Optional)**

Plug-in supports pushing execution results to a Telegraf HTTP listener using Influx line protocol.
URL and enable switch are configurable.

## 🚀 **Testing & Deliverables**

### **Expected Deliverables:**
- MT4/MT5 DLL + full commented source code
- Configurable threshold system
- TCP socket binary Protobuf logic (encoder/decoder)
- Logging system
- Installation manual

### **Optional:**
- MT5 version
- GUI panel for threshold editing

## 🔧 **Technical Requirements**

### **Data Sources Required from Broker**
The plugin needs access to these **51 fields** from the broker's MT4/MT5 system:

#### **Trade Data (Fields 1-5)**
- `open_price` - Execution price
- `sl` - Stop-loss price level
- `tp` - Take-profit price level
- `deal_type` - Trade direction (0=buy, 1=sell)
- `lot_volume` - Position size in lots

#### **Account Data (Fields 6-36)**
- `is_bonus` - Bonus credit flag
- `turnover_usd` - Total notional value
- `opening_balance` - Account equity before trade
- `concurrent_positions` - Number of open positions
- `profitable_ratio` - Historical win rate
- `num_open_trades` - Current open trades
- `num_closed_trades` - Total historical trades
- `age` - Trader age
- `days_since_reg` - Days since registration
- `deposit_lifetime` - Total deposits
- `deposit_count` - Number of deposits
- `withdraw_lifetime` - Total withdrawals
- `withdraw_count` - Number of withdrawals
- `vip` - VIP status
- `holding_time_sec` - Average trade duration
- And additional calculated fields...

#### **Profile Data (Fields 37-51)**
- `symbol` - Trading symbol
- `inst_group` - Instrument group
- `frequency` - Trading frequency
- `trading_group` - Product grouping
- `licence` - Licensing jurisdiction
- `platform` - Trading platform
- `LEVEL_OF_EDUCATION` - Education level
- `OCCUPATION` - Occupation
- `SOURCE_OF_WEALTH` - Source of wealth
- `ANNUAL_DISPOSABLE_INCOME` - Income bracket
- `AVERAGE_FREQUENCY_OF_TRADES` - Trade frequency
- `EMPLOYMENT_STATUS` - Employment status
- `country_code` - Country code
- `utm_medium` - UTM medium
- `user_id` - Client ID

## 🛠️ **Implementation Architecture**

### **Binary Protocol**
- TCP connection with length-prefixed binary packets
- Format: `[4-byte length][protobuf_body]`
- Protobuf serialization for compact transmission

### **Routing Logic**
```
score >= threshold → B-book
score < threshold → A-book
service failure → fallback score → routing decision
```

### **Configuration System**
- .ini or .csv file for thresholds
- Input parameters for service IP/port
- Global override flags

### **Logging System**
- Human-readable logs
- Trade ticket, timestamp, score, threshold, decision
- Error logging for service failures
- Optional InfluxDB integration

---

**Document Version**: 1.0  
**Status**: Complete Technical Specification  
**Next Steps**: Broker integration meeting to confirm data availability 