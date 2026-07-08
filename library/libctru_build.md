# Build a custom libctru library

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
git clone -b 3ds https://github.com/Core-2-Extreme/libctru_custom && cd libctru_custom && git reset --hard 4d27b41b1df123057c6701d3d5ca2b56962eaee8 && cd libctru && make -j 8 && sudo -E make install && cd ../../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Suppressed warnings` (`4d27b41b1df123057c6701d3d5ca2b56962eaee8`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/libctru_custom && cd libctru_custom && git reset --hard 4d27b41b1df123057c6701d3d5ca2b56962eaee8
```

## Configure
```
echo No configurations are needed, continue to the next step. \(It is a good idea to check the command before copypasta everything.\)
```

## Build and install
```
cd libctru && make -j 8 && sudo -E make install
```

## Go to parent directory
```
cd ../../
```

Then, continue to : [build citro3d](citro3d_build.md)
