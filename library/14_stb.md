# Download a stb library

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
git clone -b 3ds https://github.com/Core-2-Extreme/stb_for_3DS && cd stb_for_3DS && git reset --hard f75e8d1cad7d90d72ef7a4661f1b994ef78b4e31 && sudo cp stb_image.h /opt/devkitpro/extra_lib/include/ && sudo cp stb_image_write.h /opt/devkitpro/extra_lib/include/ && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Stb is a single header library, so no build are required, just download and copy it.
Used commit : `update README` (`f75e8d1cad7d90d72ef7a4661f1b994ef78b4e31`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/stb_for_3DS && cd stb_for_3DS && git reset --hard f75e8d1cad7d90d72ef7a4661f1b994ef78b4e31
```

## Configure
```
echo No configurations are needed, continue to the next step. \(It is a good idea to check the command before copypasta everything.\)
```

## Install
```
sudo cp stb_image.h /opt/devkitpro/extra_lib/include/ && sudo cp stb_image_write.h /opt/devkitpro/extra_lib/include/
```

## Go to parent directory
```
cd ../
```

Finally, continue to : [finalize](99_copy_libraries.md)
