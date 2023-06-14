# Tork Motors N58 Firmware

This repo contains production N58 firmware for tork devices

## Project Setup

```bash
# clone the repo
$ git clone https://github.com/bytebeamio/Tork_Motors_N58_FW.git

# step into cloned repo directory
$ cd Tork_Motors_N58_FW
```

## Project Usage

### Environment Setup

```bash
# setup nwy environment
$ nwy_env.bat
```

### Available Commands
```bash
# show nwy.py version
$ nwy.py --version

# show help message
$ nwy.py --help

# enter boot mode
$ nwy.py boot

# trigger soft reset
$ nwy.py reset

# build application
$ nwy.py build

# flash executable
$ nwy.py flash

# monitor output
$ nwy.py monitor

# build differential ota image
$ nwy.py build-ota
```

## Developer Mode

### Environment Setup

```bash
# setup nwy environment
$ nwy_env.bat

# install the python dependencies
$ pip install -r requirements.txt
```

Add your python site packges path to the prebuilts\win32\python3\python36._pth
### Making Changes

Make changes to the nwy.py script and test the same using below python command format

```bash
# python commands format
$ python script/nwy.py your_argument_goes_here
```

### Generating Binary

```bash
# step into script directory
$ cd script

# build nwy.py exectuable
$ build_nwy.py.bat
```

**Note:** If you would like to share the changes with others, We recomment to increase the nwy.py version based on your changes :)

