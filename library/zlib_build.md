# Bbuild a zlib for 3DS

It works on PureOS 11, it should also work on other GNU/Linux based machines. \
As of this writing, we are using `devkitARM r68-1`.

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
git clone -b 3ds https://github.com/Core-2-Extreme/zlib_for_3DS && cd zlib_for_3DS && git reset --hard da607da739fa6047df13e66a2af6b8bec7c2a498 && CC="/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc" CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -std=c99 -O3 -Wno-error=implicit-function-declaration" LDFLAGS=" -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" ./configure --static --prefix="/opt/devkitpro/extra_lib" && make -j 8 && sudo make install && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `zlib 1.3.2` (`da607da739fa6047df13e66a2af6b8bec7c2a498`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/zlib_for_3DS && cd zlib_for_3DS && git reset --hard da607da739fa6047df13e66a2af6b8bec7c2a498
```

## Configure
```
CC="/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc" CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -std=c99 -O3 -Wno-error=implicit-function-declaration" LDFLAGS=" -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" ./configure --static --prefix="/opt/devkitpro/extra_lib"
```

## Build and install
```
make -j 8 && sudo make install
```

## Go to parent directory
```
cd ../
```

Then, continue to : [x264](x264_build.md)
