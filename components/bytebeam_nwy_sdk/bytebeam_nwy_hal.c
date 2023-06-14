 
#include "bytebeam_nwy_hal.h"

void print_hal_message()
{
    nwy_ext_echo("I am print_hal_message()\n");
    nwy_ext_echo("I will take care of the hal level\n");
    nwy_ext_echo("I am ~print_hal_message()\n");
}