#pragma once

//settings
#define DEF_MAIN_DIR (std::string)"/Video_player/"
#define DEF_UPDATE_DIR_PREFIX (std::string)"/3ds/Video_player_ver_"
#define DEF_UPDATE_FILE_PREFIX (std::string)"video_player"
#define DEF_LOG_COLOR 0xFFBBBB00
#define DEF_SWKBD_MAX_DIC_WORDS 128
#define DEF_CHECK_INTERNET_URL (std::string)"https://connectivitycheck.gstatic.com/generate_204"
#define DEF_SEND_APP_INFO_URL (std::string)"https://script.google.com/macros/s/AKfycbyn_blFyKWXCgJr6NIF8x6ETs7CHRN5FXKYEAAIrzV6jPYcCkI/exec"
#define DEF_CHECK_UPDATE_URL (std::string)"https://script.google.com/macros/s/AKfycbwTd3jzV0npUE9MNKmZIv3isazVR5D9_7A8rexsG1vr9SE7iavDBxgtzlph8dZipwu9/exec"
#define DEF_HTTP_USER_AGENT (std::string)"video player for 3ds v1.2.0"
#define DEF_CURRENT_APP_VER (std::string)"v1.2.0"
#define DEF_CURRENT_APP_VER_INT 1056
#define DEF_ENABLE_SUB_APP0
#define DEF_ENABLE_SUB_APP1
//#define DEF_ENABLE_SUB_APP2
//#define DEF_ENABLE_SUB_APP3
//#define DEF_ENABLE_SUB_APP4
//#define DEF_ENABLE_SUB_APP5
//#define DEF_ENABLE_SUB_APP6
//#define DEF_ENABLE_SUB_APP7

//video player
#define DEF_SAPP0_NUM_OF_MSG 14
#define DEF_SAPP0_ENABLE_ICON
//#define DEF_SAPP0_ENABLE_NAME
#define DEF_SAPP0_ICON_PATH (std::string)"romfs:/gfx/draw/icon/vid_icon.t3x"
#define DEF_SAPP0_NAME (std::string)"Video\nplayer"
#define DEF_SAPP0_VER (std::string)"v1.2.0"
#define DEF_SAPP0_MAIN_STR (std::string)"Vid/Main"
#define DEF_SAPP0_INIT_STR (std::string)"Vid/Init"
#define DEF_SAPP0_EXIT_STR (std::string)"Vid/Exit"
#define DEF_SAPP0_DECODE_THREAD_STR (std::string)"Vid/Decode thread"
#define DEF_SAPP0_DECODE_VIDEO_THREAD_STR (std::string)"Vid/Decode video thread"
#define DEF_SAPP0_CONVERT_THREAD_STR (std::string)"Vid/Convert thread"

//video player(mvd)
#define DEF_SAPP1_NUM_OF_MSG 15
#define DEF_SAPP1_ENABLE_ICON
//#define DEF_SAPP1_ENABLE_NAME
#define DEF_SAPP1_ICON_PATH (std::string)"romfs:/gfx/draw/icon/vid_mvd_icon.t3x"
#define DEF_SAPP1_NAME (std::string)"mvd test"
#define DEF_SAPP1_VER (std::string)"v1.2.0"
#define DEF_SAPP1_MAIN_STR (std::string)"Vid/Main"
#define DEF_SAPP1_INIT_STR (std::string)"Vid/Init"
#define DEF_SAPP1_EXIT_STR (std::string)"Vid/Exit"
#define DEF_SAPP1_DECODE_THREAD_STR (std::string)"Vid/Decode thread"
#define DEF_SAPP1_DECODE_VIDEO_THREAD_STR (std::string)"Vid/Decode video thread"
#define DEF_SAPP1_CONVERT_THREAD_STR (std::string)"Vid/Convert thread"

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
#define DEF_SAPP2_THREAD_STR (std::string)"Sapp2/Sub thread"

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
#define DEF_SAPP3_THREAD_STR (std::string)"Sapp3/Sub thread"

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
#define DEF_SAPP4_THREAD_STR (std::string)"Sapp4/Sub thread"

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
#define DEF_SAPP5_THREAD_STR (std::string)"Sapp5/Sub thread"

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
#define DEF_SAPP6_THREAD_STR (std::string)"Sapp6/Sub thread"

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
#define DEF_SAPP7_THREAD_STR (std::string)"Sapp7/Sub thread"


//menu
#define DEF_MENU_NUM_OF_MSG 5
#define DEF_MENU_MAIN_STR (std::string)"Menu/Main"
#define DEF_MENU_INIT_STR (std::string)"Menu/Init"
#define DEF_MENU_EXIT_STR (std::string)"Menu/Exit"
#define DEF_MENU_WORKER_THREAD_STR (std::string)"Menu/Worker thread"
#define DEF_MENU_UPDATE_THREAD_STR (std::string)"Menu/Update thread"
#define DEF_MENU_SEND_APP_INFO_STR (std::string)"Menu/Send app info thread"
#define DEF_MENU_CHECK_INTERNET_STR (std::string)"Menu/Check internet thread"

