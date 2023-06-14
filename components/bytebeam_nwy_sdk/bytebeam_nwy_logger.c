#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "bytebeam.h"
#include "bytebeam_nwy_logger.h"

static LogLevel bytebeam_nwy_log_level = BYTEBEAM_NWY_LOG_TRACE;

static void nwy_log_prefix(LogLevel level, const char* file, const char* func)
{
    // print the bytebeam neoway logger prefix
    nwy_ext_echo("BytebeamNwy ");

    // print the debug level
    switch (level) {
        case BYTEBEAM_NWY_LOG_NONE  : nwy_ext_echo("NONE");  break;
        case BYTEBEAM_NWY_LOG_ERROR : nwy_ext_echo("ERROR"); break;
        case BYTEBEAM_NWY_LOG_WARN  : nwy_ext_echo("WARN");  break;
        case BYTEBEAM_NWY_LOG_INFO  : nwy_ext_echo("INFO");  break;
        case BYTEBEAM_NWY_LOG_DEBUG : nwy_ext_echo("DEBUG"); break;
        case BYTEBEAM_NWY_LOG_TRACE : nwy_ext_echo("TRACE"); break;

        default: nwy_ext_echo("UNKNOWN");
    }

    char module[20];

    // extract the module name i.e (remove the file externsion .c)
    int len = strlen(file) - 2;
    strncpy(module, file, len);
    module[len] = '\0';

    // print the module name and func name
    nwy_ext_echo(" %s::%s: ", module, func);
}   

void bytebeam_nwy_log_level_set(LogLevel level)
{
    bytebeam_nwy_log_level = level;
}

LogLevel bytebeam_nwy_log_level_get(void)
{
    return bytebeam_nwy_log_level;
}

void bytebeam_nwy_log(LogLevel level, const char* file, const char* func, const char* fmt, ...)
{
    static char nwy_msg[1024];

    // befor steppin ahead make sure you are not above bytebeam nwy log level
    if(level > bytebeam_nwy_log_level) {
        return;
    }

    va_list args;

    // get the message in the buffer
    va_start(args, fmt);
    int temp_var = vsnprintf(nwy_msg, 1024, fmt, args);
    va_end(args);

    // Check for any argument loss
    if(temp_var >= 1024) {
        return;
    }

    // start by printing the prefix
    nwy_log_prefix(level, file, func);
        
    // print the actual message
    nwy_ext_echo(nwy_msg);
    nwy_ext_echo("\n");
}