# Build a libogg library for 3DS

It works on Debian 12.10 \
As of this writing, we are using `devkitARM r65`. \
For more information, see [README](../README.md#build).

* **⚠️Install [devkitpro](00_devkitpro_install.md) first.⚠️**
* Note : This step is optional.
	* You may need to do this when :
		* You want to make a change to the library or
		* You want to change build config such as optimization and debug flag or
		* You want to use different revision of `devkitARM` as stated above

## All-in-one command
If you've done it before or experienced user, then just use this all-in-one command (and make an adjustment if needed). \
If you want to know in detail, continue to the next section for step-by-step instructions.
```
git clone -b 3ds https://github.com/Core-2-Extreme/libogg_for_3DS && cd libogg_for_3DS && ./autogen.sh && ./configure --host=arm-none-eabi CC=/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc --prefix=/opt/devkitpro/extra_lib CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -O3" LDFLAGS="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" --enable-shared=no  && make -j && sudo make install && cd ../ && echo Success.
```

## Clone and setup source code
```
git clone -b 3ds https://github.com/Core-2-Extreme/libogg_for_3DS && cd libogg_for_3DS
```

## Configure
```
./autogen.sh && ./configure --host=arm-none-eabi CC=/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc --prefix=/opt/devkitpro/extra_lib CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -O3" LDFLAGS="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" --enable-shared=no
```

## Build and install
```
make -j && sudo make install
```

## Go to parent directory
```
cd ../
```

Then, continue to : [build libtheora](08_libtheora_build.md)
