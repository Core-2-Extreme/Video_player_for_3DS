# Video player for 3DS

![GitHub all releases](https://img.shields.io/github/downloads/Core-2-Extreme/Video_player_for_3DS/total?color=purple&style=flat-square)
![GitHub commits since latest release (by SemVer)](https://img.shields.io/github/commits-since/Core-2-Extreme/Video_player_for_3DS/latest?color=orange&style=flat-square)
![GitHub commit activity](https://img.shields.io/github/commit-activity/m/Core-2-Extreme/Video_player_for_3DS?color=darkgreen&style=flat-square)

## Index
* [Screenshots](https://github.com/Core-2-Extreme/Video_player_for_3DS#screenshots)
* [Features](https://github.com/Core-2-Extreme/Video_player_for_3DS#features)
* [Prepare videos](https://github.com/Core-2-Extreme/Video_player_for_3DS#prepare-videos)
* [Prepare videos (advanced users)](https://github.com/Core-2-Extreme/Video_player_for_3DS#prepare-videos-advanced-users)
* [Controls](https://github.com/Core-2-Extreme/Video_player_for_3DS#controls)
* [Supported languages](https://github.com/Core-2-Extreme/Video_player_for_3DS#supported-languages)
* [Supported codecs](https://github.com/Core-2-Extreme/Video_player_for_3DS#supported-codecs)
* [Supported containers](https://github.com/Core-2-Extreme/Video_player_for_3DS#supported-containers-extensions)
* [Links](https://github.com/Core-2-Extreme/Video_player_for_3DS#links)
* [Build](https://github.com/Core-2-Extreme/Video_player_for_3DS#build)
* [Recommended resolution](https://github.com/Core-2-Extreme/Video_player_for_3DS#recommended-resolution)
* [Benchmark](https://github.com/Core-2-Extreme/Video_player_for_3DS#benchmark)
* [Troubleshoot/FAQ](https://github.com/Core-2-Extreme/Video_player_for_3DS#troubleshootfaq)
* [Patch note](https://github.com/Core-2-Extreme/Video_player_for_3DS#patch-note)
* [License](https://github.com/Core-2-Extreme/Video_player_for_3DS#license)
* [Credits](https://github.com/Core-2-Extreme/Video_player_for_3DS#credits)
* [Donation](https://github.com/Core-2-Extreme/Video_player_for_3DS#donation)

## Screenshots
**With multiple audio tracks support** \
<img src="https://raw.githubusercontent.com/Core-2-Extreme/Video_player_for_3DS/main/screenshots/multiple_audio_tracks.png" width="400" height="480"> \
**With subtitles** \
<img src="https://raw.githubusercontent.com/Core-2-Extreme/Video_player_for_3DS/main/screenshots/subtitles.png" width="400" height="480"> \
**With night mode** \
<img src="https://raw.githubusercontent.com/Core-2-Extreme/Video_player_for_3DS/main/screenshots/night_mode.png" width="400" height="480"> \
**And many more...**

## Features
Hardware accelerated decoding (*0) ✅ \
Hardware accelerated color conversion ✅ \
Multiple video codec support ✅ \
Multiple audio codec support ✅ \
Multiple subtitle codec support ✅ \
Seek ✅ \
3D videos (*1)✅ \
Zoom in/out video ✅ \
Move video ✅ \
File explorer ✅ \
Full-screen mode ✅

⚠️ *0 New 3DS and New 2DS only \
⚠️ *1 New 3DS and 3DS only, software decoder only, in order to see 3D video as 3D \
you need to enable 3D mode in settings(settings->LCD->Screen mode->3D)

## Prepare videos
If you are advanced user, see [Recommended resolution](https://github.com/Core-2-Extreme/Video_player_for_3DS#Recommended-resolution) and [Benchmark](https://github.com/Core-2-Extreme/Video_player_for_3DS#Benchmark) for performance then use commandline in the [Prepare videos (advanced users)](https://github.com/Core-2-Extreme/Video_player_for_3DS#prepare-videos-advanced-users).

If not, you can use these tools (**these tools are third-party tools, therefore support for these tools are provided by their developers**):
* [Nintendo Video Convertor (by T0biasCZe)](https://gbatemp.net/threads/nintendo-video-convertor-video-convertor-for-3ds-and-wii.622972/)
* [VideoPlayer3DS DS Assistant (by JustScratchCoder)](https://github.com/JustScratchCoder/VideoPlayer3DS-DS-Assistant)

## Prepare videos (advanced users)
*If you are not familiar with CUI tools, you should use third-party tools above or consult friends, family, DeepSeek, ChatGPT and/or DuckDuckGo for how to use ffmpeg.*
1. Prepare your original videos.
2. Download [ffmpeg](https://ffmpeg.org/download.html) if you don't have it.
  * If you are looking forward to making non-3D videos...
    * On NEW3DS/NEW2DS...
      * Use `ffmpeg -i {input_file_name} -c:a {audio_codec} -c:v {video_codec} -s {resolution} {q} {output_file_name}`
    * On OLD3DS/OLD2DS...
      * Use `ffmpeg -i {input_file_name} -c:a {audio_codec} -c:v {video_codec} -s {resolution} {q} {output_file_name}`
  * If you are looking forward to making 3D videos...
    * That is SBS (side-by-side) (**recommended**)...
      * On NEW3DS/NEW2DS...
        * Use `ffmpeg -i {input_file_name} -c:a {audio_codec} -c:v {video_codec} -s {resolution} {q} -metadata:s:v:0 stereo_mode={sbs_type} {output_file_name}`
      * On OLD3DS/OLD2DS...
        * Use `ffmpeg -i {input_file_name} -c:a {audio_codec} -c:v {video_codec} -s {resolution} {q} -metadata:s:v:0 stereo_mode={sbs_type} {output_file_name}`
    * That is non-SBS...
      * Referer this: [How to convert your 3D video for 3DS (by T0biasCZe)](https://gbatemp.net/threads/release-video-player-for-3ds.586094/page-10#post-9575227)
3. Copy generated videos to your 3DS (anywhere on your SD card).

Where:
* `{input_file_name}` is your original video (e.g. `my_movie.mp4`, `camera_1990_01_01.avi` etc..).
* `{output_file_name}` is a converted file that is ready to play on your 3DS (e.g. `my_movie_converted.mkv`, `for_3ds.mp4` etc..).
  * For most cases, `.mkv` is recommended because it uses less RAM when playing it (hence less chance of out of memory error).
* `{audio_codec}` is an audio format (e.g. `aac`, `mp3`, `libvorbis`, `libopus` etc..).
  * If you don't know what it is, use `aac` for `.mp4` and `libvorbis` for `.mkv`.
* `{video_codec}` is a video format (e.g. `mpeg2video`, `mpeg4`, `libx264`, `libx265` etc..).
  * For **OLD**3DS/2DS, `mpeg4` is recommended.
  * For **NEW**3DS/2DS, `libx264` is recommended.
  * If you don't know what it is, use the recommended value above.
* `{resolution}` is a resolution of video frames.
  * For **OLD**3DS/2DS, `400x240` is recommended.
  * For **NEW**3DS/2DS, `800x240` or `800x480` is recommended.
  * If you don't know what it is, use the recommended value above.
* `{q}` is a quality of your video, lower value gives higher quality (and usually larger file size).
  * Use `-crf {number}` for `libx264` and `libx265`.
    * Something around `25` is a good point to start.
      * Adjust it according to your preference.
  * Use `-q:v {number}` for `mpeg2video` and `mpeg4`.
    * Something around `8` is a good point to start.
      * Adjust it according to your preference.
* `{sbs_type}` is a stereoscopic metadata (used to know how images are aligned).
  * Note: You should use `.mkv` for `{output_file_name}` because some containers such as `.mp4` doesn't keep this metadata.
    * If your SBS video has left:right alignment (most common) (that is, **left picture** is for left eye, **right picture** is for **right eye**), use `left_right`.
    * If your SBS video has right:left alignment (that is, **left picture** is for **right eye**, **right picture** is for **left eye**), use `left_right`.
    * If your SBS video has top:bottom alignment (that is, **top picture** is for **left eye**, **bottom picture** is for **right eye**), use `top_bottom`.
    * If your SBS video has bottom:top alignment (that is, **top picture** is for **right eye**, **bottom picture** is for **left eye**), use `bottom_top`.

E.g.:
* "I want to convert my video that is called "GNU.mp4" for my **OLD**3DS."
  * `ffmpeg -i "GNU.mp4" -c:a libvorbis -c:v mpeg4 -s 400x240 -q:v 8 GNU_for_3DS.mkv`
* "I want to convert my video that is called "GNU.mkv" for my **NEW**3DS."
  * `ffmpeg -i "GNU.mkv" -c:a libvorbis -c:v libx264 -s 800x240 -crf 25 GNU_for_3DS.mkv`
* "I want to convert my video that is called "GNU.mkv" for my **NEW**3DS, **I don't care file size** and want to make it as **high quality** as possible."
  * `ffmpeg -i "GNU.mkv" -c:a libvorbis -c:v libx264 -s 800x480 -crf 0 GNU_for_3DS.mkv`
* "I want to convert my **left:right SBS** video that is called "GNU_in_SBS.mp4" for my **OLD**3DS."
  * `ffmpeg -i "GNU_in_SBS.mp4" -c:a libvorbis -c:v mpeg4 -s 400x240 -q:v 8 -metadata:s:v:0 stereo_mode=left_right GNU_in_SBS_for_3DS.mkv`
* "I want to convert my **bottom:top SBS** video that is called "GNU_in_SBS.mp4" for my **OLD**3DS, **I don't care video quality** just make it **smaller**."
  * `ffmpeg -i "GNU_in_SBS.mp4" -c:a libvorbis -c:v mpeg4 -s 400x240 -q:v 40 -metadata:s:v:0 stereo_mode=bottom_top GNU_in_SBS_for_3DS.mkv`
* "I want to convert my **left:right SBS** video that is called "GNU_in_SBS.mp4" for my **NEW**3DS."
  * `ffmpeg -i "GNU_in_SBS.mp4" -c:a libvorbis -c:v libx264 -s 800x240 -crf 25 -metadata:s:v:0 stereo_mode=left_right GNU_in_SBS_for_3DS.mkv`

Prp tip: If you find you frequently see "Processing video..." message during playback, try lowering `{resolution}` and/or \
framerate by adding `-r {fps}` (e.g. `ffmpeg -i "GNU.mp4" -c:a libvorbis -c:v mpeg4 -s 256x144 -q:v 8 -r 23.976 GNU_for_3DS.mkv` for 256x144@23.976fps).

## Controls
* In normal mode
  * A: Play/Pause
  * B: Stop
  * Y: Open settings menu
  * X: Select a file
  * R: Zoom in
  * L: Zoom out
  * CPAD: Move a video and/or subtitle
  * DPAD ←→: Seek
  * DPAD ↑↓: Change screen brightness
  * START: Back to main menu
  * SELECT: Enter full-screen mode
  * Touch on the bar: Seek
* In full-screen mode
  * A: Play/Pause
  * DPAD ←→: Seek
  * DPAD ↑↓: Change screen brightness
  * SELECT: Exit full-screen mode

## Supported languages
* English
* Japanese/日本語
* Hungarian/Magyar (translated by vargaviktor)
* Simplified Chinese/简体中文 (translated by LITTOMA)
* Italian/Italiano (translated by dixy52-beep)
* Spanish/Español (translated by Cookiee)
* Romanian/Română (translated by Tescu48)
* Polish/Polski (translated by JustScratchCoder)
* Ryukyuan/琉球諸語 (translated by kuragehimekurara1)

## Supported codecs
**Supported video codecs**
* AV1
* H261
* H262
* H263
* H263+
* H264 (AVC, MPEG4 part10)
* H265 (HEVC)
* Motion jpeg
* MPEG1video
* MPEG2video
* MPEG4 (MPEG4 part2)
* Theora
* VP9

**Supported audio codecs**
* AAC (Advanced audio coding)
* AC3
* FLAC
* MP1 (MPEG audio layer 1)
* MP2 (MPEG audio layer 2)
* MP3 (MPEG audio layer 3)
* Ogg (Vorbis)
* Opus
* PCM audio

**Supported subtitle codecs**
* (No style support)
  * Movtext
  * Subrip
  * Subviewer
* DVDSub (VobSub)

## Supported containers (extensions)
* .aac
* .ac3
* .avi
* .m4a
* .m4v
* .mkv
* .mp1
* .mp2
* .mp3
* .mp4
* .mov
* .ogg
* .wav
* .webm

## Links
[Discord channel](https://discord.gg/MMsAXvetpR) \
[GBAtemp thread](https://gbatemp.net/threads/release-video-player-for-3ds.586094)

## Build
You need:
* [devkitpro](https://devkitpro.org/wiki/Getting_Started) ([install guide](library/_devkitpro_install.md))

If you want to build .cia, then you also need:
* [bannertool](https://github.com/diasurgical/bannertool/releases/tag/1.2.0)
  * Broken links? Try our [backup](https://github.com/Core-2-Extreme/bannertool_fork/releases/tag/1.2.0).
* [makerom](https://github.com/3DSGuy/Project_CTR/releases/tag/makerom-v0.18.4)
  * Broken links? Try our [backup](https://github.com/Core-2-Extreme/Project_CTR_fork/releases/tag/makerom-v0.18.4).

(Copy them in your path e.g. in `{devkitPro_install_dir}\tools\bin`).

As of this writing, we use these packages to build this project. \
Note: Not all of them are necessary e.g. `devkitARM-gdb` is not required to build and we don't use \
standard `libctru` and `citro2(3)d` (that means you don't need them to build this project), \
but we just document all of them in case someone need these information. \
For more information, see [here](library/_devkitpro_install.md#install-devkitpro).
```
$ dkp-pacman -Q
3ds-cmake 1.5.1-1
3ds-examples 20240917-1
3ds-pkg-config 0.28-5
3dslink 0.6.3-1
3dstools 1.3.1-3
catnip 0.1.0-1
citro2d 1.6.0-1
citro3d 1.7.1-2
devkit-env 1.0.1-2
devkitARM r65-1
devkitARM-gdb 14.1-2
devkitarm-cmake 1.2.2-1
devkitarm-crtls 1.2.6-1
devkitarm-rules 1.5.1-1
devkitpro-keyring 20241017-2
dkp-cmake-common-utils 1.5.2-1
general-tools 1.4.4-1
libctru 2.4.1-1
pacman 6.0.1-7
picasso 2.7.2-3
tex3ds 2.3.0-4
```

For .cia build
```
bannertool 1.2.0
makerom v0.18.4
```

If you want to make changes to the libraries, then follow [this guide](library/_devkitpro_install.md#build-libraries-usually-optional).

After having all dependencies, do:
* Clone this repository (`git clone https://github.com/core-2-extreme/{project_name}`).
  * On windows run `build_3dsx.bat` for `.3dsx` only build or `build.bat` for `.3dsx`+`.cia` build.
  * On other system, run `make 3dsx` for `.3dsx` only build or `make all` for `.3dsx`+`.cia` build.

## Recommended resolution
Videos that in this resolution will be played without any problems in most of the situations.
|  Recommended resolution  | mpeg1video | mpeg2video |    H263+   |    H264    |    H265    |
| ------------------------ | ---------- | ---------- | ---------- | ---------- | ---------- |
| OLD3DS Software decoding | 400x240@30 | 400x240@30 | 400x240@24 | 256x144@24 | 256x144@10 |
| NEW3DS Software decoding | 800x240@30 | 800x240@30 | 800x240@30 | 800x240@30 | 800x240@20 |
| NEW3DS Hardware decoding |    none    |    none    |    none    | 800x240@60 |    none    |


Videos that in this resolution may be played without problems if scene changes, camera panning etc.. are infrequent.
|    Maximum resolution    | mpeg1video | mpeg2video |    H263+   |    H264    |    H265    |
| ------------------------ | ---------- | ---------- | ---------- | ---------- | ---------- |
| OLD3DS Software decoding | 800x240@24 | 800x240@24 | 800x240@20 | 400x240@15 | 256x144@20 |
| NEW3DS Software decoding | 800x480@30 | 800x480@30 | 800x480@30 | 800x480@24 | 800x240@30 |
| NEW3DS Hardware decoding |    none    |    none    |    none    |1280x720@20 |    none    |

**Note: Video resolution that exceeds screen resolution (400x240 on OLD2DS, 800x240 on OLD3DS, NEW2DS and NEW3DS) has little visible effect.**

## Benchmark
⚠️ Decoding speed depends on encoder option, video type, video scene, etc...

Original video file: [Big Buck Bunny](https://www.youtube.com/watch?v=YE7VzlLtp-4) \
The test videos were generated with following commands: \
mjpeg: `ffmpeg -i {input_file} -acodec copy -vcodec mjpeg -s {width}x{height} -q:v 5 -t 03:00 {output_file}` \
mpeg1video: `ffmpeg -i {input_file} -acodec copy -vcodec mpeg1video -s {width}x{height} -q:v 5 -t 03:00 {output_file}` \
mpeg2video: `ffmpeg -i {input_file} -acodec copy -vcodec mpeg2video -s {width}x{height} -q:v 5 -t 03:00 {output_file}` \
mpeg4: `ffmpeg -i {input_file} -acodec copy -vcodec mpeg4 -s {width}x{height} -q:v 5 -t 03:00 {output_file}` \
h263p: `ffmpeg -i {input_file} -acodec copy -vcodec h263p -s {width}x{height} -q:v 5 -t 03:00 {output_file}` \
h264: `ffmpeg -i {input_file} -acodec copy -vcodec libx264 -s {width}x{height} -crf 25 -t 03:00 {output_file}` \
h265: `ffmpeg -i {input_file} -acodec copy -vcodec libx265 -s {width}x{height} -crf 30 -t 03:00 {output_file}` \
av1: `ffmpeg -i {input_file} -acodec copy -vcodec libsvtav1 -s {width}x{height} -crf 40 -row-mt 1 -cpu-used 5 -t 03:00 {output_file}`

NEW3DS:
![new3ds_decoding_speed](https://user-images.githubusercontent.com/45873899/221850778-c58cb243-854c-499f-9084-4646bd8c9de9.png)

OLD3DS:
![old3ds_decoding_speed](https://user-images.githubusercontent.com/45873899/221850879-c96f4764-b608-45ee-aa80-da36234ee92e.png)

## Troubleshoot/FAQ
1. Q. Can I watch SBS (side-by-side) 3D videos?????
    * A. **Yes** ✅, make sure your video has [stereoscopic metadata](https://github.com/Core-2-Extreme/Video_player_for_3DS#prepare-videos-advanced-users)!!!!!
2. Q. I've changed volume in app settings but it doesn't apply!!!!! Why?????
    * A. For now, manual volume changes in app settings **takes some time to apply** ⚠️ due to implementation!!!!!
3. Q. Can I use variable-framerate videos?????
    * A. **No** ❌, only constant-framerate videos are supported!!!!!
4. Q. Can I change playback speed?????
    * A. **No** ❌, playback speed is fixed to x1!!!!!
5. Q. Can I stream videos from {Your favorite streaming service}?????
    * A. **No** ❌, streaming from Internet (including both WAN and LAN) is NOT supported (only local storage (i.e. SD card) is supported)!!!!!
6. Q. Where do I put my videos?????
    * A. You can put your videos on **anyware on your SD card** ✅!!!!!
7. Q. How do I play videos?????
    * A. **Open video player for 3DS, click on <img src="https://raw.githubusercontent.com/Core-2-Extreme/Video_player_for_3DS/main/screenshots/video_player_icon.png" width="50" height="50">, then press X to open file explorer, finally select your video to play it** ✅!!!!!
8. Q. Is there bitrate limit?????
    * A. **Yes** and **no** ⚠️, video player itself doesn't have any rate limit, however if 3DS's SD card reader can NOT keep up, you'll see "processing video" messages (in this case, lower bitrate)!!!!!
9. Q. Is there length limit?????
    * A. **Yes** and **no** ⚠️, video player itself doesn't have any length limit, however if 3DS runs out of memory, you'll see "out of (linear) memory" error (in this case shorten video length and/or use .mkv)!!!!!
10. Q. Can I add/update language translation?????
    * A. **Yes** ✅, you can just create/update language files on `romfs/gfx/msg/` and make a pull request on GitHub or submit it on our [Discord server](https://discord.gg/MMsAXvetpR)!!!!!
11. Q. I found bugs, can I report it?????
    * A. **Yes** ✅, you can report an issue on GitHub or on our [Discord server](https://discord.gg/MMsAXvetpR), please provide as much details as possible!!!!!
12. Q. My pull/feature request got rejected, can I fork it and make a custom version?????
    * A. **Yes** ✅, this software is licensed under GPLv3 so you can freely use, modify and distribute!!!!!
13. Q. Can I ask what is <img src="https://github.com/user-attachments/assets/2ad7bede-0660-4154-a9b8-d80fbf495006" width="50" height="50">?????
    * A. **Yes** ✅, you should join our [Discord server](https://discord.gg/MMsAXvetpR) for the answer; longer you are in our [Discord server](https://discord.gg/MMsAXvetpR), more you know what <img src="https://github.com/user-attachments/assets/2ad7bede-0660-4154-a9b8-d80fbf495006" width="50" height="50"> is!!!!!

## Patch note
* [v1.7.0](https://github.com/Core-2-Extreme/Video_player_for_3DS#v170)
* [v1.6.1](https://github.com/Core-2-Extreme/Video_player_for_3DS#v161)
* [v1.6.0](https://github.com/Core-2-Extreme/Video_player_for_3DS#v160)
* [v1.5.3](https://github.com/Core-2-Extreme/Video_player_for_3DS#v153)
* [v1.5.2](https://github.com/Core-2-Extreme/Video_player_for_3DS#v152)
* [v1.5.1](https://github.com/Core-2-Extreme/Video_player_for_3DS#v151)
* [v1.5.0](https://github.com/Core-2-Extreme/Video_player_for_3DS#v150)
* [v1.4.2](https://github.com/Core-2-Extreme/Video_player_for_3DS#v142)
* [v1.4.1](https://github.com/Core-2-Extreme/Video_player_for_3DS#v141)
* [v1.4.0](https://github.com/Core-2-Extreme/Video_player_for_3DS#v140)
* [v1.3.3](https://github.com/Core-2-Extreme/Video_player_for_3DS#v133)
* [v1.3.2](https://github.com/Core-2-Extreme/Video_player_for_3DS#v132)
* [v1.3.1](https://github.com/Core-2-Extreme/Video_player_for_3DS#v131)
* [v1.3.0](https://github.com/Core-2-Extreme/Video_player_for_3DS#v130)
* [v1.2.0](https://github.com/Core-2-Extreme/Video_player_for_3DS#v120)
* [v1.1.1](https://github.com/Core-2-Extreme/Video_player_for_3DS#v111)
* [v1.1.0](https://github.com/Core-2-Extreme/Video_player_for_3DS#v110)
* [v1.0.1](https://github.com/Core-2-Extreme/Video_player_for_3DS#v101)
* [v1.0.0](https://github.com/Core-2-Extreme/Video_player_for_3DS#v100)

### v1.7.0
**Changes** \
Controls is now always displayed on bottom screen unless settings is opened. \
File explorer close button has been changed from Y to X. \
Move contents mode is now disabled by default. \
Support for SBS (side-by-side) videos have been added. \
Improved OOM recovery on HW decoder and dav1d decoder. \
Ported performance improvement from a fork project.

**Fixed bugs** \
Decoding stall (causing frequent "processing video") on media files that contain unsupported codec has been fixed. \
Initial video position has been fixed (it was slightly offset right depending on videos).

### v1.6.1
**Changes** \
Number of decoding threads are selectable now. \
Full-screen transition period has been changed from 5 seconds to 3 seconds. \
Progress message (`xx.yy%`) has been added to `seeking` and `processing video` messages. \
3D slider bar no longer moves video.

**Fixed bugs** \
Race condition that may cause model detection to fail and disable use of HW decoder has been fixed. \
Seek issue on audio only file and 3D videos have been fixed. \
Many issues on code that may cause problems on 3D videos have been fixed. \
Unable to play 3D videos if HW decoder is enabled has been fixed (by disabling HW decoder on 3D videos).

### v1.6.0
**Changes** \
VP9 and flac have been supported ([#64](https://github.com/Core-2-Extreme/Video_player_for_3DS/issues/64)). \
DVDSub (VobSub) has been supported ([#58](https://github.com/Core-2-Extreme/Video_player_for_3DS/issues/58)). \
AV desync management has been re-implemented. \
New sound banner has been added for `.cia` ([#82](https://github.com/Core-2-Extreme/Video_player_for_3DS/issues/82)) (by UrbanVersis32). \
Theora has been supported ([#83](https://github.com/Core-2-Extreme/Video_player_for_3DS/issues/83)) (by RetroGamer02). \
German translation has been added ([#85](https://github.com/Core-2-Extreme/Video_player_for_3DS/issues/85)) (by 0n1cOn3). \
High RAM build (only for `.cia`) has been added for release (see release page for detail).

**Fixed bugs** \
Many minor bugs that may cause crash have been fixed. \
Random crash on hardware decorder has been fixed ([#69](https://github.com/Core-2-Extreme/Video_player_for_3DS/issues/69)). \
Auto-playing video after coming back from Nintendo's home menu and keyboard even it was previously paused has been fixed. \
AV desync on more than 2ch audio has been fixed.

### v1.5.3
**Changes** \
Audio files more than 2ch (e.g. 2.1ch) has been supported. \
Many pixel formats have been supported, however, YUV420P is recommended for performance reason. \
Ryukyuan(琉球諸語) translation has been added (by kuragehimekurara1). \
Auto mode for top screen mode has been added, when this is enabled (settings -> LCD -> screen mode -> auto) you can just use 3D slider to change between 3D <-> 800px mode. \
Simplified Chinese(简体中文) translation has been updated.

**Fixed bugs** \
Many problems with seek function including backward seeking has been fixed. \
Problem that it won't enter sleep mode in some case has been fixed. \
Problem that h263p video is not played correctly in some resolution has been fixed.

### v1.5.2
**Changes** \
Ignore unsupported codec so that you can play supported codec only. \
(e.g. You can now play videos that contain unsupported subtitles/audio)

**Fixed bugs** \
Hardware decoder won't play videos that contain B-frames smoothly has been fixed. \
(It means you don't have to care about B-frames when encoding to H.264 videos)

### v1.5.1
**Changes** \
Screen update frequency in audio only files have been increased. \
Performance has been improved by adding DMA. \
Video playback is automatically paused when runs out of buffer. \
(You can change threshold by changing 'Restart playback threshold' settings) \
Seeking speed has been improved. \
Disallow sleep when only headset is connected. \
(allow sleep if headset is disconnected during playback) \
App directory has been changed from "sdmc:/Video_player/" to "sdmc:/3ds/Video_player/". \
(App will automatically move folder) \
Simplified Chinese(简体中文) translation has been updated. \
Enabled 'correct aspect ratio option' by default.

**Fixed bugs** \
Crashes in hw decoder in some videos have been fixed. \
Automatically enter full-screen mode even file explorer is opened has been fixed. \
App will freeze if you seek after EOF has been fixed. \
Unable to pause/resume in tagged mp3 has been fixed. \
Video position won't be saved in some cases have been fixed. \
Glitch on video in full-screen mode has been fixed. \
Some directories can't be entered have been fixed. \
Some videos won't be played at correct speed has been fixed. \
Frame desync in some 3D videos have been fixed.


### v1.5.0
Subtitles have been supported. (*0) \
Disable video, audio and subtitle have been added. \
Audio desync has been fixed. \
AV1 videos have been supported. \
The problem some video won't play smoothly in sw decoder has been fixed. \
Screen brightness adjustment function has been added (DPAD "↑" and "↓"). \
*0 No style support, only plain text \
Other minor changes.

### v1.4.2
Unexpected touch in Nintendo's home menu has been fixed \
Multi-threaded decoding stability has been fixed and it is enabled by default \
Seeking stability has been fixed \
Repeat, in order, random playback mode have been added \
Other minor changes.

### v1.4.1
The "FSUSER_OpenFile() failed" error has been fixed. \
Adjusted font and button size in settings menu. \
Multi-threaded decoding is disabled by default because it has stability problem. \
Polish(Polski) translation has been added (by JustScratchCoder). \
Other minor changes.

### v1.4.0
Decoded image(raw) buffer has been added and it makes playback. \
much much much much better (especially on OLD3DS)
Spanish(español) translation has been added (by Cookiee). \
Romanian(Română) translation has been added (by Tescu48). \
Other minor changes.

### v1.3.3
'aspect ratio 10:3 mode' has been changed to 'correct aspect ratio mode' (follow sar value(*0)). \
Color conversion speed has been improved. \
Simplified Chinese(简体中文) translation has been added (by LITTOMA). \
Italian(italiano) translation has been added (by dixy52-beep). \
*0 if video size is 800x240 and no sar value is set, it autmatically apply sar 1:2. \
Other minor changes.

### v1.3.2
Added aspect ratio 10:3 mode (for 800x240 videos). \
Added disable resize and move video mode. \
Added remember video pos mode (resume from that pos next time). \
Other minor changes.

### v1.3.1
Volume adjustment has been added (from 0% to 999%). \
Direction pad seeking has been added (from 1 second to 99 seconds). \
Hungarian(magyar) translation has been added (by vargaviktor). \
Other minor changes.

### v1.3.0
The video that has more than one audio tracks has been supported (Press Y key-> select audio track to select track). \
Multi-threaded decoding has been supported (Press Y key-> use multi-threaded decoding to toggle). \
Hardware decoder has been merged (Press Y key-> use hw decoder to toggle). \
Full-screen mode has been supported (Press select key to toggle). \
Other minor changes.

### v1.2.0
Hardware decoding won't work in .cia has been fixed. \
Hardware decoding won't work in some resolution has been fixed. \
Added hardware color conversion for software decoder. \
Added 3D video support(software decoder only). \
Other minor changes and optimization.

### v1.1.1
Video will not be decoded correctly in hardware decoder has been fixed.

### v1.1.0
Added hardware decoder (β).

### v1.0.1
Added allow skip frames option.

### v1.0.0
Initial release.

## License
This software is licensed under GNU General Public License v3.0 or later.

Third party libraries are licensed under:

| Library                                                                     | License                                |
| --------------------------------------------------------------------------- | -------------------------------------- |
| [libctru](https://github.com/devkitPro/libctru#license)                     | zlib License                           |
| [citro3d](https://github.com/devkitPro/citro3d/blob/master/LICENSE)         | zlib License                           |
| [citro2d](https://github.com/devkitPro/citro2d/blob/master/LICENSE)         | zlib License                           |
| [x264](https://github.com/mirror/x264/blob/master/COPYING)                  | GNU General Public License v2.0        |
| [libmp3lame](https://github.com/gypified/libmp3lame/blob/master/COPYING)    | GNU Lesser General Public License v2.0 |
| [dav1d](https://github.com/videolan/dav1d/blob/master/COPYING)              | BSD 2-Clause                           |
| [ffmpeg](https://github.com/FFmpeg/FFmpeg/blob/master/COPYING.GPLv3)        | GNU General Public License v3.0        |
| [zlib](https://github.com/madler/zlib/blob/master/LICENSE)                  | zlib License                           |
| [mbedtls](https://github.com/Mbed-TLS/mbedtls/blob/development/LICENSE)     | Apache License 2.0                     |
| [nghttp2](https://github.com/nghttp2/nghttp2/blob/master/COPYING)           | MIT License                            |
| [curl](https://github.com/curl/curl/blob/master/COPYING)                    | The curl license                       |
| [stb_image](https://github.com/nothings/stb/blob/master/LICENSE)            | Public Domain                          |

## Credits
* Core 2 Extreme
* dixy52-beep (icon, banner, in app texture, Italian translation)
* windows-server-2003 (bug fix)
* vargaviktor (Hungarian translation)
* HIDE810 (bug fix)
* LITTOMA (Simplified chinese translation)
* Cookiee (Spanish translation)
* Tescu48 (Romanian translation)
* JustScratchCoder (Polish translation)
* T0biasCZe (Nintendo Video Convertor)
* kuragehimekurara1 (Ryukyuan translation)
* UrbanVersis32 (banner sound)
* RetroGamer02 (Theora support)
* 0n1cOn3 (German translation)

## Donation
If you find my app helpful, buy me a cup of coffee.
* BTC: bc1qch33qdce5hwxte0pm8pn0a6qqnartg2ujklhhc
* LTC: MKTD3U2vCMi7S7Jb1EQ2FiS4AdHC23PxJh
