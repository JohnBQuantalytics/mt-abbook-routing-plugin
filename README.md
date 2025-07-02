# MT4/MT5 A/B-Book Router

A comprehensive server-side plugin system for MetaTrader 4 and MetaTrader 5 that routes trades to A-book or B-book based on real-time scores from an external machine learning service.

## Features

- **Real-time Scoring**: Integrates with external ML scoring service via TCP/Protobuf
- **Configurable Thresholds**: Per-instrument group routing thresholds
- **Fallback Mechanism**: Automatic fallback when scoring service is unavailable
- **Comprehensive Logging**: Detailed trade routing decisions and performance metrics
- **InfluxDB Integration**: Optional metrics export for monitoring and analytics
- **File-based Configuration**: Easy-to-update INI file for threshold management
- **Broker Agnostic**: Designed to work with any MT4/MT5 broker platform

## File Documentation

### Core Implementation Files

#### **`MT4_Server_Plugin.cpp`** - Primary Server Plugin Implementation
- **Function**: Production-ready server-side C++ plugin for MT4/MT5 Server SDK integration
- **Key Features**:
  - `OnTradeRequest()` hook for server-side trade interception
  - Complete 51-field feature vector extraction
  - Production-ready TCP client implementation
  - Thread-safe configuration management
  - Real-time A-book/B-book routing decisions
- **Interactions**:
  - Uses `scoring.proto` definition for protobuf messaging
  - Integrates with `BrokerIntegration_Example.cpp` for routing
  - Reads from `ABBook_Config.ini` for configuration
  - Built using `build_plugin.bat` with `plugin_exports.def`

#### **`MT4_ABBook_Plugin.cpp`** - Alternative Plugin Implementation
- **Function**: Simplified server-side plugin for demonstration and testing
- **Key Features**: 
  - Demonstrates core plugin structure and API
  - Extracts 12-field simplified feature vector for scoring
  - TCP socket communication with scoring service
  - JSON messaging with fallback mechanism
  - A-book/B-book routing decisions based on thresholds
- **Interactions**: 
  - Reads from `ABBook_Config.ini` for thresholds
  - Can use `ABBook_ProtobufLib.cpp` for protobuf encoding
  - Integrates with `BrokerIntegration_Example.cpp` for routing
  - Sends metrics to `InfluxDB_HTTPClient.cpp` for monitoring

#### **`ABBook_ProtobufLib.cpp`** - Advanced Protobuf Communication
- **Function**: DLL providing native protobuf binary encoding/decoding
- **Key Features**:
  - Full 51-field protobuf message construction
  - Binary serialization for production efficiency
  - Error handling and validation
  - Memory management for large messages
- **Interactions**:
  - Implements `scoring.proto` message definitions
  - Called by both MT4 and server plugins for protobuf operations
  - Alternative to JSON messaging in production environments

### Configuration & Protocol Files

#### **`ABBook_Config.ini`** - Central Configuration Management
- **Function**: Stores all configurable parameters and thresholds
- **Contents**:
  - Scoring service connection details (IP, port, timeout)
  - Per-instrument group thresholds (FXMajors, Crypto, etc.)
  - Fallback behavior settings
  - Logging and monitoring options
- **Interactions**:
  - Read by all plugin implementations on startup
  - Modified by configuration panel
  - Monitored for real-time updates without restart

#### **`scoring.proto`** - Protobuf Message Definitions
- **Function**: Defines the exact structure of 51-field feature vector
- **Key Messages**:
  - `ScoringRequest`: Complete trade and account data
  - `ScoringResponse`: ML score and metadata
- **Interactions**:
  - Used by `ABBook_ProtobufLib.cpp` for code generation
  - Reference for all plugin implementations
  - Must match scoring service expectations

#### **`plugin_exports.def`** - DLL Export Definitions
- **Function**: Defines which functions are exported from compiled DLLs
- **Purpose**: Ensures proper linking between MT4/MT5 Server and C++ plugin components
- **Interactions**: Used by `build_plugin.bat` during compilation

