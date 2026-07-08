# Bbuild a nghttp2 for 3DS

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
git clone -b 3ds https://github.com/Core-2-Extreme/nghttp2_for_3DS && cd nghttp2_for_3DS && git reset --hard 68cb6900fde14c77f0cd7add0e094a862960eb99 && autoreconf -fi && ./configure --host=arm-none-eabi CC=/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc --prefix=/opt/devkitpro/extra_lib CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -O3 -Wno-error=implicit-function-declaration" LDFLAGS="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" --disable-shared --enable-static --enable-lib-only && make -j 8 && sudo make install && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Update bash_completion` (`68cb6900fde14c77f0cd7add0e094a862960eb99`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/nghttp2_for_3DS && cd nghttp2_for_3DS && git reset --hard 68cb6900fde14c77f0cd7add0e094a862960eb99
```

## Configure
Note : You may need to install autoconf and libtool (`sudo apt install autoconf libtool`) if you haven't installed it.
```
autoreconf -fi && ./configure --host=arm-none-eabi CC=/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc --prefix=/opt/devkitpro/extra_lib CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -O3 -Wno-error=implicit-function-declaration" LDFLAGS="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" --disable-shared --enable-static --enable-lib-only
```

## Build and install
```
make -j 8 && sudo make install
```

## Go to parent directory
```
cd ../
```

Then, continue to : [build curl](curl_build.md)
