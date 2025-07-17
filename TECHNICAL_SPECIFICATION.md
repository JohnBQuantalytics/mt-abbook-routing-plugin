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
  // Core Trade Data (1-5)
  float open_price                   = 1;  // Execution price at which the trade will open
  float sl                           = 2;  // Stop-loss price level the trader sets
  float tp                           = 3;  // Take-profit price level the trader sets
  int64  deal_type                   = 4;  // Trade direction: 0 = buy (long), 1 = sell (short)
  float lot_volume                   = 5;  // Position size in standard lots

  // Account & Trading History (6-36)
  int64  is_bonus                    = 6;  // Flag: 1 if bonus credit will be used, 0 if real funds
  float turnover_usd                 = 7;  // Total notional (USD) = open_price * lot_volume * contract_size
  float opening_balance              = 8;  // Account equity immediately before opening this trade
  int64  concurrent_positions        = 9;  // Number of other positions open at placement
  float sl_perc                      = 10; // Stop-loss distance fraction = |open_price - sl| / open_price
  float tp_perc                      = 11; // Take-profit distance fraction = |tp - open_price| / open_price
  int64  has_sl                      = 12; // Indicator: 1 if a stop-loss is set, else 0
  int64  has_tp                      = 13; // Indicator: 1 if a take-profit is set, else 0
  float profitable_ratio             = 14; // Proportion of past closed trades that were profitable
  int64  num_open_trades             = 15; // Number of trades currently open
  int64  num_closed_trades           = 16; // Total number of historical trades closed
  int64  age                         = 17; // Trader's age in years
  int64  days_since_reg              = 18; // Days since registration
  float deposit_lifetime             = 19; // Total deposited over account lifetime
  int64  deposit_count               = 20; // Number of deposit transactions
  float withdraw_lifetime            = 21; // Total withdrawn over account lifetime
  int64  withdraw_count              = 22; // Number of withdrawal transactions
  int64  vip                         = 23; // VIP status: 1 = VIP, 0 = regular
  int64  holding_time_sec            = 24; // Average duration of trades in the last 1–2h
  float lot_usd_value                = 25; // USD value of one lot
  float max_drawdown                 = 26; // Maximum drawdown (negative value)
  float max_runup                    = 27; // Maximum runup (positive value)
  float volume_24h                   = 28; // Total trading volume (lots) in last 24h
  float trader_tenure_days           = 29; // Days since account opened
  float deposit_to_withdraw_ratio    = 30; // deposit_lifetime / max(1, withdraw_lifetime)
  int64  education_known             = 31; // 1 if education provided, else 0
  int64  occupation_known            = 32; // 1 if occupation provided, else 0
  float lot_to_balance_ratio         = 33; // (lot_volume * contract_size) / opening_balance
  float deposit_density              = 34; // deposit_count / max(1, days_since_reg)
  float withdrawal_density           = 35; // withdraw_count / max(1, days_since_reg)
  float turnover_per_trade           = 36; // turnover_usd / max(1, num_closed_trades)

  // Recent Performance Metrics (37-45) - NEW GRANULAR PROFITABILITY DATA
  float profitable_ratio_24h         = 37; // Profitability rate for last 24 hours
  float profitable_ratio_48h         = 38; // Profitability rate for last 48 hours
  float profitable_ratio_72h         = 39; // Profitability rate for last 72 hours
  int64  trades_count_24h            = 40; // Number of trades closed in last 24 hours
  int64  trades_count_48h            = 41; // Number of trades closed in last 48 hours
  int64  trades_count_72h            = 42; // Number of trades closed in last 72 hours
  float avg_profit_24h               = 43; // Average profit per trade in last 24 hours (USD)
  float avg_profit_48h               = 44; // Average profit per trade in last 48 hours (USD)
  float avg_profit_72h               = 45; // Average profit per trade in last 72 hours (USD)

  // Context & Metadata (46-60)
  string symbol                      = 46; // Trading symbol (e.g., "EURUSD")
  string inst_group                  = 47; // Instrument group (e.g., "FXMajors")
  string frequency                   = 48; // Trading frequency (e.g., "medium")
  string trading_group               = 49; // Product grouping/path code
  string licence                     = 50; // Licensing jurisdiction (e.g., "CY")
  string platform                    = 51; // Trading platform (e.g., "MT5")
  string LEVEL_OF_EDUCATION          = 52; // Education level (e.g., "bachelor")
  string OCCUPATION                  = 53; // Occupation (e.g., "engineer")
  string SOURCE_OF_WEALTH            = 54; // Source of wealth (e.g., "salary")
  string ANNUAL_DISPOSABLE_INCOME    = 55; // Income bracket (e.g., "50k-100k")
  string AVERAGE_FREQUENCY_OF_TRADES = 56; // Trade frequency (e.g., "weekly")
  string EMPLOYMENT_STATUS           = 57; // Employment status (e.g., "employed")
  string country_code                = 58; // Country code (e.g., "CY")
  string utm_medium                  = 59; // UTM medium (e.g., "cpc")
  string user_id                     = 60; // Client-provided ID for external service queries
}
```