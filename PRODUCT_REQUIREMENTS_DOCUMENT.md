# MT4/MT5 A/B-Book Routing Plugin - Product Requirements Document (PRD)

## 🎯 **Product Overview**

The MT4/MT5 A/B-book routing plugin is a server-side C++ plugin that automatically routes client trades to either A-book (hedged with liquidity providers) or B-book (kept in-house) based on real-time machine learning risk scores.

## 🔧 **Core Functionality**

### **Trade Processing Flow**
1. **Trade Interception**: Plugin intercepts client trades through MT4/MT5 server's `OnTradeRequest()` hook
2. **Feature Extraction**: Extracts comprehensive 51-field feature vector containing:
   - Trade details (symbol, volume, price, stop loss, take profit)
   - Account characteristics (balance, equity, leverage, group)
   - Behavioral patterns (trading history, deposit patterns, risk indicators)
3. **ML Scoring**: Encodes data using Protocol Buffers and sends via TCP to external ML scoring service
4. **Score Analysis**: ML service analyzes client risk profile and returns numerical score (0-1)
5. **Routing Decision**: Compares score against configurable per-instrument thresholds
6. **Trade Execution**: Routes to A-book or B-book based on decision

### **Scoring Logic**
- **Score > Threshold**: Route to **A-Book** (hedged with liquidity providers)
- **Score < Threshold**: Route to **B-Book** (kept in-house for higher profit potential)
- **Service Unavailable**: Fallback mechanism automatically routes to A-book

### **Performance Requirements**
- **Scoring Response Time**: ~5ms from colocated virtual machine (CVM)
- **High-Frequency Trading**: Supports connection pooling, asynchronous processing
- **Configurable Timeouts**: Optimized for production trading environments

## 📊 **Technical Architecture**

### **Data Flow**
```
MT4/MT5 Server → OnTradeRequest() Hook → Plugin → 51-Field Feature Vector → 
Protocol Buffers → TCP → External ML Scoring Service (CVM) → 
Score (0-1) → Threshold Comparison → A-Book/B-Book Routing Decision
```

### **Key Components**
1. **Server-Side Plugin**: C++ plugin integrated with MT4/MT5 server
2. **Feature Extraction**: 51-field comprehensive trade and account data
3. **ML Scoring Service**: External service on colocated virtual machine
4. **Configuration System**: `ABBook_Config.ini` for threshold management
5. **Logging System**: Full context logging and optional InfluxDB integration
6. **Broker Integration**: Template for connecting to specific routing APIs

## 🎮 **User Experience Requirements**

### **1. Risk Threshold Configuration**
- **Per Asset Class Thresholds**: Easily configurable by client
  - FX Majors: Default 0.08
  - Crypto: Default 0.12
  - Metals: Default 0.06
  - Energy: Default 0.10
  - Indices: Default 0.07
  - Other: Default 0.05

### **2. Reporting Dashboard**
- **B-Book Revenue**: Shows revenue if nothing was done (baseline)
- **TDNA Revenue**: Shows revenue based on AI recommendations
- **Comparison Metrics**: Performance difference between approaches
- **Real-time Analytics**: Live monitoring of routing decisions

### **3. Override Controls**
Three manual override buttons:
- **All A-Book**: Force all trades to A-book routing
- **All B-Book**: Force all trades to B-book routing  
- **TDNA Scores**: Use AI scoring recommendations (default mode)

## 🔄 **Position Management**

### **Opening Positions**
- **Full Scoring**: Every opening position gets scored by ML service
- **Feature Vector**: Complete 51-field analysis
- **Real-time Decision**: Routing decision within ~5ms
- **Audit Trail**: Full logging of decision rationale

### **Closing Positions**
- **No Scoring**: Closing positions are NOT scored
- **Position Matching**: Simply matches closing trade with original opening position
- **Same Routing**: Closes via same route as opening (A-book or B-book)
- **Automatic Processing**: No manual intervention required

## 📋 **Configuration Management**

