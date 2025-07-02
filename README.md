# MT4/MT5 A/B-Book Router

A comprehensive server-side plugin system for MetaTrader 4 and MetaTrader 5 that routes trades to A-book or B-book based on real-time scores from an external machine learning service.

## 🎯 Features

- **Real-time Scoring**: Integrates with external ML scoring service via TCP/Protobuf
- **Configurable Thresholds**: Per-instrument group routing thresholds
- **Fallback Mechanism**: Automatic fallback when scoring service is unavailable
- **Comprehensive Logging**: Detailed trade routing decisions and performance metrics
- **InfluxDB Integration**: Optional metrics export for monitoring and analytics
- **GUI Configuration**: Easy-to-use configuration panel for threshold management
- **Broker Agnostic**: Designed to work with any MT4/MT5 broker platform

## 📁 Project Structure

```
Mt4 plugin/
├── MT4_ABBook_Router.mq4          # Main MT4 Expert Advisor
├── MT5_ABBook_Router.mq5          # Main MT5 Expert Advisor  
├── ABBook_Config.ini              # Configuration file template
├── ABBook_ConfigPanel.mq4         # GUI configuration panel
├── scoring.proto                  # Protobuf message definitions
├── test_scoring_service.py        # Test scoring service
├── ABBook_ProtobufLib.cpp         # Advanced protobuf DLL (optional)
├── BrokerIntegration_Example.cpp  # Broker API integration template
├── InfluxDB_HTTPClient.cpp        # HTTP client for InfluxDB metrics
├── INSTALLATION_MANUAL.md         # Detailed installation guide
├── DEPLOYMENT_CHECKLIST.md        # Production deployment checklist
└── README.md                      # This file
```

## 🏗️ System Architecture

```
┌─────────────────┐    TCP/Protobuf    ┌──────────────────┐
│   MT4/MT5 EA    │◄──────────────────►│ Scoring Service  │
│                 │                    │                  │
│ • Real-time     │                    │ • ML Model       │
│   Trade Det.    │                    │ • 51-field       │
│ • Feature       │                    │   Features       │
│   Extraction    │                    │ • Score          │
│ • A/B Routing   │                    │   Generation     │
│                 │                    │                  │
└─────────────────┘                    └──────────────────┘
         │                                        │
         │ ┌─────────────────┐                   │
         ├─┤ Broker API DLL  │                   │
         │ │                 │                   │
         │ │ • A-book Route  │                   │
         │ │ • B-book Route  │                   │
         │ │ • Risk Limits   │                   │
         │ │ • P&L Monitor   │                   │
         │ └─────────────────┘                   │
         │                                        │
         │ ┌─────────────────┐                   │
         ├─┤ InfluxDB Client │◄──────────────────┘
         │ │                 │
         │ │ • HTTP Client   │
         │ │ • Line Protocol │
         │ │ • Batch Send    │
         │ │ • Monitoring    │
         │ └─────────────────┘
         │
         ▼
┌─────────────────┐                    ┌──────────────────┐
│ Broker Platform │                    │    InfluxDB      │
│                 │                    │                  │
│ • A-book        │                    │ • Real-time      │
│ • B-book        │                    │   Metrics        │
│ • Risk Mgmt     │                    │ • Historical     │
│                 │                    │   Analytics      │
└─────────────────┘                    └──────────────────┘
```

## 🚀 Quick Start

### 1. Install Test Scoring Service

```bash
python test_scoring_service.py
```

### 2. Install MT4/MT5 Plugin

**For MT4:**
1. Copy `MT4_ABBook_Router.mq4` to `[MT4_DATA]/MQL4/Experts/`
2. Copy `ABBook_Config.ini` to `[MT4_DATA]/MQL4/Files/`
3. Compile the EA in MetaEditor

**For MT5:**
1. Copy `MT5_ABBook_Router.mq5` to `[MT5_DATA]/MQL5/Experts/`
2. Copy `ABBook_Config.ini` to `[MT5_DATA]/MQL5/Files/`
3. Compile the EA in MetaEditor

### 3. Configure and Run

1. Attach the EA to any chart in MT4/MT5
2. Configure the scoring service IP and port
3. Adjust thresholds using the configuration panel
4. Monitor logs for routing decisions

## ⚙️ Configuration

### Default Thresholds

| Instrument Group | Default Threshold | Description |
|------------------|-------------------|-------------|
| FXMajors         | 0.08             | Major currency pairs |
| Crypto           | 0.12             | Cryptocurrency pairs |
| Metals           | 0.06             | Gold, Silver, etc. |
| Energy           | 0.10             | Oil, Gas, etc. |
| Indices          | 0.07             | Stock indices |
| Other            | 0.05             | All other instruments |

### Routing Logic

- **Score ≥ Threshold**: Route to B-book
- **Score < Threshold**: Route to A-book
- **Service Unavailable**: Use fallback score (default: route to A-book)

