# Build a dav1d library for 3DS

It works on PureOS 11, it should also work on other GNU/Linux based machines. \
As of this writing, we are using `devkitARM r68-1`.

* **⚠️Install [devkitpro](_devkitpro_install.md) first.⚠️**
* Note : This step is optional.
	* You may need to do this when :
		* You want to make a change to the library or
		* You want to change build config such as optimization and debug flag or
		* You want to use different revision of `devkitARM` as stated above

## All-in-one command
If you've done it before or experienced user, then just use this all-in-one command (and make an adjustment if needed). \
If you want to know in detail, continue to the next section for step-by-step instructions.
```
git clone -b 3ds https://github.com/Core-2-Extreme/dav1d_for_3DS && cd dav1d_for_3DS && git reset --hard c40d9602629d39ae63bedd50c31fd926fa5eb51e && mkdir build && cd build && meson setup --cross-file=../package/crossfiles/arm-3ds.meson --default-library=static && ninja && sudo meson install && cd ../../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Merge branch 'master' into 3ds (NEWS for 1.5.3)` (`c40d9602629d39ae63bedd50c31fd926fa5eb51e`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/dav1d_for_3DS && cd dav1d_for_3DS && git reset --hard c40d9602629d39ae63bedd50c31fd926fa5eb51e
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

Then, continue to : [build ffmpeg](ffmpeg_build.md)
