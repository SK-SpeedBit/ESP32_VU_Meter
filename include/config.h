


// ── Debug Output ──────────────────────────────────────────────────────────────
// Define ENABLE_DEBUG=1 in platformio.ini build_flags to enable serial logging.

#if ENABLE_DEBUG == 1

#define DEBUG_PRINT(x)    Serial.print(x)
#define DEBUG_PRINTLN(x)  Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)

#else

#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)

#endif
