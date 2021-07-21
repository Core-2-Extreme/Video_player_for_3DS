# Video player for 3DS

## Patch note
### v1.2.0
Hardware decoding won't work in .cia has been fixed \
Hardware decoding won't work in some resolution has been fixed \
Added hardware color conversion for software decoder \
Added 3D video support(software decoder only) \
Other minor changes and optimization

### v1.1.1
Video will not be decoded correctly in hardware decoder has been fixed

### v1.1.0
Added hardware decoder (β)

### v1.0.1
Added allow skip frames option

### v1.0.0
Initial release

## Summary
Video player for 3DS \
![00000026](https://user-images.githubusercontent.com/45873899/113381113-12e28500-93b9-11eb-8539-7e2f64491a58.jpg)

## Features
Hardware accelerated decoding(*0)✅ \
Hardware accelerated color conversion✅ \
Multiple video codec support(MPEG4, H.264, H.265, Motion Jpeg) ✅ \
Multiple audio codec support(mp1, mp2, mp3, ac3, aac, ogg, pcm audio) ✅ \
Seek ✅ \
3D video(*1)✅ \
Zoom in/out video ✅ \
Move video ✅ \
File explorer ✅

### *0 New 3DS or New 2DS only
### *1 New 3DS or 3DS only, software decoder only, in order to see 3D video as 3D
### you need to enable 3D mode in settings(settings->LCD->Screen mode->3D)

### Performance

Software decoding
* 256x144(144p)@30fps(H.264) on OLD 3DS
* 640x360(360p)@24fps(H.264) on NEW 3DS

Hardware decoding
* 854x480(480p)@40~50fps(H.264) on NEW 3DS

Known issues : 
* ~~Video won't play in some resolution~~ (fixed in v1.2.0)
* ~~Video will not be decoded correctly at first~~ (fixed in v1.1.1)
* ~~It does not work in .cia~~ (fixed in v1.2.0)

### Supported video codec
* Motion jpeg
* MPEG4 (MPEG4 part2)
* H.264 (MPEG4 part10)
* H.265 (HEVC)

### Supported audio codec
* mp1 (MPEG audio layer 1)
* mp2 (MPEG audio layer 2)
* mp3 (MPEG audio layer 3)
* ac3
* aac (Advanced audio coding)
* ogg (Vorbis)
* pcm audio

### Contorols
* A : Play/Pause
* B : Stop
* Y : Debug
* X : Select file
* R : Zoom in
* L : Zoom out
* C/DPAD : Move video
* touch the bar : Seek

### Credits
* Core 2 Extreme
* dixy52-beep (icon, banner, in app texture)
