#ifndef COMMON_H_FED49E88_6D61_11E4_BC38_10FEED04CD1C
#define COMMON_H_FED49E88_6D61_11E4_BC38_10FEED04CD1C

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
} LogType;

void printLog(LogType type, const char* fmt, ...);

#define printInfoLog(...)    printLog(LOG_INFO,  __VA_ARGS__);
#define printWarningLog(...) printLog(LOG_WARNING,  __VA_ARGS__);
#define printErrorLog(...)   printLog(LOG_ERROR,  __VA_ARGS__);

#ifdef __cplusplus
}
#endif

#endif // COMMON_H_FED49E88_6D61_11E4_BC38_10FEED04CD1C

