# Build a custom citro3d library

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
git clone -b 3ds https://github.com/Core-2-Extreme/citro3d_custom && cd citro3d_custom && git reset --hard 5166574ccc8131673c39c5a7d8648861d2db476a && make -j 8 && sudo -E make install && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Used commit : `Fixed install directory and libctru directory` (`5166574ccc8131673c39c5a7d8648861d2db476a`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/citro3d_custom && cd citro3d_custom && git reset --hard 5166574ccc8131673c39c5a7d8648861d2db476a
```

## Configure
```
echo No configurations are needed, continue to the next step. \(It is a good idea to check the command before copypasta everything.\)
```

## Build and install
```
make -j 8 && sudo -E make install
```

## Go to parent directory
```
cd ../
```

Then, continue to : [build citro2d](citro2d_build.md)
