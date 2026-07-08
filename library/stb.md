# Download a stb library

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
git clone -b 3ds https://github.com/Core-2-Extreme/stb_for_3DS && cd stb_for_3DS && git reset --hard 8c3b4f1a58aa77f9d020a3c9f53847e231e37fc5 && sudo cp stb_image.h /opt/devkitpro/extra_lib/include/ && sudo cp stb_image_write.h /opt/devkitpro/extra_lib/include/ && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Stb is a single header library, so no build are required, just download and copy it.
Used commit : `Suppressed warnings` (`8c3b4f1a58aa77f9d020a3c9f53847e231e37fc5`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/stb_for_3DS && cd stb_for_3DS && git reset --hard 8c3b4f1a58aa77f9d020a3c9f53847e231e37fc5
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

Then, continue to : [jsmn](jsmn.md)