### **ABBook_Config.ini Structure**
```ini
[CVM_Connection]
CVM_IP=YOUR_CVM_IP
CVM_Port=YOUR_CVM_PORT
ConnectionTimeout=5000

[Thresholds]
Threshold_FXMajors=0.08
Threshold_Crypto=0.12
Threshold_Metals=0.06
Threshold_Energy=0.10
Threshold_Indices=0.07
Threshold_Other=0.05

[Routing_Overrides]
ForceABook=false
ForceBBook=false
UseTDNAScores=true
```

## 🚀 **Integration Requirements**

### **Broker Integration**
- **API Connectivity**: Template for connecting to broker-specific routing APIs
- **Existing Infrastructure**: Seamless integration with current trading systems
- **A-Book Routing**: Connection to liquidity providers
- **B-Book Routing**: Internal trade management

### **ML Scoring Service**
- **Protocol**: TCP connection with Protocol Buffers encoding
- **Location**: Colocated virtual machine (CVM) for low latency
- **Response Time**: Target ~5ms for scoring response
- **Availability**: High availability with fallback mechanisms

## 📈 **Monitoring & Analytics**

### **Logging System**
- **Full Context Logging**: Every routing decision with complete rationale
- **Performance Metrics**: Response times, throughput, error rates
- **Audit Trail**: Complete compliance and regulatory reporting

### **InfluxDB Integration** (Optional)
- **Real-time Metrics**: Live performance monitoring
- **Analytics Dashboards**: Business intelligence and reporting
- **Historical Analysis**: Trend analysis and optimization insights

## 🧪 **Testing & Development**

### **Test Environment**
- **Python Test Service**: Simulates ML scoring service for development
- **Mock Data**: Realistic trade scenarios for validation
- **Performance Testing**: Load testing for high-frequency scenarios

### **Validation Components**
- **Connection Testing**: TCP connectivity validation
- **Plugin Testing**: Full workflow testing
- **Integration Testing**: End-to-end broker integration validation

## 🛡️ **Risk Management**

### **Fallback Mechanisms**
- **Service Unavailable**: Automatic A-book routing if ML service down
- **Timeout Protection**: Configurable timeouts prevent trade delays
- **Error Handling**: Graceful degradation with full logging

### **Compliance**
- **Audit Trail**: Complete logging for regulatory compliance
- **Risk Controls**: Configurable thresholds and override capabilities
- **Monitoring**: Real-time alerting for system issues

## 🎯 **Business Objectives**

### **Profit Optimization**
- **Intelligent Routing**: Data-driven decisions for maximum profitability
- **Risk Mitigation**: Automated risk assessment and management
- **Performance Tracking**: Clear ROI measurement and reporting

### **Operational Efficiency**
- **Automated Processing**: Eliminates manual trade routing decisions
- **Real-time Operation**: Immediate routing decisions without delays
- **Scalable Architecture**: Handles high-frequency trading environments

## 🔧 **Technical Specifications**

### **System Requirements**
- **Platform**: Windows Server 2016+ with MT4/MT5 Server
- **Network**: TCP connectivity to colocated virtual machine
- **Performance**: Sub-10ms end-to-end processing time
- **Reliability**: 99.9% uptime requirement

### **Integration Points**
- **MT4/MT5 Server**: OnTradeRequest() hook integration
- **ML Scoring Service**: TCP/Protocol Buffers communication
- **Broker APIs**: Custom routing API integration
- **Monitoring Systems**: InfluxDB and logging integration

## 📊 **Success Metrics**

### **Performance KPIs**
- **Latency**: < 10ms end-to-end processing
- **Throughput**: Support for 1000+ trades/second
- **Availability**: 99.9% uptime
- **Accuracy**: Consistent ML scoring and routing decisions

### **Business KPIs**
- **Revenue Optimization**: Measurable improvement in profitability
- **Risk Reduction**: Decreased exposure to high-risk trades
- **Operational Efficiency**: Reduced manual intervention
- **Compliance**: Full audit trail and regulatory compliance

---

**Document Version**: 1.0  
**Last Updated**: January 2024  
**Status**: Final PRD for Implementation 