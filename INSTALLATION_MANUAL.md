# MT4/MT5 A/B-Book Router Installation Manual

## Overview

This plugin provides real-time scoring-based A/B-book routing for MT4 and MT5 platforms. It communicates with an external scoring service to make routing decisions based on trade and account characteristics.

## System Requirements

### MT4/MT5 Platform
- MetaTrader 4 (build 1170+) or MetaTrader 5 (build 2650+)
- Windows Server 2016+ or Windows 10+
- .NET Framework 4.7.2+ (for DLL components)
- Minimum 2GB RAM, 1GB free disk space

### External Scoring Service
- Python 3.7+ (for test service)
- Network connectivity between MT4/MT5 and scoring service
- TCP port accessibility (default: 8080)

## Installation Steps

### 1. Copy Files to MT4/MT5 Data Folder

**For MT4:**
```
[MT4_DATA_FOLDER]/MQL4/Experts/MT4_ABBook_Router.mq4
[MT4_DATA_FOLDER]/MQL4/Files/ABBook_Config.ini
```

**For MT5:**
```
[MT5_DATA_FOLDER]/MQL5/Experts/MT5_ABBook_Router.mq5
[MT5_DATA_FOLDER]/MQL5/Files/ABBook_Config.ini
```

### 2. Compile Expert Advisors

1. Open MetaEditor (F4 in MT4/MT5)
2. Open the `.mq4` or `.mq5` file
3. Compile (F7) - ensure no errors
4. The compiled `.ex4` or `.ex5` file will be created automatically

### 3. Configure Thresholds

Edit `ABBook_Config.ini` to set appropriate thresholds for your trading requirements:

```ini
# Routing thresholds by instrument group
Threshold_FXMajors=0.08
Threshold_Crypto=0.12
Threshold_Metals=0.06
Threshold_Energy=0.10
Threshold_Indices=0.07
Threshold_Other=0.05
```

### 4. Set Up External Scoring Service

#### Option A: Test Service (for testing/demo)
```bash
python test_scoring_service.py
```

#### Option B: Production Service
Deploy your production scoring service that implements the protobuf interface defined in `scoring.proto`.

### 5. Configure Expert Advisor Parameters

When attaching the EA to a chart, configure these input parameters:

| Parameter | Description | Default |
|-----------|-------------|---------|
| ScoringServiceIP | IP address of scoring service | 127.0.0.1 |
| ScoringServicePort | Port of scoring service | 8080 |
| ConnectionTimeout | Timeout in milliseconds | 5000 |
| FallbackScore | Score to use if service unavailable | -1.0 |
| ConfigFile | Configuration file name | ABBook_Config.ini |
| ForceABook | Force all trades to A-book | false |
| ForceBBook | Force all trades to B-book | false |
| EnableInfluxLogging | Enable InfluxDB logging | false |
| EnableDetailedLogging | Enable detailed file logging | true |
| LogFilePrefix | Prefix for log files | ABBook_ |

## Configuration Details

### Threshold Configuration

The routing logic works as follows:
- **Score >= Threshold**: Route to B-book
- **Score < Threshold**: Route to A-book

Adjust thresholds based on your business requirements:
- **Lower threshold**: More trades go to B-book
- **Higher threshold**: More trades go to A-book

### Instrument Groups

The system automatically categorizes symbols into groups:
- **FXMajors**: EURUSD, GBPUSD, USDJPY, USDCHF, AUDUSD, USDCAD, NZDUSD
- **Crypto**: BTC*, ETH*, LTC*, XRP*
- **Metals**: GOLD*, XAU*, SILVER*, XAG*
- **Energy**: OIL*, WTI*, BRENT*
- **Indices**: SPX*, NDX*, DAX*, FTSE*
- **Other**: All other symbols

### Fallback Behavior

When the scoring service is unavailable:
1. The system uses the configured `FallbackScore`
2. If `FallbackScore` is negative, trades are routed to A-book by default
3. All fallback events are logged with error details

## Logging

