# Build a dav1d library for 3DS

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
git clone -b 3ds https://github.com/Core-2-Extreme/dav1d_for_3DS && cd dav1d_for_3DS && git reset --hard 77086ffb99c33da9adfb497802195b0434126753 && mkdir build && cd build && meson setup --cross-file=../package/crossfiles/arm-3ds.meson --default-library=static && ninja && sudo meson install && cd ../../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Added ld flags` (`77086ffb99c33da9adfb497802195b0434126753`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/dav1d_for_3DS && cd dav1d_for_3DS && git reset --hard 77086ffb99c33da9adfb497802195b0434126753
```

## Configure
Note : You may need to install meson (`sudo apt install meson`) if you haven't installed it.
```
mkdir build && cd build && meson setup --cross-file=../package/crossfiles/arm-3ds.meson --default-library=static
```

## Build and install
```
ninja && sudo meson install
```

## Go to parent directory
```
cd ../../
```

Then, continue to : [build ffmpeg](07_ffmpeg_build.md)
