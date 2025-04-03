#pragma once

#define CHARSIZE 25

#define DATA_FILE "./traybot.data.txt"
#define LOG_FILE "./traybot.log.txt"

#define ACTION_OPEN_FOLDER 0
#define ACTION_SET_CLIPBOARD 1
#define ACTION_RUN_PROGRAM 2

#define TRAY_SHOW_WINDOW 0
#define TRAY_ADD_NEW 1
#define TRAY_LIST_ALL 2
#define TRAY_REMOVE 3
#define TRAY_EXIT 4

#define TRAY_CALLBACK_ID (WM_USER + 1)

#define BTN_CREATE_NEW_OK 1
#define BTN_PROMPT_OK 4