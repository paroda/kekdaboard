# Dependency

## Install build tools (Archlinux)

```sh
sudo pacman -Syu base-devel cmake
sudo pacman -Sy arm-none-eabi-binutils arm-none-eabi-gcc arm-none-eabi-newlib
```

## PICO SDK & TinyUSB

Add the PICO SDK as git submodules. And update the lib/tinyusb git submodule within it.

```sh
git submodule add https://github.com/raspberrypi/pico-sdk pico-sdk
cd pico-sdk
git submodule update --init
```

### commit ids
- pico-sdk - commit 2062372d203b372849d573f252cf7c6dc2800c0a, tag: 1.3.0
- lib/tinyusb - commit 9c8c5c1c53214b8954bb28c33a496b5b423d5ecc

# Build

```sh
mkdir build
cd build
cmake ..

make hid_composite
```

It will generate the binary target`build/hid_composite/hid_composite.uf2` which you can deploy to the pico board.
