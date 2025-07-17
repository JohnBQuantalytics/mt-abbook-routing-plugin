# ABBook Plugin Testing Guide - ML Scoring Service Integration

## Overview

This guide provides comprehensive testing procedures for the ABBook Plugin with the new ML scoring service at **188.245.254.12:50051**. The plugin now includes enhanced MT4 server journal logging and comprehensive testing capabilities.

## Prerequisites

### System Requirements
- Windows 10 or Windows Server 2016+
- Visual Studio 2019+ with C++ development tools
- Network connectivity to ML scoring service (188.245.254.12:50051)

### Files Required
- `MT4_ABBook_Plugin.cpp` - Main plugin source code
- `ABBook_Config.ini` - Configuration file (updated with new ML service IP/port)
- `test_mt4_plugin_comprehensive.cpp` - Comprehensive test suite
- `build_comprehensive_test.bat` - Build script
- `plugin_exports.def` - Plugin export definitions

## Configuration Update

The plugin has been updated to use your new ML scoring service:

```ini
[CVM_Connection]
CVM_IP=188.245.254.12
CVM_Port=50051
ConnectionTimeout=5000
FallbackScore=-1.0
```

## Building the Plugin and Test Suite

### Step 1: Open Developer Command Prompt
1. Open Start Menu
2. Search for "Developer Command Prompt for VS 2019" (or your VS version)
3. Run as Administrator
4. Navigate to your plugin directory

### Step 2: Build Everything
```batch
build_comprehensive_test.bat
```

This will build:
- `ABBook_Plugin_32bit.dll` - The main plugin
- `test_mt4_plugin_comprehensive.exe` - Comprehensive test application

## Testing Procedures

### Test 1: Direct ML Service Connection Test

First, test direct connectivity to your ML scoring service:

```batch
test_mt4_plugin_comprehensive.exe
```

The test will:
1. **Connection Test**: Attempt to connect to 188.245.254.12:50051
2. **Protocol Test**: Send a test message using the expected protocol
3. **Response Test**: Verify response reception

**Expected Output:**
```
[2024-01-15 14:30:15] INFO: Testing direct connection to ML scoring service...
[2024-01-15 14:30:15] INFO: Attempting connection to 188.245.254.12:50051...
[2024-01-15 14:30:15] SUCCESS: Successfully connected to ML scoring service
[2024-01-15 14:30:15] SUCCESS: Test message sent successfully
[2024-01-15 14:30:15] SUCCESS: Received response: {"score": 0.045}
```

### Test 2: Plugin Loading and Initialization Test

The test suite will:
1. Load the plugin DLL
2. Initialize the plugin
3. Test configuration loading
4. Verify all plugin functions are accessible

**Expected Output:**
```
[2024-01-15 14:30:16] SUCCESS: Plugin DLL loaded successfully
[2024-01-15 14:30:16] SUCCESS: Plugin functions loaded successfully
[2024-01-15 14:30:16] SUCCESS: Plugin initialized successfully
[2024-01-15 14:30:16] SUCCESS: Configuration reloaded
```

### Test 3: Trade Processing Simulation

The test will simulate 5 different trade scenarios:

1. **EURUSD Buy** - FX Major (Login: 12345)
2. **BTCUSD Sell** - Crypto (Login: 12346)
3. **XAUUSD Buy** - Metals (Login: 12347)
4. **CRUDE Buy** - Energy (Login: 12348)
5. **SPX500 Sell** - Indices (Login: 12349)

**Expected Output for Each Trade:**
```
[2024-01-15 14:30:17] INFO: === Processing Trade 1 ===
[2024-01-15 14:30:17] INFO: Symbol: EURUSD
[2024-01-15 14:30:17] INFO: Login: 12345
[2024-01-15 14:30:17] INFO: Type: BUY
[2024-01-15 14:30:17] INFO: Volume: 1 lots
[2024-01-15 14:30:17] INFO: Price: 1.1234
[2024-01-15 14:30:17] INFO: Balance: 10000
[2024-01-15 14:30:17] SUCCESS: Trade processed successfully
```

## MT4 Server Journal Logging

The plugin now includes enhanced MT4 server journal logging. In a real MT4 server environment, you'll see these messages in the server journal:

### Connection Messages
```
[ABBook Plugin] Attempting connection to ML scoring service at 188.245.254.12:50051
[ABBook Plugin] Successfully connected to ML scoring service
[ABBook Plugin] ML scoring service returned score: 0.045
```

### Trading Decision Messages
```
[ABBook Plugin] TRADING DECISION: Login:12345 Symbol:EURUSD Score:0.045 Threshold:0.08 Decision:A-BOOK
[ABBook Plugin] TRADING DECISION: Login:12346 Symbol:BTCUSD Score:0.15 Threshold:0.12 Decision:B-BOOK
```

