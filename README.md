# Video player for 3DS

## Patch note
### v1.4.0
Decoded image(raw) buffer has been added and it makes playback \
much much much much better (especially on OLD3DS)
Spanish(español) translation has been added (by Cookiee) \
Romanian(Română) translation has been added (by Tescu48)

### v1.3.3
'aspect ratio 10:3 mode' has been changed to 'correct aspect ratio mode' (follow sar value(*0)) \
Color conversion speed has been improved \
Simplified Chinese(简体中文) translation has been added (by LITTOMA) \
Italian(italiano) translation has been added (by dixy52-beep) \
*0 if video size is 800x240 and no sar value is set, it autmatically apply sar 1:2

### v1.3.2
Added aspect ratio 10:3 mode (for 800x240 videos) \
Added disable resize and move video mode \
Added remember video pos mode (resume from that pos next time) \
Other minor changes

### v1.3.1
Volume adjustment has been added (from 0% to 999%) \
Direction pad seeking has been added (from 1 second to 99 seconds) \
Hungarian(magyar) translation has been added (by vargaviktor) \
Other minor changes

### v1.3.0
The video that has more than one audio tracks has been supported (Press Y key-> select audio track to select track) \
Multi-threaded decoding has been supported (Press Y key-> use multi-threaded decoding to toggle) \
Hardware decoder has been merged (Press Y key-> use hw decoder to toggle) \
Full screen mode has been supported (Press select key to toggle) \
Other minor changes

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

⚠️ *0 New 3DS and New 2DS only \
⚠️ *1 New 3DS and 3DS only, software decoder only, in order to see 3D video as 3D \
you need to enable 3D mode in settings(settings->LCD->Screen mode->3D)

## Performance

⚠️ Decoding speed depends on encoder option, video type, video scene, etc... \
⚠️ This table shows average fps, so you may hear stutter audio if you use this framerate. \
(Lower video resolution or framerate in that case) \
Software decoding in this table uses only one thread not multi-threaded decoding.

#### MPEG1video
MPEG1video test file was encoded following command : \
ffmpeg -i {input_file_name} -acodec copy -vcodec mpeg1video -s {width}x{height} -r 30 -q:v 15 {output_file_name}

|        MPEG1video        | 256x144 (144p) | 426x240 (240p) | 640x360 (360p) | 800x240 | 854x480 (480p) |
| ------------------------ | -------------- | -------------- | -------------- | ------- | -------------- |
| OLD3DS Software decoding |     69.0fps    |     39.9fps    |     24.0fps    | 27.4fps |     16.7fps    |
| NEW3DS Software decoding |    532.0fps    |    267.2fps    |    119.8fps    |158.0fps |     69.4fps    |

#### MPEG2video
MPEG2video test file was encoded following command : \
ffmpeg -i {input_file_name} -acodec copy -vcodec mpeg2video -s {width}x{height} -r 30 -q:v 15 {output_file_name}

|        MPEG2video        | 256x144 (144p) | 426x240 (240p) | 640x360 (360p) | 800x240 | 854x480 (480p) |
| ------------------------ | -------------- | -------------- | -------------- | ------- | -------------- |
| OLD3DS Software decoding |     67.1fps    |     37.6fps    |     22.3fps    | 26.3fps |     15.4fps    |
| NEW3DS Software decoding |    518.8fps    |    254.2fps    |    113.9fps    |145.7fps |     65.9fps    |

#### H263+
H263+ test file was encoded following command : \
ffmpeg -i {input_file_name} -acodec copy -vcodec h263p -s {width}x{height} -r 30 -q:v 15 {output_file_name}

|          H.263+          | 256x144 (144p) | 424x240 (240p) | 640x360 (360p) | 800x240 | 856x480 (480p) |
| ------------------------ | -------------- | -------------- | -------------- | ------- | -------------- |
| OLD3DS Software decoding |     62.8fps    |     35.6fps    |     21.1fps    | 24.6fps |      8.7fps    |
| NEW3DS Software decoding |    527.9fps    |    257.8fps    |    113.1fps    |144.2fps |     33.9fps    |

#### H264
H264 test file was encoded following command : \
ffmpeg -i {input_file_name} -acodec copy -vcodec libx264 -s {width}x{height} -r 30 -preset fast -profile:v baseline  {output_file_name}

|          H.264           | 256x144 (144p) | 426x240 (240p) | 640x360 (360p) | 800x240 | 854x480 (480p) |
| ------------------------ | -------------- | -------------- | -------------- | ------- | -------------- |
| OLD3DS Software decoding |     30.7fps    |     15.9fps    |      8.8fps    | 10.2fps |      5.4fps    |
| NEW3DS Software decoding |    226.7fps    |     95.1fps    |     43.4fps    | 53.2fps |     25.1fps    |
| NEW3DS Hardware decoding |    560.6fps    |    338.7fps    |    206.0fps    |235.3fps |    114.7fps    |

#### H265
H265 test file was encoded following command : \
ffmpeg -i {input_file_name} -acodec copy -vcodec libx265 -s {width}x{height} -r 30 -preset fast -profile:v main  {output_file_name}

|          H.265           | 256x144 (144p) | 426x240 (240p) | 640x360 (360p) | 800x240 | 854x480 (480p) |
| ------------------------ | -------------- | -------------- | -------------- | ------- | -------------- |
| OLD3DS Software decoding |     22.4fps    |     11.3fps    |      6.2fps    |  7.2fps |      3.9fps    |
| NEW3DS Software decoding |    136.8fps    |     55.7fps    |     26.4fps    | 30.9fps |     15.3fps    |

Known issues : 
* ~~Video won't play in some resolution~~ (fixed in v1.2.0)
* ~~Video will not be decoded correctly at first~~ (fixed in v1.1.1)
* ~~It does not work in .cia~~ (fixed in v1.2.0 by 
windows-server-2003)
* If video contain B-frames, hardware decoder won't play it smoothly. \
Workaround : encode your video without using B-frames. \
`ffmpeg -i {input_file_name} -acodec copy -vcodec h264 -crf 20 -s {width}x{height} -x264-params bframes=0 {output_file_name}`

## Supported languages
* English
* Japanese/日本語
* Hungarian/Magyar (translated by vargaviktor)
* Simplified Chinese/简体中文 (translated by LITTOMA)
* Italian/Italiano (translated by dixy52-beep)
* Spanish/Español (translated by Cookiee)
* Romanian/Română (translated by Tescu48)

## Supported video codec
* Motion jpeg
* MPEG4 (MPEG4 part2)
* MPEG1video
* MPEG2video
* H.263
* H.263+
* H.264 (AVC, MPEG4 part10)
* H.265 (HEVC)

## Supported audio codec
* mp1 (MPEG audio layer 1)
* mp2 (MPEG audio layer 2)
* mp3 (MPEG audio layer 3)
* ac3
* aac (Advanced audio coding)
* ogg (Vorbis)
* pcm audio

## Controls
* A : Play/Pause
* B : Stop
* Y : Debug
* X : Select file
* R : Zoom in
* L : Zoom out
* C/DPAD : Move video
* touch the bar : Seek

## Credits
* Core 2 Extreme
* dixy52-beep (icon, banner, in app texture, Italian translation)
* windows-server-2003 (bug fix)
* vargaviktor (Hungarian translation)
* HIDE810 (bug fix)
* LITTOMA (Simplified chinese translation)
* Cookiee (Spanish translation)
* Tescu48 (Romanian translation)
