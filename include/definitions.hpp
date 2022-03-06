#pragma once

//settings
#define DEF_MAIN_DIR (std::string)"/Video_player/"
#define DEF_UPDATE_DIR_PREFIX (std::string)"/3ds/Video_player_ver_"
#define DEF_UPDATE_FILE_PREFIX (std::string)"video_player"
#define DEF_CHECK_INTERNET_URL (std::string)"https://connectivitycheck.gstatic.com/generate_204"
#define DEF_SEND_APP_INFO_URL (std::string)"https://script.google.com/macros/s/AKfycbyn_blFyKWXCgJr6NIF8x6ETs7CHRN5FXKYEAAIrzV6jPYcCkI/exec"
#define DEF_CHECK_UPDATE_URL (std::string)"https://script.google.com/macros/s/AKfycbwTd3jzV0npUE9MNKmZIv3isazVR5D9_7A8rexsG1vr9SE7iavDBxgtzlph8dZipwu9/exec"
#define DEF_HTTP_USER_AGENT (std::string)"video player for 3ds " + DEF_CURRENT_APP_VER
#define DEF_CURRENT_APP_VER (std::string)"v1.5.0-d"
#define DEF_CURRENT_APP_VER_INT 1120
#define DEF_ENABLE_SUB_APP0
//#define DEF_ENABLE_SUB_APP1
//#define DEF_ENABLE_SUB_APP2
//#define DEF_ENABLE_SUB_APP3
//#define DEF_ENABLE_SUB_APP4
//#define DEF_ENABLE_SUB_APP5
//#define DEF_ENABLE_SUB_APP6
//#define DEF_ENABLE_SUB_APP7
#define DEF_DRAW_MAX_NUM_OF_SPRITE_SHEETS 128
#define DEF_DECODER_MAX_CACHE_PACKETS 256
#define DEF_DECODER_MAX_AUDIO_TRACKS 8
#define DEF_DECODER_MAX_VIDEO_TRACKS 2
#define DEF_DECODER_MAX_SUBTITLE_TRACKS 8
#define DEF_DECODER_MAX_RAW_IMAGE 128
#define DEF_DECODER_MAX_SUBTITLE_DATA 16
#define DEF_DECODER_MAX_SESSIONS 2
#define DEF_ENCODER_MAX_SESSIONS 2
#define DEF_EXPL_MAX_FILES 1024
#define DEF_HTTP_POST_BUFFER_SIZE 0x100000
#define DEF_LOG_BUFFER_LINES 512
#define DEF_LOG_COLOR 0xFFBBBB00
#define DEF_MUXER_MAX_SESSIONS 2
#define DEF_SPEAKER_MAX_BUFFERS 192
#define DEF_SWKBD_MAX_DIC_WORDS 128

