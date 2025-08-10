#!/bin/bash

source venv/bin/activate
cmake -B build -DARM_NONE_EABI_TOOLCHAIN_PATH=$HOME/local/gcc-arm-none-eabi-10.3-2021.10/ -DNRF5_SDK_PATH=$HOME/local/nRF5_SDK_15.3.0_59ac345/ .
cmake --build build
adafruit-nrfutil dfu genpkg --dev-type 0x0052 --application build/src/pinetime-mcuboot-app-image-1.15.0.bin build/dfu.zip
cp build/dfu.zip  ~/Sync
