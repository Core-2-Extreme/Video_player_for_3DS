# Build a x264 library for 3DS

It works on Ubuntu 24.04, it should also work on WSL. \
As of this writing, we are using `devkitARM r65-1`. \
For more information, see [README](../README.md#build).

* **⚠️Install [devkitpro](_devkitpro_install.md) first.⚠️**
* Note : This step is optional.
	* You may need to do this when :
		* You want to make a change to the library or
		* You want to change build config such as optimization and debug flag or
		* You want to use different revision of `devkitARM` as stated above

## All-in-one command
If you've done it before or experienced user, then just use this all-in-one command (and make an adjustment if needed such as -j value). \
If you want to know in detail, continue to the next section for step-by-step instructions.
```
git clone -b 3ds https://github.com/Core-2-Extreme/x264_for_3DS && cd x264_for_3DS && git reset --hard eaa68fad9e5d201d42fde51665f2d137ae96baf0 && ./configure --host=arm-linux --cross-prefix=/opt/devkitpro/devkitARM/bin/arm-none-eabi- --prefix=/opt/devkitpro/extra_lib --disable-asm --disable-thread --disable-opencl --extra-cflags="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -Wno-error=incompatible-pointer-types" --extra-ldflags="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" --bit-depth=8 --chroma-format=420 --disable-avs --enable-static --disable-cli && make -j 8 && sudo make install && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Fix high bit depth deinterleave of YUYV or UYVY` (`eaa68fad9e5d201d42fde51665f2d137ae96baf0`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/x264_for_3DS && cd x264_for_3DS && git reset --hard eaa68fad9e5d201d42fde51665f2d137ae96baf0
```

## Configure
```
./configure --host=arm-linux --cross-prefix=/opt/devkitpro/devkitARM/bin/arm-none-eabi- --prefix=/opt/devkitpro/extra_lib --disable-asm --disable-thread --disable-opencl --extra-cflags="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -Wno-error=incompatible-pointer-types" --extra-ldflags="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" --bit-depth=8 --chroma-format=420 --disable-avs --enable-static --disable-cli
```

## Build and install
```
make -j 8 && sudo make install
```

## Go to parent directory
```
cd ../
```

Then, continue to : [build libmp3lame](libmp3lame_build.md)
