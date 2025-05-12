# Build a libmp3lame library for 3DS

It works on Ubuntu 24.04, it should also work on WSL. \
As of this writing, we are using `devkitARM r65-1`. \
For more information, see [README](../README.md#build).

* **⚠️Install [devkitpro](00_devkitpro_install.md) first.⚠️**
* Note : This step is optional.
	* You may need to do this when :
		* You want to make a change to the library or
		* You want to change build config such as optimization and debug flag or
		* You want to use different revision of `devkitARM` as stated above

## All-in-one command
If you've done it before or experienced user, then just use this all-in-one command (and make an adjustment if needed such as -j value). \
If you want to know in detail, continue to the next section for step-by-step instructions.
```
git clone -b 3ds https://github.com/Core-2-Extreme/libmp3lame_for_3DS && cd libmp3lame_for_3DS && git reset --hard f416c19b3140a8610507ebb60ac7cd06e94472b8 && ./configure --host=arm-none-eabi CC=/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc --prefix=/opt/devkitpro/extra_lib CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -O3" LDFLAGS="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" --enable-shared=no --disable-frontend --disable-decoder --disable-analyzer-hooks && make -j 8 && sudo make install && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `lame: disable the decoder in the config files` (`f416c19b3140a8610507ebb60ac7cd06e94472b8`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/libmp3lame_for_3DS && cd libmp3lame_for_3DS && git reset --hard f416c19b3140a8610507ebb60ac7cd06e94472b8
```

## Configure
```
./configure --host=arm-none-eabi CC=/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc --prefix=/opt/devkitpro/extra_lib CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -O3" LDFLAGS="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" --enable-shared=no --disable-frontend --disable-decoder --disable-analyzer-hooks
```

## Build and install
```
make -j 8 && sudo make install
```

## Go to parent directory
```
cd ../
```

Then, continue to : [build dav1d](06_dav1d_build.md)