### Integration Templates

#### **`BrokerIntegration_Example.cpp`** - Broker API Integration
- **Function**: Template showing how to connect to broker's routing systems
- **Key Functions**:
  - `RouteToABook()`: Sends trades to liquidity providers
  - `RouteToBBook()`: Keeps trades in-house
  - `GetAccountRiskProfile()`: Retrieves client risk data
  - `MonitorPositionPnL()`: Tracks trade performance
- **Interactions**:
  - Called by main plugins after routing decisions
  - Integrates with broker's specific APIs (needs customization)
  - Provides risk management and P&L monitoring

#### **`InfluxDB_HTTPClient.cpp`** - Metrics Export System
- **Function**: HTTP client for sending real-time metrics to InfluxDB
- **Key Features**:
  - Native HTTP/HTTPS connectivity
  - InfluxDB line protocol formatting
  - Batch sending for performance
  - Connection pooling and retry logic
- **Interactions**:
  - Receives metric data from main plugins
  - Sends to InfluxDB for monitoring dashboards
  - Independent component - optional for core functionality

### Testing & Development Files

#### **`test_scoring_service.py`** - Mock Scoring Service
- **Function**: Python service simulating ML scoring for development/testing
- **Features**:
  - TCP server listening on configurable port
  - Accepts JSON requests (protobuf simulation)
  - Returns mock scores based on trade characteristics
  - Multi-threaded for concurrent connections
- **Interactions**:
  - Receives requests from all plugin implementations
  - Used for end-to-end testing without real ML service
  - Reference implementation for actual scoring service

#### **`simple_connection_test.cpp`** - Connection Validator
- **Function**: Standalone C++ program to test TCP connectivity
- **Purpose**: Validates network connectivity and message format
- **Interactions**: Tests connection to scoring service independently

#### **`test_plugin.cpp`** - Plugin Functionality Tester  
- **Function**: Unit test program for plugin functions
- **Purpose**: Validates routing logic and configuration handling
- **Interactions**: Loads and tests compiled plugin DLLs

### Build & Deployment Files

#### **`build_plugin.bat`** - Automated Build Script
- **Function**: Compiles all C++ components into DLLs
- **Process**:
  1. Compiles `MT4_Server_Plugin.cpp` with exports
  2. Links required libraries (WinSock, etc.)
  3. Creates `ABBook_Plugin.dll` for production use
- **Interactions**: Uses `plugin_exports.def` for proper exports

#### **`test_connection.bat`** - Quick Connectivity Test
- **Function**: Batch script for rapid connection testing
- **Purpose**: Validates scoring service availability
- **Interactions**: Can launch `simple_connection_test.cpp` or Python scripts

### Documentation Files

#### **`INSTALLATION_MANUAL.md`** - Complete Setup Guide
- **Function**: Step-by-step installation instructions
- **Coverage**: System requirements, file placement, configuration, testing
- **Target**: Technical teams implementing the system

#### **`DEPLOYMENT_CHECKLIST.md`** - Production Deployment Guide
- **Function**: 200+ point checklist for production deployment
- **Coverage**: Pre-deployment, deployment, post-deployment verification
- **Target**: DevOps and system administrators

#### **`CPP_PLUGIN_README.md`** - C++ Plugin Documentation
- **Function**: Technical documentation for C++ components
- **Coverage**: API reference, integration points, performance considerations
- **Target**: Developers working with server-side plugins

## Component Interaction Flow

```
1. Trade Request → MT4/MT5 Server receives client trade
2. Server calls → Plugin OnTradeRequest() hook
3. Plugin extracts → Complete 51-field feature vector
4. Plugin encodes → Using ABBook_ProtobufLib.cpp for protobuf
5. Plugin sends → TCP request to scoring service
6. Service returns → ML score via scoring.proto format
7. Plugin compares → Score vs threshold from config
8. Plugin routes → Via BrokerIntegration_Example.cpp
9. Plugin logs → Decision and sends metrics to InfluxDB
10. Server executes → A-book or B-book routing decision
```

