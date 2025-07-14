# Plugin Architecture Compatibility

## Error 193 - Architecture Mismatch

If you see the error:
```
API: 'ABBook_Plugin.dll' plugin dll load error [193]
```

This means there's an architecture mismatch between your MT4/MT5 server and the plugin.

## Solutions

### Option 1: Use 32-bit Plugin (Most Common)
Most MT4 servers are 32-bit. Use:
- **File:** `ABBook_Plugin_32bit.dll`
- **Build:** Run `build_plugin_32bit.bat`

### Option 2: Use 64-bit Plugin
If your MT4 server is 64-bit, use:
- **File:** `ABBook_Plugin.dll`
- **Build:** Run `build_plugin.bat`

### How to Check Your Server Architecture

1. **In MT4 Manager:** Tools → Options → About
2. **Look for:** "x64" = 64-bit, "x86" = 32-bit
3. **Or:** Check server .exe file properties

## Deployment

1. Copy the correct DLL to your MT4 server plugins directory
2. Restart the MT4 server
3. Check server logs for successful plugin loading

## Files Available

- `ABBook_Plugin.dll` - 64-bit version
- `ABBook_Plugin_32bit.dll` - 32-bit version
- `ABBook_Plugin_v1.0.zip` - Contains both versions 