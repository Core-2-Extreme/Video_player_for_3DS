# Bbuild a mbedtls for 3DS

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
git clone -b 3ds https://github.com/Core-2-Extreme/mbedtls_for_3DS && cd mbedtls_for_3DS && git reset --hard ecf77d19bfc2b2630cccabb033ab7227ff6b0beb && git submodule update --init && cd tf-psa-crypto/ && git submodule update --init && cd ../ && cmake -S . -B build -DCMAKE_C_COMPILER="/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc" -DCMAKE_C_FLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -std=c99 -O3" -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF && cmake --build build --target lib -j 8 && sudo cmake --install build --prefix "/opt/devkitpro/extra_lib" && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Forgot to commit mbedtls config` (`ecf77d19bfc2b2630cccabb033ab7227ff6b0beb`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/mbedtls_for_3DS && cd mbedtls_for_3DS && git reset --hard ecf77d19bfc2b2630cccabb033ab7227ff6b0beb && git submodule update --init && cd tf-psa-crypto/ && git submodule update --init && cd ../
```

## Configure
Note : You may need to install jinja2 (`sudo apt install python3-jinja2`) if you haven't installed it.
```
cmake -S . -B build -DCMAKE_C_COMPILER="/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc" -DCMAKE_C_FLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -std=c99 -O3" -DCMAKE_C_STANDARD_LIRBARIES="-lctru" -DCMAKE_EXE_LINKER_FLAGS="-L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs" -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF
```

## Build and install
```
cmake --build build --target lib -j 8 && sudo cmake --install build --prefix "/opt/devkitpro/extra_lib"
```

## Go to parent directory
```
cd ../
```

Then, continue to : [build nghttp2](nghttp2_build.md)