## File Dependencies

```
MT4_Server_Plugin.cpp (Primary Implementation)
├── ABBook_Config.ini (configuration)
├── ABBook_ProtobufLib.cpp (protobuf encoding)
├── BrokerIntegration_Example.cpp (routing)
├── InfluxDB_HTTPClient.cpp (metrics)
├── scoring.proto (message format)
├── plugin_exports.def (DLL exports)
└── build_plugin.bat (compilation)

MT4_ABBook_Plugin.cpp (Alternative Implementation)
├── ABBook_Config.ini (configuration)
├── scoring.proto (message format)
└── BrokerIntegration_Example.cpp (routing)

Test Components
├── test_scoring_service.py (mock service)
├── simple_connection_test.cpp (connectivity)
├── test_plugin.cpp (unit tests)
└── test_connection.bat (automation)
```

## System Architecture

```
┌─────────────────┐    TCP/Protobuf    ┌──────────────────┐
│ MT4/MT5 Server   │◄──────────────────►│ Scoring Service  │
│    C++ Plugin    │                    │                  │
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

## Quick Start

### 1. Install Test Scoring Service

```bash
python test_scoring_service.py
```

### 2. Build and Install Server Plugin

**Build Plugin:**
1. Run `build_plugin.bat` to compile the C++ plugin
2. This creates `ABBook_Plugin.dll` from the source files
3. Ensure all dependencies are linked properly

**Install Plugin:**
1. Copy `ABBook_Plugin.dll` to MT4/MT5 server plugins directory
2. Copy `ABBook_Config.ini` to server configuration directory
3. Register plugin in MT4/MT5 server configuration file

### 3. Configure and Deploy

1. Update `ABBook_Config.ini` with your scoring service details
2. Configure thresholds per instrument group
3. Restart MT4/MT5 server to load the plugin
4. Monitor server logs for routing decisions

## Configuration

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

## Scoring Model Features

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

## Monitoring and Logging

### File Logging
- Location: `Server/Logs/ABBook_YYYY-MM-DD.log`
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

## Production Components

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

## Testing

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

## Requirements

### System Requirements
- MetaTrader 4 (build 1170+) or MetaTrader 5 (build 2650+)
- Windows Server 2016+ or Windows 10+
- .NET Framework 4.7.2+ (for DLL components)
- Minimum 2GB RAM, 1GB free disk space

### Network Requirements
- TCP connectivity to scoring service
- Configurable ports (default: 8080)
- Low latency network for optimal performance

## Development

### Adding New Features
1. **New Instrument Groups**: Update `GetInstrumentGroup()` function
2. **Additional Fields**: Modify protobuf definitions and encoding logic
3. **Custom Routing Logic**: Extend decision-making algorithms
4. **Integration APIs**: Add broker-specific integration points

### Code Structure
- **Main Plugin**: Core routing logic and service communication
- **Configuration**: File-based threshold management
- **Logging**: Comprehensive audit trail
- **Protobuf**: Message serialization/deserialization
- **GUI**: User-friendly configuration interface

## Documentation

- **[Installation Manual](INSTALLATION_MANUAL.md)**: Complete setup guide
- **[Protobuf Definitions](scoring.proto)**: Message format specifications
- **Code Comments**: Inline documentation for all functions
- **Configuration Examples**: Sample configuration files

## Support

For technical issues or questions:
1. Check the installation manual
2. Review log files for error messages
3. Verify network connectivity
4. Test with the provided mock service
5. Contact system administrator

## License

Copyright 2024, Trading System. All rights reserved.

---

**Important Notice**: This is a demonstration implementation. For production use, ensure proper integration with your broker's specific routing APIs and implement appropriate security measures. 