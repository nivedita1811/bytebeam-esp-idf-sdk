#ifndef LOGGER_H
#define LOGGER_H

#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
 #define __file__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
 #define __file__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif /* defined(_WIN32) || defined(_WIN64) */

typedef enum {
    BYTEBEAM_NWY_LOG_NONE,
    BYTEBEAM_NWY_LOG_ERROR,
    BYTEBEAM_NWY_LOG_WARN,
    BYTEBEAM_NWY_LOG_INFO,
    BYTEBEAM_NWY_LOG_DEBUG,
    BYTEBEAM_NWY_LOG_TRACE
} LogLevel;

#define BYTEBEAM_NWY_LOGN(fmt, ...) bytebeam_nwy_log(BYTEBEAM_NWY_LOG_NONE,  __file__, __func__, fmt, ##__VA_ARGS__)
#define BYTEBEAM_NWY_LOGE(fmt, ...) bytebeam_nwy_log(BYTEBEAM_NWY_LOG_ERROR, __file__, __func__, fmt, ##__VA_ARGS__)
#define BYTEBEAM_NWY_LOGW(fmt, ...) bytebeam_nwy_log(BYTEBEAM_NWY_LOG_WARN , __file__, __func__, fmt, ##__VA_ARGS__)
#define BYTEBEAM_NWY_LOGI(fmt, ...) bytebeam_nwy_log(BYTEBEAM_NWY_LOG_INFO , __file__, __func__, fmt, ##__VA_ARGS__)
#define BYTEBEAM_NWY_LOGD(fmt, ...) bytebeam_nwy_log(BYTEBEAM_NWY_LOG_DEBUG, __file__, __func__, fmt, ##__VA_ARGS__)
#define BYTEBEAM_NWY_LOGT(fmt, ...) bytebeam_nwy_log(BYTEBEAM_NWY_LOG_TRACE, __file__, __func__, fmt, ##__VA_ARGS__)

void bytebeam_nwy_log_level_set(LogLevel);
LogLevel bytebeam_nwy_log_level_get(void);

void bytebeam_nwy_log(LogLevel level, const char* file, const char* func, const char* fmt, ...);

#endif /* LOGGER_H */