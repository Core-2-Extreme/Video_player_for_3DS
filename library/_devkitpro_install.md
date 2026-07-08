# How to install devkitpro

It works on PureOS 11, it should also work on other GNU/Linux based machines. \
As of this writing, we are using `devkitARM r68-1`.

## Download the script

```
wget https://apt.devkitpro.org/install-devkitpro-pacman; chmod +x ./install-devkitpro-pacman
```

## Install devkitpro-pacman

```
sudo ./install-devkitpro-pacman
```

## Install devkitpro

* Minimal configuration (to build `.3dsx`):
	* Only **devkitARM** (7), **3dstools** (13), **devkit-env** (14) and **picasso** (19) are required to build this project.
```
sudo dkp-pacman -S 3ds-dev
:: There are 20 members in group 3ds-dev:
:: Repository dkp-libs
1) 3ds-cmake  2) 3ds-examples  3) 3ds-pkg-config  4) catnip  5) citro2d  6) citro3d  7) devkitARM  8) devkitarm-cmake
2) devkitarm-crtls  10) devkitarm-newlib  11) libctru
:: Repository dkp-linux
1)  3dslink  13) 3dstools  14) devkit-env  15) devkitARM-gdb  16) devkitarm-binutils  17) devkitarm-gcc  18) general-tools  19) picasso
2)  tex3ds

Enter a selection (default=all): 7,13,14,19     
resolving dependencies...
looking for conflicting packages...

Packages (10) devkitarm-binutils-2.46.0-1  devkitarm-crtls-1.2.6-1  devkitarm-gcc-16.1.0-1  devkitarm-newlib-4.6.0.20260123-5
			devkitarm-rules-1.6.0-4  general-tools-1.4.4-1  3dstools-1.3.1-3  devkit-env-1.0.1-2  devkitARM-r68-1  picasso-2.7.2-3

Total Download Size:    92.06 MiB
Total Installed Size:  380.21 MiB

:: Proceed with installation? [Y/n] y
:: Retrieving packages...

...
...
...

dkp-pacman -Q
3dstools 1.3.1-3
devkit-env 1.0.1-2
devkitARM r68-1
devkitarm-binutils 2.46.0-1
devkitarm-crtls 1.2.6-1
devkitarm-gcc 16.1.0-1
devkitarm-newlib 4.6.0.20260123-5
devkitarm-rules 1.6.0-4
devkitpro-keyring 20180316-1
general-tools 1.4.4-1
pacman 6.0.1-7
picasso 2.7.2-3
```
* Full configuration (to debug application etc...):
```
sudo dkp-pacman -S 3ds-dev
:: There are 20 members in group 3ds-dev:
:: Repository dkp-libs
1) 3ds-cmake  2) 3ds-examples  3) 3ds-pkg-config  4) catnip  5) citro2d  6) citro3d  7) devkitARM  8) devkitarm-cmake
2) devkitarm-crtls  10) devkitarm-newlib  11) libctru
:: Repository dkp-linux
1)  3dslink  13) 3dstools  14) devkit-env  15) devkitARM-gdb  16) devkitarm-binutils  17) devkitarm-gcc  18) general-tools  19) picasso
2)  tex3ds

Enter a selection (default=all): 
resolving dependencies...
looking for conflicting packages...

Packages (22) devkitarm-rules-1.6.0-4  dkp-cmake-common-utils-1.5.3-1  3ds-cmake-1.5.2-1  3ds-examples-20240917-1  3ds-pkg-config-0.28-5
			3dslink-0.6.3-1  3dstools-1.3.1-3  catnip-0.2.0-1  citro2d-1.7.0-1  citro3d-1.7.1-2  devkit-env-1.0.1-2  devkitARM-r68-1
			devkitARM-gdb-14.1-2  devkitarm-binutils-2.46.0-1  devkitarm-cmake-1.2.4-1  devkitarm-crtls-1.2.6-1  devkitarm-gcc-16.1.0-1
			devkitarm-newlib-4.6.0.20260123-5  general-tools-1.4.4-1  libctru-2.7.0-1  picasso-2.7.2-3  tex3ds-2.3.0-4

Total Download Size:   105.84 MiB
Total Installed Size:  415.84 MiB

:: Proceed with installation? [Y/n] y
:: Retrieving packages...

...
...
...

dkp-pacman -Q
3ds-cmake 1.5.2-1
3ds-examples 20240917-1
3ds-pkg-config 0.28-5
3dslink 0.6.3-1
3dstools 1.3.1-3
catnip 0.2.0-1
citro2d 1.7.0-1
citro3d 1.7.1-2
devkit-env 1.0.1-2
devkitARM r68-1
devkitARM-gdb 14.1-2
devkitarm-binutils 2.46.0-1
devkitarm-cmake 1.2.4-1
devkitarm-crtls 1.2.6-1
devkitarm-gcc 16.1.0-1
devkitarm-newlib 4.6.0.20260123-5
devkitarm-rules 1.6.0-4
devkitpro-keyring 20180316-1
dkp-cmake-common-utils 1.5.3-1
general-tools 1.4.4-1
libctru 2.7.0-1
pacman 6.0.1-7
picasso 2.7.2-3
tex3ds 2.3.0-4
```

If you want to build `.cia`, then you also need (you don't need full configuration to build `.cia`, minimal configuration + these tools are enough):
* [bannertool](https://github.com/diasurgical/bannertool/releases/tag/1.2.0)
  * Broken links? Try our [backup](https://github.com/Core-2-Extreme/bannertool_fork/releases/tag/1.2.0).
* [makerom](https://github.com/3DSGuy/Project_CTR/releases/tag/makerom-v0.18.4)
  * Broken links? Try our [backup](https://github.com/Core-2-Extreme/Project_CTR_fork/releases/tag/makerom-v0.18.4).
* (Copy them in your path e.g. in `{devkitPro_install_dir}\tools\bin`).
```
bannertool 1.2.0
makerom v0.18.4
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
1. [libctru](libctru_build.md)
2. [citro3d](citro3d_build.md)
3. [citro2d](citro2d_build.md)
4. [zlib](zlib_build.md)
5. [x264](x264_build.md)
6. [libmp3lame](libmp3lame_build.md)
7. [dav1d](dav1d_build.md)
8. [ffmpeg](ffmpeg_build.md)
9. [mbedtls](mbedtls_build.md)
10. [nghttp2](nghttp2_build.md)
11. [curl](curl_build.md)
12. [stb](stb.md)
13. [jsmn](jsmn.md)
14. [finalize](copy_libraries.md)
