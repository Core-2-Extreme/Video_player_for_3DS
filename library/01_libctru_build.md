# Build a custom libctru library

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
git clone -b 3ds https://github.com/Core-2-Extreme/libctru_custom && cd libctru_custom && git reset --hard 9ded1b1802637a61ef89b3e4b78844898507b051 && cd libctru && make -j && sudo -E make install && cd ../../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Moved hton and ntoh to os.c and removed static` (`9ded1b1802637a61ef89b3e4b78844898507b051`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/libctru_custom && cd libctru_custom && git reset --hard 9ded1b1802637a61ef89b3e4b78844898507b051
```

## Configure
```
echo No configurations are needed, continue to the next step. \(It is a good idea to check the command before copypasta everything.\)
```

## Build and install
```
cd libctru && make -j && sudo -E make install
```

## Go to parent directory
```
cd ../../
```

Then, continue to : [build citro3d](02_citro3d_build.md)