### Error Messages
```
[ABBook Plugin] Failed to connect to ML scoring service - using fallback score
[ABBook Plugin] Invalid response format from ML scoring service
```

## Log Files Generated

After running tests, check these log files:

### 1. ABBook_Plugin_Debug.log
Contains detailed plugin operation logs:
```
[2024-01-15 14:30:15.123] [INFO] === MtSrvStartup() called ===
[2024-01-15 14:30:15.124] [INFO] Initializing MT4 server logging...
[2024-01-15 14:30:15.125] [INFO] Initializing Winsock...
[2024-01-15 14:30:15.126] [INFO] Loading configuration from ABBook_Config.ini
[2024-01-15 14:30:15.127] [INFO] CVM_IP: 188.245.254.12
[2024-01-15 14:30:15.128] [INFO] CVM_Port: 50051
```

### 2. ABBook_Plugin.log
Contains trade routing decisions:
```
2024-01-15 14:30:17 - Login:12345 Symbol:EURUSD Score:0.045 Threshold:0.08 Decision:A-BOOK
2024-01-15 14:30:17 - Login:12346 Symbol:BTCUSD Score:0.15 Threshold:0.12 Decision:B-BOOK
```

### 3. ABBook_Test_YYYYMMDD_HHMMSS.log
Contains test execution results:
```
[2024-01-15 14:30:15] INFO: Starting comprehensive plugin test
[2024-01-15 14:30:15] SUCCESS: Successfully connected to ML scoring service
[2024-01-15 14:30:16] SUCCESS: Plugin DLL loaded successfully
```

## Testing Scenarios

### Scenario 1: ML Service Available
1. Ensure ML service is running at 188.245.254.12:50051
2. Run comprehensive test
3. Verify all trades get scored
4. Check routing decisions are based on actual scores

### Scenario 2: ML Service Unavailable
1. Ensure ML service is NOT running (or block connection)
2. Run comprehensive test
3. Verify fallback score (-1.0) is used
4. Check all trades are routed to A-book (fallback behavior)

### Scenario 3: Configuration Override Testing
1. Edit `ABBook_Config.ini` to set `ForceABook=true`
2. Run comprehensive test
3. Verify all trades go to A-book regardless of score

## Real MT4 Server Integration

### For Testing in Real MT4 Server Environment:

1. **Copy Files to MT4 Server:**
   ```
   MT4_Server_Directory/
   ├── ABBook_Plugin_32bit.dll
   ├── ABBook_Config.ini
   └── (other MT4 server files)
   ```

2. **Configure MT4 Server:**
   - Add plugin to server configuration
   - Ensure plugin is loaded during server startup
   - Monitor server journal for plugin messages

3. **Monitor Server Journal:**
   Look for these types of messages in the MT4 server journal:
   - Plugin initialization messages
   - ML service connection status
   - Trade routing decisions
   - Error messages if ML service is unavailable

## Troubleshooting

### Connection Issues
- **Error**: "Failed to connect to ML scoring service"
- **Solution**: Check network connectivity, firewall settings, and ML service status
- **Test**: `telnet 188.245.254.12 50051`

### Plugin Loading Issues
- **Error**: "Could not load ABBook_Plugin_32bit.dll"
- **Solution**: Ensure DLL is compiled for correct architecture (32-bit)
- **Test**: Use `dumpbin /headers ABBook_Plugin_32bit.dll` to verify architecture

### Configuration Issues
- **Error**: "Could not open ABBook_Config.ini"
- **Solution**: Ensure config file is in same directory as plugin or MT4 server
- **Test**: Check file permissions and path

## Performance Monitoring

### Key Metrics to Monitor:
1. **Connection Time**: Time to connect to ML service
2. **Scoring Time**: Time to get score from ML service
3. **Total Processing Time**: End-to-end trade processing time
4. **Error Rate**: Percentage of failed ML service calls
5. **Fallback Usage**: Frequency of fallback score usage

### Expected Performance:
- **Connection Time**: < 100ms
- **Scoring Time**: < 500ms
- **Total Processing Time**: < 1000ms
- **Error Rate**: < 1%

## Next Steps

1. **Run the comprehensive test** to verify everything works
2. **Check all log files** for proper operation
3. **Test with real MT4 server** if available
4. **Monitor performance** in production environment
5. **Set up alerts** for ML service connectivity issues

## Support

For issues or questions:
1. Check log files first
2. Verify ML service connectivity
3. Review configuration settings
4. Test with fallback mode

The comprehensive test suite provides detailed diagnostics to help identify and resolve any issues with the plugin integration. 