//video player
#define DEF_VID_NUM_OF_MSG 35
#define DEF_VID_ENABLE_ICON
//#define DEF_VID_ENABLE_NAME
#define DEF_VID_ICON_PATH (std::string)"romfs:/gfx/draw/icon/vid_icon.t3x"
#define DEF_VID_NAME (std::string)"Video\nplayer"
#define DEF_VID_VER (std::string)"v1.5.0-d"
#define DEF_VID_MAIN_STR (std::string)"Vid/Main"
#define DEF_VID_INIT_STR (std::string)"Vid/Init"
#define DEF_VID_EXIT_STR (std::string)"Vid/Exit"
#define DEF_VID_DECODE_THREAD_STR (std::string)"Vid/Decode thread"
#define DEF_VID_DECODE_VIDEO_THREAD_STR (std::string)"Vid/Decode video thread"
#define DEF_VID_CONVERT_THREAD_STR (std::string)"Vid/Convert thread"
#define DEF_VID_READ_PACKET_THREAD_STR (std::string)"Vid/Read packet thread"
#define DEF_VID_TEX_FILTER_MSG 0
#define DEF_VID_CONTROLS_MSG 1
#define DEF_VID_SKIP_FRAME_MSG 2
#define DEF_VID_CONTROL_DESCRIPTION_MSG 3
#define DEF_VID_AUDIO_TRACK_DESCRIPTION_MSG 10
#define DEF_VID_AUDIO_TRACK_MSG 11
#define DEF_VID_FULL_SCREEN_MSG 12
#define DEF_VID_HW_DECODER_MSG 13
#define DEF_VID_HW_CONVERTER_MSG 14
#define DEF_VID_MULTI_THREAD_MSG 15
#define DEF_VID_SKIP_KEY_FRAME_MSG 16
#define DEF_VID_LOWER_RESOLUTION_MSG 17
#define DEF_VID_SEEKING_MSG 18
#define DEF_VID_SEEK_MSG 19
#define DEF_VID_VOLUME_MSG 20
#define DEF_VID_ASPECT_RATIO_MSG 21
#define DEF_VID_MOVE_MODE_MSG 22
#define DEF_VID_REMEMBER_POS_MSG 23
#define DEF_VID_PLAY_METHOD_MSG 24
#define DEF_VID_NO_REPEAT_MSG 25
#define DEF_VID_REPEAT_MSG 26
#define DEF_VID_IN_ORDER_MSG 27
#define DEF_VID_RANDOM_MSG 28
#define DEF_VID_MOVE_MODE_EDIABLE_MSG 29
#define DEF_VID_MOVE_MODE_ENABLE_BOTH_MSG 30
#define DEF_VID_MOVE_MODE_ENABLE_VIDEO_MSG 31
#define DEF_VID_MOVE_MODE_ENABLE_SUBTITLE_MSG 32
#define DEF_VID_SUBTITLE_TRACK_DESCRIPTION_MSG 33
#define DEF_VID_SUBTITLE_TRACK_MSG 34

#define DEF_VID_MENU_NONE -1
#define DEF_VID_MENU_SETTINGS_0 0
#define DEF_VID_MENU_SETTINGS_1 1
#define DEF_VID_MENU_INFO 2

#define DEF_VID_NO_REPEAT 0
#define DEF_VID_REPEAT 1
#define DEF_VID_IN_ORDER 2
#define DEF_VID_RANDOM 3

#define DEF_VID_MOVE_DISABLE 0
#define DEF_VID_MOVE_BOTH 1
#define DEF_VID_MOVE_VIDEO 2
#define DEF_VID_MOVE_SUBTITLE 3

#define DEF_VID_BUFFERS 3

//sample app 1
#define DEF_SAPP1_NUM_OF_MSG 1
//#define DEF_SAPP1_ENABLE_ICON
#define DEF_SAPP1_ENABLE_NAME
#define DEF_SAPP1_ICON_PATH (std::string)"romfs:/"
#define DEF_SAPP1_NAME (std::string)"sample 1"
#define DEF_SAPP1_VER (std::string)"v0.0.1"
#define DEF_SAPP1_MAIN_STR (std::string)"Sapp1/Main"
#define DEF_SAPP1_INIT_STR (std::string)"Sapp1/Init"
#define DEF_SAPP1_EXIT_STR (std::string)"Sapp1/Exit"
#define DEF_SAPP1_WORKER_THREAD_STR (std::string)"Sapp1/Worker thread"

//sample app 2
#define DEF_SAPP2_NUM_OF_MSG 1
//#define DEF_SAPP2_ENABLE_ICON
#define DEF_SAPP2_ENABLE_NAME
#define DEF_SAPP2_ICON_PATH (std::string)"romfs:/"
#define DEF_SAPP2_NAME (std::string)"sample 2"
#define DEF_SAPP2_VER (std::string)"v0.0.1"
#define DEF_SAPP2_MAIN_STR (std::string)"Sapp2/Main"
#define DEF_SAPP2_INIT_STR (std::string)"Sapp2/Init"
#define DEF_SAPP2_EXIT_STR (std::string)"Sapp2/Exit"
#define DEF_SAPP2_WORKER_THREAD_STR (std::string)"Sapp2/Worker thread"

