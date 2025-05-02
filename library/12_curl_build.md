# Bbuild a curl for 3DS

It works on WSL Ubuntu 20.04, is should also work on real Ubuntu. \
As of this writing, we are using `devkitARM r64-2`. \
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
git clone -b 3ds https://github.com/Core-2-Extreme/curl_for_3DS && cd curl_for_3DS && git reset --hard d8aee88e8bca77ed4d6090fa58b550d4e23796e3 && autoreconf -fi && ./configure --host=arm-none-eabi CC=/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc --prefix=/opt/devkitpro/extra_lib CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -O3" CPPFLAGS="-I/opt/devkitpro/extra_lib/include" LDFLAGS="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs" LIBS="-lctru" --disable-shared --enable-static --enable-http --enable-ftp --enable-websockets --disable-threaded-resolver --disable-ntlm-wb --disable-docs --without-zstd --enable-file --enable-proxy --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-mqtt --disable-manual --disable-docs --disable-ipv6 --disable-unix-sockets --with-mbedtls --with-nghttp2 --without-zlib --without-libpsl --without-zstd && make -j && sudo make install && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Applied patch based on curl port for 3DS` (`d8aee88e8bca77ed4d6090fa58b550d4e23796e3`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/curl_for_3DS && cd curl_for_3DS && git reset --hard d8aee88e8bca77ed4d6090fa58b550d4e23796e3
```

## Configure
Note : You may need to install autoconf and libtool (`sudo apt install autoconf libtool`) if you haven't installed it.
```
autoreconf -fi && ./configure --host=arm-none-eabi CC=/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc --prefix=/opt/devkitpro/extra_lib CFLAGS="-march=armv6k -mfloat-abi=hard -mtune=mpcore -mtp=cp15 -O3" CPPFLAGS="-I/opt/devkitpro/extra_lib/include" LDFLAGS="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib -specs=3dsx.specs" LIBS="-lctru" --disable-shared --enable-static --enable-http --enable-ftp --enable-websockets --disable-threaded-resolver --disable-ntlm-wb --disable-docs --without-zstd --enable-file --enable-proxy --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-mqtt --disable-manual --disable-docs --disable-ipv6 --disable-unix-sockets --with-mbedtls --with-nghttp2 --without-zlib --without-libpsl --without-zstd
```

## Build and install
```
make -j && sudo make install
```

## Go to parent directory
```
cd ../
```

Then, continue to : [stb](13_stb.md)