//setting menu
#define DEF_SEM_NUM_OF_MSG 59
#define DEF_SEM_ENABLE_ICON
//#define DEF_SEM_ENABLE_NAME
#define DEF_SEM_ICON_PATH (std::string)"romfs:/gfx/draw/icon/sem_icon.t3x"
#define DEF_SEM_NAME (std::string)"Settings"
#define DEF_SEM_INIT_STR (std::string)"Sem/Init"
#define DEF_SEM_EXIT_STR (std::string)"Sem/Exit"
#define DEF_SEM_WORKER_THREAD_STR (std::string)"Sem/Worker thread"
#define DEF_SEM_UPDATE_THREAD_STR (std::string)"Sem/Update thread"
#define DEF_SEM_ENCODE_THREAD_STR (std::string)"Sem/Encode thread"
#define DEF_SEM_RECORD_THREAD_STR (std::string)"Sem/Record thread"

//explorer
#define DEF_EXPL_INIT_STR (std::string)"Expl/Init"
#define DEF_EXPL_EXIT_STR (std::string)"Expl/Exit"
#define DEF_EXPL_READ_DIR_THREAD_STR (std::string)"Expl/Read dir thread"

//external font
#define DEF_EXFONT_NUM_OF_FONT_NAME 50
#define DEF_EXFONT_INIT_STR (std::string)"Exfont/Init"
#define DEF_EXFONT_EXIT_STR (std::string)"Exfont/Exit"
#define DEF_EXFONT_LOAD_FONT_THREAD_STR (std::string)"Exfont/Load font thread"

//draw
#define DEF_DRAW_MAX_NUM_OF_SPRITE_SHEETS 128

//abgr8888 color
#define DEF_DRAW_RED 0xFF0000FF
#define DEF_DRAW_GREEN 0xFF00FF00
#define DEF_DRAW_BLUE 0xFFFF0000
#define DEF_DRAW_BLACK 0xFF000000
#define DEF_DRAW_WHITE 0xFFFFFFFF
#define DEF_DRAW_AQUA 0xFFFFFF00
#define DEF_DRAW_YELLOW 0xFF00C5FF
#define DEF_DRAW_WEAK_RED 0x500000FF
#define DEF_DRAW_WEAK_GREEN 0x5000FF00
#define DEF_DRAW_WEAK_BLUE 0x50FF0000
#define DEF_DRAW_WEAK_BLACK 0x50000000
#define DEF_DRAW_WEAK_WHITE 0x50FFFFFF
#define DEF_DRAW_WEAK_AQUA 0x50FFFF00
#define DEF_DRAW_WEAK_YELLOW 0x5000C5FF
#define DEF_DRAW_NO_COLOR 0x0

//decoder 
#define DEF_DECODER_MAX_AUDIO_TRACKS 8
#define DEF_DECODER_MAX_VIDEO_TRACKS 2

//error num
#define DEF_ERR_SUMMARY 0
#define DEF_ERR_DESCRIPTION 1
#define DEF_ERR_PLACE 2
#define DEF_ERR_CODE 3

//error code
#define DEF_ERR_OTHER 0xFFFFFFFF
#define DEF_ERR_OUT_OF_MEMORY 0xFFFFFFFE
#define DEF_ERR_OUT_OF_LINEAR_MEMORY 0xFFFFFFFD
#define DEF_ERR_GAS_RETURNED_NOT_SUCCESS 0xFFFFFFFC
#define DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS 0xFFFFFFFB
#define DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS 0xFFFFFFFA
#define DEF_ERR_INVALID_ARG 0xFFFFFFF9

#define DEF_ERR_OTHER_STR (std::string)"[Error] Something went wrong. "
#define DEF_ERR_OUT_OF_MEMORY_STR (std::string)"[Error] Out of memory. "
#define DEF_ERR_OUT_OF_LINEAR_MEMORY_STR (std::string)"[Error] Out of linear memory. "
#define DEF_ERR_GAS_RETURNED_NOT_SUCCESS_STR (std::string)"[Error] Google apps script returned NOT success. "
#define DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS_STR (std::string)"[Error] stb image returned NOT success. "
#define DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR (std::string)"[Error] ffmpeg returned NOT success. "
#define DEF_ERR_INVALID_ARG_STR (std::string)"[Error] Invalid arg. "

//log
#define DEF_LOG_BUFFER_LINES 512
#define DEF_LOG_DISPLAYED_LINES 23

//thread
#define DEF_STACKSIZE (64 * 1024)
#define DEF_INACTIVE_THREAD_SLEEP_TIME 100000
#define DEF_ACTIVE_THREAD_SLEEP_TIME 50000
//0x18~0x3F
#define DEF_THREAD_PRIORITY_IDLE 0x36
#define DEF_THREAD_PRIORITY_LOW 0x25
#define DEF_THREAD_PRIORITY_NORMAL 0x24
#define DEF_THREAD_PRIORITY_HIGH 0x23
#define DEF_THREAD_PRIORITY_REALTIME 0x18