//sample app 3
#define DEF_SAPP3_NUM_OF_MSG 1
//#define DEF_SAPP3_ENABLE_ICON
#define DEF_SAPP3_ENABLE_NAME
#define DEF_SAPP3_ICON_PATH (std::string)"romfs:/"
#define DEF_SAPP3_NAME (std::string)"sample 3"
#define DEF_SAPP3_VER (std::string)"v0.0.1"
#define DEF_SAPP3_MAIN_STR (std::string)"Sapp3/Main"
#define DEF_SAPP3_INIT_STR (std::string)"Sapp3/Init"
#define DEF_SAPP3_EXIT_STR (std::string)"Sapp3/Exit"
#define DEF_SAPP3_WORKER_THREAD_STR (std::string)"Sapp3/Worker thread"

//sample app 4
#define DEF_SAPP4_NUM_OF_MSG 1
//#define DEF_SAPP4_ENABLE_ICON
#define DEF_SAPP4_ENABLE_NAME
#define DEF_SAPP4_ICON_PATH (std::string)"romfs:/"
#define DEF_SAPP4_NAME (std::string)"sample 4"
#define DEF_SAPP4_VER (std::string)"v0.0.1"
#define DEF_SAPP4_MAIN_STR (std::string)"Sapp4/Main"
#define DEF_SAPP4_INIT_STR (std::string)"Sapp4/Init"
#define DEF_SAPP4_EXIT_STR (std::string)"Sapp4/Exit"
#define DEF_SAPP4_WORKER_THREAD_STR (std::string)"Sapp4/Worker thread"

//sample app 5
#define DEF_SAPP5_NUM_OF_MSG 1
//#define DEF_SAPP5_ENABLE_ICON
#define DEF_SAPP5_ENABLE_NAME
#define DEF_SAPP5_ICON_PATH (std::string)"romfs:/"
#define DEF_SAPP5_NAME (std::string)"sample 5"
#define DEF_SAPP5_VER (std::string)"v0.0.1"
#define DEF_SAPP5_MAIN_STR (std::string)"Sapp5/Main"
#define DEF_SAPP5_INIT_STR (std::string)"Sapp5/Init"
#define DEF_SAPP5_EXIT_STR (std::string)"Sapp5/Exit"
#define DEF_SAPP5_WORKER_THREAD_STR (std::string)"Sapp5/Worker thread"

//sample app 6
#define DEF_SAPP6_NUM_OF_MSG 1
//#define DEF_SAPP6_ENABLE_ICON
#define DEF_SAPP6_ENABLE_NAME
#define DEF_SAPP6_ICON_PATH (std::string)"romfs:/"
#define DEF_SAPP6_NAME (std::string)"sample 6"
#define DEF_SAPP6_VER (std::string)"v0.0.1"
#define DEF_SAPP6_MAIN_STR (std::string)"Sapp6/Main"
#define DEF_SAPP6_INIT_STR (std::string)"Sapp6/Init"
#define DEF_SAPP6_EXIT_STR (std::string)"Sapp6/Exit"
#define DEF_SAPP6_WORKER_THREAD_STR (std::string)"Sapp6/Worker thread"

//sample app 7
#define DEF_SAPP7_NUM_OF_MSG 1
//#define DEF_SAPP7_ENABLE_ICON
#define DEF_SAPP7_ENABLE_NAME
#define DEF_SAPP7_ICON_PATH (std::string)"romfs:/"
#define DEF_SAPP7_NAME (std::string)"sample 7"
#define DEF_SAPP7_VER (std::string)"v0.0.1"
#define DEF_SAPP7_MAIN_STR (std::string)"Sapp7/Main"
#define DEF_SAPP7_INIT_STR (std::string)"Sapp7/Init"
#define DEF_SAPP7_EXIT_STR (std::string)"Sapp7/Exit"
#define DEF_SAPP7_WORKER_THREAD_STR (std::string)"Sapp7/Worker thread"
