# Bbuild a mbedtls for 3DS

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
git clone -b 3ds https://github.com/Core-2-Extreme/mbedtls_for_3DS && cd mbedtls_for_3DS && git reset --hard f34674083bced4c8802ee9630ca9eb113856e8a6 && git submodule update --init && make lib -j 8 CC="/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc" CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -std=c99 -O3" LDFLAGS=" -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" && sudo make install DESTDIR="/opt/devkitpro/extra_lib" && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Applied changes to compile with devkitpro` (`f34674083bced4c8802ee9630ca9eb113856e8a6`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/mbedtls_for_3DS && cd mbedtls_for_3DS && git reset --hard f34674083bced4c8802ee9630ca9eb113856e8a6 && git submodule update --init
```

## Configure
```
echo Configurations are passed to the make when building, continue to the next step. \(It is a good idea to check the command before copypasta everything.\)
```

## Build and install
```
make lib -j 8 CC="/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc" CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -std=c99 -O3" LDFLAGS=" -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs -lctru" && sudo make install DESTDIR="/opt/devkitpro/extra_lib"
```

## Go to parent directory
```
cd ../
```

Then, continue to : [build nghttp2](nghttp2_build.md)
