# How to install devkitpro

It works on WSL Ubuntu 20.04, is should also work on real Ubuntu.
As of this writing, we are using `devkitARM r64-2`. \
For more information, see [README](../README.md#build).

## Download the script

```
wget https://apt.devkitpro.org/install-devkitpro-pacman; chmod +x ./install-devkitpro-pacman
```

## Install devkitpro-pacman

```
sudo ./install-devkitpro-pacman
```

## Install devkitpro
Only 3dstools (11), devkit-env (12), devkitARM (13) and picasso (16) are required to build this project.

```
sudo dkp-pacman -S 3ds-dev

:: There are 17 members in group 3ds-dev:
:: Repository dkp-libs
	1) 3ds-cmake  2) 3ds-examples  3) 3ds-pkg-config  4) catnip  5) citro2d  6) citro3d  7) devkitarm-cmake  8) devkitarm-crtls  9) libctru
:: Repository dkp-linux
	1)  3dslink  11) 3dstools  12) devkit-env  13) devkitARM  14) devkitARM-gdb  15) general-tools  16) picasso  17) tex3ds

Enter a selection (default=all): 11,12,13,16
```

## Setup path

```
export DEVKITPRO=/opt/devkitpro; export DEVKITARM=${DEVKITPRO}/devkitARM; export PATH=${DEVKITPRO}/tools/bin:$PATH
```

## Build libraries (usually optional)
* Note : This step is usually optional.
	* You may need to do this when :
		* You want to make a change to the library or
		* You want to change build config such as optimization and debug flag or
		* You want to use different revision of `devkitARM` as stated above

If you wish to build them, then do so in this order.
1. [libctru](01_libctru_build.md)
2. [citro3d](02_citro3d_build.md)
3. [citro2d](03_citro2d_build.md)
4. [x264](04_x264_build.md)
5. [libmp3lame](05_libmp3lame_build.md)
6. [dav1d](06_dav1d_build.md)
7. [ffmpeg](07_ffmpeg_build.md)
8. [zlib](08_zlib_build.md)
9. [mbedtls](09_mbedtls_build.md)
10. [nghttp2](10_nghttp2_build.md)
11. [curl](11_curl_build.md)
12. [stb](12_stb.md)
13. [finalize](99_copy_libraries.md)
