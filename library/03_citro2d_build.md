# Build a custom citro2d library

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
git clone -b 3ds https://github.com/Core-2-Extreme/citro2d_custom && cd citro2d_custom && git reset --hard ccd5a8260c0ad8f0606e98bd9fa2fb6a0c2d9cce && make -j && sudo -E make install && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Fixed install directory and libctru directory` (`ccd5a8260c0ad8f0606e98bd9fa2fb6a0c2d9cce`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/citro2d_custom && cd citro2d_custom && git reset --hard ccd5a8260c0ad8f0606e98bd9fa2fb6a0c2d9cce
```

## Configure
```
echo No configurations are needed, continue to the next step. \(It is a good idea to check the command before copypasta everything.\)
```

## Build and install
```
make -j && sudo -E make install
```

## Go to parent directory
```
cd ../
```

Then, continue to : [build x264](04_x264_build.md)
