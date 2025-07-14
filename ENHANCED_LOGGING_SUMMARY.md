# Enhanced Logging Plugin - Debug Version

## 📋 Overview

This is an enhanced version of the ABBook 32-bit plugin with comprehensive logging designed to help debug the "invalid plugin" error your client is experiencing. The plugin now includes extensive logging throughout every critical function.

## 🔧 What's Been Added

### 1. **Comprehensive Logging System**
- **New Logger Class**: `PluginLogger` with timestamp precision to milliseconds
- **Multiple Log Levels**: ERROR, WARN, INFO, DEBUG
- **Thread-Safe Logging**: Mutex protection for concurrent access
- **Dual Output**: Logs to both file and console (when available)
- **Windows Error Logging**: Automatic translation of Windows/WinSock error codes

### 2. **Enhanced Log Files**
- **`ABBook_Plugin_Debug.log`**: Main debug log with comprehensive details
- **`ABBook_Plugin.log`**: Original log file (maintained for compatibility)
- **`ABBook_Plugin_Emergency.log`**: Emergency log for critical DLL errors

### 3. **Detailed Logging Coverage**

#### DLL Lifecycle Logging
- **DLL_PROCESS_ATTACH**: Logs when plugin is loaded
- **DLL_PROCESS_DETACH**: Logs when plugin is unloaded
- **Process Detection**: Identifies if loaded by MT4/MT5 server
- **Module Information**: Logs DLL path and module handle

#### Plugin Initialization (`PluginInit()`)
- **Step-by-step initialization**: Each phase logged separately
- **Winsock initialization**: Version information and error handling
- **Configuration loading**: Detailed configuration parsing
- **System information**: Computer name, process ID, thread ID, working directory
- **Functionality testing**: Socket creation, file permissions
- **Module path logging**: Plugin location and dependencies

#### Configuration Loading (`LoadConfiguration()`)
- **File access**: Logs if config file exists or needs creation
- **Line-by-line parsing**: Each configuration parameter logged
- **Error handling**: Detailed error messages for invalid values
- **Final configuration summary**: Complete settings overview

#### Trade Processing (`OnTradeRequest()`)
- **Input validation**: Checks for null pointers
- **Trade details**: Complete trade information logging
- **Server context**: Logs MT4 server context pointer
- **Processing results**: Routing decision and reason codes
- **Exception handling**: Comprehensive error catching

#### Other Functions
- **OnTradeClose()**: Trade closure logging
- **OnConfigUpdate()**: Configuration reload logging
- **PluginCleanup()**: Cleanup process logging

## 📁 Files in Enhanced Package

1. **`ABBook_Plugin_32bit.dll`** - Enhanced plugin with comprehensive logging
2. **`ABBook_Config.ini`** - Configuration file
3. **`diagnose_plugin_issue.bat`** - Diagnostic script to identify issues
4. **`CLIENT_ACCESS_GUIDE.md`** - Guide for client communication and troubleshooting
5. **`ENHANCED_LOGGING_SUMMARY.md`** - This file

## 🚀 How to Use

### 1. **Basic Deployment**
```bash
# Copy the enhanced plugin to MT4 server plugins directory
copy ABBook_Plugin_32bit.dll "C:\Program Files\MT4 Server\plugins\"

# Ensure configuration file is in the same directory
copy ABBook_Config.ini "C:\Program Files\MT4 Server\plugins\"
```

### 2. **Enable Logging**
The plugin automatically starts logging when loaded. No additional configuration needed.

### 3. **Monitor Log Files**
After attempting to load the plugin, check these files:
- **`ABBook_Plugin_Debug.log`** - Main debug information
- **`ABBook_Plugin_Emergency.log`** - Critical errors during DLL loading
- **`ABBook_Plugin.log`** - Original format logs

### 4. **Run Diagnostics**
```bash
# Run the diagnostic script to identify system issues
diagnose_plugin_issue.bat
```

## 🔍 What the Logs Will Show

### If Plugin Loads Successfully
You'll see logs like:
```
[2025-01-XX 10:30:15.123] [INFO] === DLL_PROCESS_ATTACH ===
[2025-01-XX 10:30:15.124] [INFO] DLL is being loaded into process
[2025-01-XX 10:30:15.125] [INFO] Process name: C:\Program Files\MT4 Server\terminal.exe
[2025-01-XX 10:30:15.126] [INFO] Detected MT4/MT5 process
[2025-01-XX 10:30:15.127] [INFO] === PluginInit() called ===
[2025-01-XX 10:30:15.128] [INFO] Winsock initialized successfully
[2025-01-XX 10:30:15.129] [INFO] Configuration loaded successfully
[2025-01-XX 10:30:15.130] [INFO] Plugin initialization completed successfully
```

### If Plugin Fails to Load
You'll see detailed error information:
```
[2025-01-XX 10:30:15.123] [ERROR] WSAStartup failed with error: 10091
[2025-01-XX 10:30:15.124] [ERROR] Configuration loading failed
[2025-01-XX 10:30:15.125] [ERROR] Exception in PluginInit: Access denied
```

## 🛠️ Troubleshooting Steps

### Step 1: Check Log Files
1. Look for `ABBook_Plugin_Debug.log` - main debug info
2. Check `ABBook_Plugin_Emergency.log` - critical DLL errors
3. Review `plugin_diagnostic.log` - system diagnostic info

### Step 2: Common Issues and Solutions

#### Issue: "DLL not found"
**Solution**: Ensure plugin is in correct directory with proper permissions

#### Issue: "WSAStartup failed"
**Solution**: Install Visual C++ Redistributable (x86)

#### Issue: "Configuration loading failed"
**Solution**: Check if `ABBook_Config.ini` exists and is readable

#### Issue: "Access denied"
**Solution**: Run MT4 server as administrator

### Step 3: System Diagnostics
Run the diagnostic script to check:
- System requirements
- File permissions
- Dependencies
- Visual C++ runtime
- Antivirus interference

## 📧 Sending Logs to Support

When reporting issues, please include:
1. **`ABBook_Plugin_Debug.log`** - Complete debug information
2. **`ABBook_Plugin_Emergency.log`** - Critical errors (if exists)
3. **`plugin_diagnostic.log`** - System diagnostic information
4. **MT4 server error messages** - Any errors from MT4 server logs

## 🔧 Technical Details

### Logging Improvements
- **Timestamp precision**: Millisecond accuracy
- **Thread identification**: Thread ID logging for multi-threaded debugging
- **Error code translation**: Windows and WinSock error codes explained
- **Memory safety**: Extensive null pointer checking
- **Exception handling**: Comprehensive try-catch blocks

### Performance Impact
- **Minimal overhead**: Logging optimized for production use
- **File I/O optimization**: Efficient file writing with proper buffering
- **Memory efficient**: Smart string handling and resource management

### Security Considerations
- **No sensitive data**: Configuration values logged safely
- **File permissions**: Logs created with appropriate access rights
- **Thread safety**: Mutex protection prevents log corruption

## 📞 Support

If you need assistance interpreting the logs or resolving issues:
- **Priority 1**: Send all log files for analysis
- **Priority 2**: Run diagnostic script and send results
- **Priority 3**: Provide MT4 server version and system information

The enhanced logging will help us quickly identify and resolve the "invalid plugin" error your client is experiencing.

---

**Version**: Enhanced Debug v1.0
**Date**: 2025-01-XX
**Compatibility**: MT4/MT5 32-bit servers
**Dependencies**: Visual C++ Redistributable (x86) 