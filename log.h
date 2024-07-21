#include <stdio.h>

/* out log file */

#define LOG_COLOR "\033[0m"
#define INFO_COLOR "\033[0;32m"
#define DEBUG_COLOR "\033[0;33m"
#define ERROR_COLOR "\033[0;31m"

#define Log(format, ...) fprintf(stdout, format "\n", ##__VA_ARGS__)
#define Info(format, ...) fprintf(stdout, INFO_COLOR"[INFO] " format "\n" LOG_COLOR, ##__VA_ARGS__)
#define Debug(format, ...) fprintf(stdout, DEBUG_COLOR"[Debug] " format "\n" LOG_COLOR, ##__VA_ARGS__)
#define Error(format, ...) fprintf(stdout, ERROR_COLOR "[Error] " format "\n" LOG_COLOR, ##__VA_ARGS__)