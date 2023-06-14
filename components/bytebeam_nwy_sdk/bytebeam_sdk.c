#include "bytebeam_nwy_hal.h"
#include "bytebeam_nwy_logger.h"
#include "bytebeam_sdk.h"

void print_sdk_message() 
{
    nwy_ext_echo("I am print_sdk_message()\n");
    nwy_ext_echo("I will take care of the sdk level\n");
    nwy_ext_echo("I am ~print_sdk_message()\n");
}

void show_sdk_info()
{
    print_hal_message();
    print_sdk_message();
}

void test_bytebeam_nwy_logger()
{
    BYTEBEAM_NWY_LOGN("I am %s log", "None");
    BYTEBEAM_NWY_LOGE("I am %s log", "Error");
    BYTEBEAM_NWY_LOGW("I am %s log", "Warn");
    BYTEBEAM_NWY_LOGI("I am %s log", "Info");
    BYTEBEAM_NWY_LOGD("I am %s log", "Debug");
    BYTEBEAM_NWY_LOGT("I am %s log", "Trace");
}