#####################################################################################
#                                                                                   #
#                            Script Usage (nwy.py.exe)                              #
#                                                                                   #
#                   Shoew nwy.py Version  ---> nwy.py --version                     #
#                   Show help message     ---> nwy.py --help                        #
#                   Enter Boot Mode       ---> nwy.py boot                          #
#                   Trigger Soft Reset    ---> nwy.py reset                         #
#                   Build Application     ---> nwy.py build                         #
#                   Flash Executable      ---> nwy.py flash                         #
#                   Monitor Output        ---> nwy.py monitor                       #
#                                                                                   #
#####################################################################################

import re
import os
import sys
import time
import serial
import pyautogui
import subprocess
import serial.tools.list_ports

# Upgrade the version whenever there is a new change that you want to share with others
NWY_PY_VERSION = "1.0.3"

# AT Cmd Port Parameters (no need to fill we will identify)
AT_COM_PORT = 'COM'
AT_COM_BAUDRATE = 9600

# Monitor Port Parameters (no need to fill we will identify)
MON_COM_PORT = 'COM'
MON_COM_BAUDRATE = 9600

##
## Use below code snippet to set the mouse click whenever needed
##

# print("Starting Application in 3 sec...")
# time.sleep(3)
# print("Ok, Starting Application")

# print(pyautogui.size())
# pyautogui.moveTo(950, 1050, duration = 1)
# time.sleep(10000)

def identify_com_ports():
    global AT_COM_PORT
    global MON_COM_PORT

    available_ports = serial.tools.list_ports.comports()
    search_strings = {
        "open_con": "Neoway USB OPEN_CON",
        "at": "Neoway USB AT"
    }
    pattern = re.compile(r"([A-Za-z0-9]+(&[A-Za-z0-9]+)+)", re.IGNORECASE)
    hwid_to_port = {}
    for port, desc, hwid in available_ports:
        hwid_match = pattern.search(hwid)
        if hwid_match:
            hwid = hwid_match.group(0)
            if hwid not in hwid_to_port:
                hwid_to_port[hwid] = {}
            for key, search_string in search_strings.items():
                if desc.find(search_string) != -1:
                    hwid_to_port[hwid][key] = port

    for hwid in hwid_to_port.keys():
        if 'at' in hwid_to_port[hwid] and 'open_con' in hwid_to_port[hwid]:
            AT_COM_PORT  = hwid_to_port[hwid]['at']
            MON_COM_PORT = hwid_to_port[hwid]['open_con']

def show_version():
    print(NWY_PY_VERSION)

def show_help():
    print('Usage: nwy.py [COMMAND]\n')
    print('Commands:')
    print('--help\t\t\tshow help message')
    print('--version\t\tshow idf.py version')
    print('boot\t\t\tenter the boot mode')
    print('reset\t\t\treset the nwy modem')
    print('build\t\t\tbuild the application')
    print('flash\t\t\tflash the executable')
    print('monitor\t\t\tmonitor the output')
    print('build-ota\t\tbuild differential ota image')

def do_boot():
    ser_at = serial.Serial(AT_COM_PORT, AT_COM_BAUDRATE)
    ser_at.write('AT+CFUN=9\r\n'.encode('utf-8'))
    ser_at.close()

def do_reset():
    ser_at = serial.Serial(AT_COM_PORT, AT_COM_BAUDRATE)
    ser_at.write('AT+CFUN=1,1\r\n'.encode('utf-8'))
    ser_at.close()

def do_build():
    os.system('nwy_opencpu.bat && type log.txt')

def do_flash():
    # open app
    pyautogui.click(950, 1050)
    time.sleep(1)

    # load file
    pyautogui.click(45, 55)
    time.sleep(1)

    # select the file
    pyautogui.click(875, 425)
    time.sleep(0.1)
    pyautogui.click(875, 425)
    time.sleep(1)

    # start downloading
    pyautogui.click(195, 55)
    time.sleep(5)

    # stop downloading
    pyautogui.click(245, 55)
    time.sleep(1)

    # close app
    pyautogui.click(950, 1050)
    time.sleep(1)

def do_monitor():
    magic_idx = 0
    ser_port = False

    while 1:
        if ser_port:
            try:
                chr = ser_mon.read().decode('utf-8')
                print(chr, end="")
            except Exception as exception:
                print('\nError: Read COM Port')
                magic_idx = 0
                ser_port = False
        else:
            try:
                if magic_idx == 0:
                    print('Opening {} at {}bps'.format(MON_COM_PORT, MON_COM_BAUDRATE))

                identify_com_ports()
                ser_mon = serial.Serial(MON_COM_PORT, MON_COM_BAUDRATE)
                print('Listening to {} at {}bps'.format(MON_COM_PORT, MON_COM_BAUDRATE))
                ser_port = True
            except Exception as exception:
                magic_idx += 1

                if magic_idx % 10 == 0:
                    print('Error: Open COM Port')

                # keep trying every 100 ms
                time.sleep(0.1)

                ser_port = False

def do_build_ota():
    subprocess.call(["tools\\win32\\dtools", "pacmerge",
                     "--id", "APPIMG,PS", "build_ota\\N58-R08-STD-OE_V20_VOLTE_BYTEBEAM-002.pac",  "build_ota\\old.pac",  "merge_old.pac"])
    subprocess.call(["tools\\win32\\dtools", "pacmerge",
                     "--id", "APPIMG,PS", "build_ota\\N58-R08-STD-OE_V20_VOLTE_BYTEBEAM-002.pac",  "build_ota\\new.pac",  "merge_new.pac"])
    subprocess.call(["tools\\win32\\dtools", "fotacreate2",
                     "--pac", "merge_old.pac,merge_new.pac,tools\\win32\\setting\\fota8910.xml", "build_ota\\test_app.pack"])
    os.unlink("merge_old.pac")
    os.unlink("merge_new.pac")

def do_something():
    if sys.argv[1] == '--version':
        show_version()
    elif sys.argv[1] == '--help':
        show_help()
    elif sys.argv[1] == 'boot':
        do_boot()
    elif sys.argv[1] == 'reset' :
        do_reset()
    elif sys.argv[1] == 'build':
        do_build()
    elif sys.argv[1] == 'flash':
        do_flash()
    elif sys.argv[1] == 'monitor':
        do_monitor()
    elif sys.argv[1] == 'build-ota':
        do_build_ota()
    else:
        print('Invalid command')

if __name__ == "__main__":
    # identify the needy ports
    identify_com_ports()

    # do something :)
    do_something()