## 📊 Scoring Model Features

The system sends 51 features to the scoring service:

**Trade Features:**
- Open price, stop loss, take profit
- Position size, trade direction
- Risk management indicators

**Account Features:**
- Balance, equity, trading history
- Deposit/withdrawal patterns
- Account demographics

**Behavioral Features:**
- Trading frequency, holding times
- Risk-taking patterns
- Abuse risk indicators

See `scoring.proto` for complete field definitions.

## 📈 Monitoring and Logging

### File Logging
- Location: `MQL4/5/Files/ABBook_YYYY-MM-DD.log`
- Format: Timestamped trade decisions with full context
- Rotation: Daily

### InfluxDB Integration
```sql
abbook_routing,symbol=EURUSD,group=FXMajors,decision=A_BOOK 
  score=0.045,threshold=0.08,volume=1.0,price=1.1234 
  1645123456789012345
```

### Key Metrics
- Routing decision accuracy
- Service response times
- Fallback usage frequency
- Score distribution by instrument group

## 🔧 Production Components

### Broker Integration DLL
The `BrokerIntegration_Example.cpp` provides a template for connecting to your broker's routing APIs:

```cpp
// Example integration
__declspec(dllexport) bool __stdcall RouteTradeAdvanced(
    int ticket, const char* symbol, int type, double volume, 
    double price, const char* routing_decision, float score, 
    double threshold, const char* reason
);
```

**Key Features:**
- Real-time A-book/B-book routing
- Automatic risk management for B-book trades
- Bulk trade processing for high-frequency scenarios
- Position monitoring and P&L tracking

### InfluxDB HTTP Client
The `InfluxDB_HTTPClient.cpp` enables real-time metrics export:

```cpp
// Send trade metrics
SendTradeMetric("EURUSD", "FXMajors", "A_BOOK", 
                0.045, 0.08, 1.0, 1.1234, timestamp);
```

**Capabilities:**
- HTTP/HTTPS connectivity to InfluxDB
- Line protocol formatting
- Batch metric sending
- Connection pooling and error handling

### Production Deployment

Replace placeholder routing functions with broker-specific API calls:

```cpp
void RouteToABook(int ticket, string reason)
{
    // Use the broker integration DLL
    if (RouteTradeToABook(ticket)) {
        LogMessage("Successfully routed to A-book: " + reason);
    }
}
```

### Security
- Use VPN for service communication
- Implement authentication in scoring service
- Encrypt sensitive data transmission
- Monitor for suspicious activities

### Performance
- Connection pooling for high-frequency trading
- Score caching for duplicate requests
- Load balancing across multiple scoring instances
- Asynchronous processing for high volumes

## 🧪 Testing

### Test Scenarios
1. **Normal Operation**: Service available, various score ranges
2. **Fallback Mode**: Service unavailable, fallback scoring
3. **Configuration Changes**: Dynamic threshold updates
4. **High Load**: Multiple concurrent trades
5. **Network Issues**: Connection timeouts, retries

### Validation
- Compare routing decisions against expected outcomes
- Monitor score distributions
- Verify fallback behavior
- Check log accuracy and completeness

## 📋 Requirements

### System Requirements
- MetaTrader 4 (build 1170+) or MetaTrader 5 (build 2650+)
- Windows Server 2016+ or Windows 10+
- .NET Framework 4.7.2+ (for DLL components)
- Minimum 2GB RAM, 1GB free disk space

### Network Requirements
- TCP connectivity to scoring service
- Configurable ports (default: 8080)
- Low latency network for optimal performance

## 🛠️ Development

### Adding New Features
1. **New Instrument Groups**: Update `GetInstrumentGroup()` function
2. **Additional Fields**: Modify protobuf definitions and encoding logic
3. **Custom Routing Logic**: Extend decision-making algorithms
4. **Integration APIs**: Add broker-specific integration points

### Code Structure
- **Main EA**: Core routing logic and service communication
- **Configuration**: File-based threshold management
- **Logging**: Comprehensive audit trail
- **Protobuf**: Message serialization/deserialization
- **GUI**: User-friendly configuration interface

## 📚 Documentation

- **[Installation Manual](INSTALLATION_MANUAL.md)**: Complete setup guide
- **[Protobuf Definitions](scoring.proto)**: Message format specifications
- **Code Comments**: Inline documentation for all functions
- **Configuration Examples**: Sample configuration files

## 🤝 Support

For technical issues or questions:
1. Check the installation manual
2. Review log files for error messages
3. Verify network connectivity
4. Test with the provided mock service
5. Contact system administrator

## 📄 License

Copyright 2024, Trading System. All rights reserved.

---

**⚠️ Important Notice**: This is a demonstration implementation. For production use, ensure proper integration with your broker's specific routing APIs and implement appropriate security measures. 