# Download a jsmn library

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
git clone -b 3ds https://github.com/Core-2-Extreme/jsmn_for_3DS && cd jsmn_for_3DS && git reset --hard 25647e692c7906b96ffd2b05ca54c097948e879c && sudo cp jsmn.h /opt/devkitpro/extra_lib/include/ && cd ../ && echo Success.
```

## Clone and setup source code to specific version (commit)
Jsmn is a single header library, so no build are required, just download and copy it.
Used commit : `Fix position of a comment in string parsing` (`25647e692c7906b96ffd2b05ca54c097948e879c`).
```
git clone -b 3ds https://github.com/Core-2-Extreme/jsmn_for_3DS && cd jsmn_for_3DS && git reset --hard 25647e692c7906b96ffd2b05ca54c097948e879c
```

## Configure
```
echo No configurations are needed, continue to the next step. \(It is a good idea to check the command before copypasta everything.\)
```

## Install
```
sudo cp jsmn.h /opt/devkitpro/extra_lib/include/
```

## Go to parent directory
```
cd ../
```

Finally, continue to : [finalize](copy_libraries.md)
