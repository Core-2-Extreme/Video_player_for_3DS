## Copy libraries
After building libraries, copy these files :
```
/opt/devkitpro/extra_lib/lib/*
/opt/devkitpro/extra_lib/include/*
```

to :

```
{project_folder}/library/lib/
{project_folder}/library/include/
```

To build this project you only need `.a` files for binaries, so
```
cp -r /opt/devkitpro/extra_lib/lib/*.a {project_folder}/library/lib/ && cp -r /opt/devkitpro/extra_lib/include/* {project_folder}/library/include/
```
is enough.

Congratulations, you've done!!!!! \
Go back to [top](../README.md#build).
