# Build a ffmpeg library for 3DS

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
git clone -b 3ds https://github.com/Core-2-Extreme/FFmpeg_for_3DS && cd FFmpeg_for_3DS && git reset --hard 2a054b87a7bacf3bc55690f621cf56ed9cf1ba5d && ./configure --enable-cross-compile --cross-prefix=/opt/devkitpro/devkitARM/bin/arm-none-eabi- --prefix=/opt/devkitpro/extra_lib --cpu=armv6k --arch=arm --target-os=linux --extra-cflags="-mfloat-abi=hard -mtune=mpcore -mtp=cp15 -Wno-error=incompatible-pointer-types -I/opt/devkitpro/extra_lib/include" --extra-ldflags="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib  -specs=3dsx.specs" --extra-libs="-lctru" --enable-optimizations --disable-filters --disable-devices --disable-bsfs --disable-parsers --disable-hwaccels --disable-debug --disable-stripping --disable-programs --disable-avdevice --disable-postproc --disable-avfilter --disable-decoders --disable-demuxers --disable-encoders --disable-muxers --disable-asm --disable-protocols --disable-txtpages --disable-podpages --disable-manpages --disable-htmlpages --disable-doc --enable-inline-asm --enable-vfp --enable-armv5te --enable-armv6 --enable-decoder="aac,ac3,flac,h261,h262,h263,h264,hevc,jpeg,jpeg2000,libdav1d,theora,mjpeg,mp1,mp2,mp3,mpeg1video,mpeg2video,mpeg4,msmpeg4*,opus,*pcm*,vorbis,vp9,webp,dvdsub,subrip,subviewer*,movtext" --enable-demuxer="aac,ac3,avi,h261,h262,h263,h264,hevc,matroska,m4v,mjpeg,mjpeg_2000,mpegvideo,mpjpeg,mp3,mov,*pcm*,ogg,vp8,vp9,wav,srt,subviewer*" --enable-encoder="aac,ac3,libmp3lame,mp2,mpeg4,mjpeg,mpeg2video,libx264" --enable-muxer="mp4,mp3,mp2,ac3" --enable-protocol="file" --enable-libx264 --enable-libmp3lame --enable-libdav1d --enable-libtheora --enable-gpl --enable-pthreads && make -j && sudo make install && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Changed arg name` (`2a054b87a7bacf3bc55690f621cf56ed9cf1ba5d`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/FFmpeg_for_3DS && cd FFmpeg_for_3DS && git reset --hard 2a054b87a7bacf3bc55690f621cf56ed9cf1ba5d
```

## Configure
```
./configure --enable-cross-compile --cross-prefix=/opt/devkitpro/devkitARM/bin/arm-none-eabi- --prefix=/opt/devkitpro/extra_lib --cpu=armv6k --arch=arm --target-os=linux --extra-cflags="-mfloat-abi=hard -mtune=mpcore -mtp=cp15 -Wno-error=incompatible-pointer-types -I/opt/devkitpro/extra_lib/include" --extra-ldflags="-mfloat-abi=hard -L/opt/devkitpro/extra_lib/lib  -specs=3dsx.specs" --extra-libs="-lctru" --enable-optimizations --disable-filters --disable-devices --disable-bsfs --disable-parsers --disable-hwaccels --disable-debug --disable-stripping --disable-programs --disable-avdevice --disable-postproc --disable-avfilter --disable-decoders --disable-demuxers --disable-encoders --disable-muxers --disable-asm --disable-protocols --disable-txtpages --disable-podpages --disable-manpages --disable-htmlpages --disable-doc --enable-inline-asm --enable-vfp --enable-armv5te --enable-armv6 --enable-decoder="aac,ac3,flac,h261,h262,h263,h264,hevc,jpeg,jpeg2000,libdav1d,theora,mjpeg,mp1,mp2,mp3,mpeg1video,mpeg2video,mpeg4,msmpeg4*,opus,*pcm*,vorbis,vp9,webp,dvdsub,subrip,subviewer*,movtext" --enable-demuxer="aac,ac3,avi,h261,h262,h263,h264,hevc,matroska,m4v,mjpeg,mjpeg_2000,mpegvideo,mpjpeg,mp3,mov,*pcm*,ogg,vp8,vp9,wav,srt,subviewer*" --enable-encoder="aac,ac3,libmp3lame,mp2,mpeg4,mjpeg,mpeg2video,libx264" --enable-muxer="mp4,mp3,mp2,ac3" --enable-protocol="file" --enable-libx264 --enable-libmp3lame --enable-libdav1d --enable-libtheora --enable-gpl --enable-pthreads
```

## Build and install
```
make -j && sudo make install
```

## Go to parent directory
```
cd ../
```

~~Then, continue to : [build zlib](11_zlib_build.md)~~ \
Currently, we don't use zlib so continue to [build mbedtls](12_mbedtls_build.md)