### File Logging
- Location: `[MT4/MT5_DATA_FOLDER]/MQL4(5)/Files/ABBook_YYYY-MM-DD.log`
- Format: `TIMESTAMP - LEVEL: MESSAGE`
- Rotation: Daily

### Trade Decision Logging
Each routing decision is logged with:
- Trade ticket number
- Symbol and instrument group
- Score received and threshold used
- Final routing decision (A-book/B-book)
- Trade volume and price

### InfluxDB Integration (Optional)
Enable `EnableInfluxLogging` to send metrics to InfluxDB:
```
abbook_routing,symbol=EURUSD,group=FXMajors,decision=A_BOOK score=0.045,threshold=0.08,volume=1.0,price=1.1234 1645123456789012345
```

## Testing and Validation

### 1. Start Test Scoring Service
```bash
python test_scoring_service.py
```

### 2. Attach EA to Chart
1. Open MT4/MT5
2. Navigate to Expert Advisors
3. Drag EA to any chart
4. Configure parameters as needed
5. Enable "Allow DLL imports" if using DLL version

### 3. Monitor Logs
- Check EA logs in MetaTrader Experts tab
- Monitor log files for detailed information
- Verify scoring service receives requests

### 4. Test Scenarios
- Test with scoring service running
- Test with scoring service stopped (fallback behavior)
- Test different instrument types
- Test various trade sizes and account balances

## Production Deployment

### Broker Integration
The current implementation includes placeholders for broker-specific routing logic in functions:
- `RouteToABook(ticket, reason)`
- `RouteToBBook(ticket, reason)`

Replace these with calls to your broker's specific API for actual trade routing.

### Security Considerations
1. **Network Security**: Use VPN or secure network for service communication
2. **Authentication**: Implement authentication in scoring service if needed
3. **Encryption**: Consider TLS encryption for sensitive data
4. **Access Control**: Restrict access to configuration files
5. **Monitoring**: Implement alerting for service failures

### Performance Optimization
1. **Connection Pooling**: Consider implementing connection pooling for high-frequency trading
2. **Caching**: Cache scores for duplicate requests within short time windows
3. **Async Processing**: For high-volume brokers, consider asynchronous processing
4. **Load Balancing**: Deploy multiple scoring service instances

## Troubleshooting

### Common Issues

#### EA Not Loading
- Check compilation errors in MetaEditor
- Verify file permissions
- Ensure "Allow DLL imports" is enabled if needed

#### Connection Failures
- Verify scoring service is running
- Check firewall settings
- Confirm IP address and port configuration
- Test network connectivity: `telnet [IP] [PORT]`

#### Incorrect Routing
- Verify threshold configuration
- Check instrument group classification
- Review scoring service logic
- Monitor logs for score values

#### Log Issues
- Check file permissions in MQL4/5 Files directory
- Verify disk space availability
- Confirm log file path configuration

### Debug Mode
Enable detailed logging and monitor:
1. Expert Advisor logs in MT4/MT5
2. Log files in MQL4/5/Files directory
3. Scoring service console output
4. Network traffic if needed

## Support and Maintenance

### Regular Maintenance
1. **Log Rotation**: Clean up old log files periodically
2. **Configuration Review**: Review and adjust thresholds based on performance
3. **Service Monitoring**: Monitor scoring service health and response times
4. **Version Updates**: Keep EA and scoring service updated

### Monitoring Checklist
- [ ] Scoring service availability
- [ ] Network connectivity
- [ ] Log file growth
- [ ] Routing decision accuracy
- [ ] Performance metrics
- [ ] Error rates

## API Reference

### Protobuf Message Format
See `scoring.proto` for complete message definitions with all 51 fields required by the scoring model.

### Configuration File Format
```ini
# Comments start with #
Threshold_[GROUP]=[VALUE]
```

### Log File Format
```
YYYY-MM-DD HH:MM:SS - LEVEL: MESSAGE
YYYY-MM-DD HH:MM:SS - TRADE_DECISION,Ticket=123,Symbol=EURUSD,Group=FXMajors,Score=0.045,Threshold=0.08,Decision=A_BOOK
```

---

For technical support or questions, please refer to the documentation or contact your system administrator. 