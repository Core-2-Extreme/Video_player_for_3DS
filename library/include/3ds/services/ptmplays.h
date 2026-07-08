/**
 * @file ptmplays.h
 * @brief PTMPLAYS service. Has access to ptm:u commands, but this is not exposed here.
 */
#pragma once

#include <3ds/types.h>

/// Maximum number of play events that can be stored.
#define PTM_MAX_PLAY_EVENTS 0x11D28u

/// Invalid title ID constant for PTM.
#define PTM_INVALID_TITLE_ID 0xFFFFFFFFFFFFFFFFull

/// Play event types. "Applet" includes "HOME Menu".
typedef enum PtmPlayEventType {
	PTMPLAYEVENT_APPLICATION_LAUNCH         = 0,    ///< Application launch (special meaning for invalid TitleId upon launching DSi title)
	PTMPLAYEVENT_APPLICATION_EXIT           = 1,    ///< Application exit   (likewise)
	PTMPLAYEVENT_APPLET_LAUNCH              = 2,    ///< Applet launch
	PTMPLAYEVENT_APPLET_EXIT                = 3,    ///< Applet exit
	PTMPLAYEVENT_JUMP_TO_APPLICATION        = 4,    ///< Jump to application
	PTMPLAYEVENT_LEAVE_APPLICATION          = 5,    ///< Leave application
	PTMPLAYEVENT_JUMP_TO_APPLET             = 6,    ///< Jump to applet
	PTMPLAYEVENT_LEAVE_APPLET               = 7,    ///< Leave applet
	PTMPLAYEVENT_SHELL_CLOSE                = 8,    ///< Shell close (no TitleId)
	PTMPLAYEVENT_SHELL_OPEN                 = 9,    ///< Shell open (no TitleId)
	PTMPLAYEVENT_SYSTEM_SHUTDOWN            = 10,   ///< System shutdown (no TitleId)
	PTMPLAYEVENT_USER_TIME_CHANGE_OLD_TIME  = 11,   ///< User time change - old time (no TitleId, old user time stored in \ref minutesSince2000)
	PTMPLAYEVENT_USER_TIME_CHANGE_NEW_TIME  = 12,   ///< User time change - new time (no TitleId, new user time stored in \ref minutesSince2000)
} PtmPlayEventType;

/// Play event entry.
typedef struct PtmPlayEvent {
	u32 tidHigh;                    ///< Upper 32 bits of title ID
	u32 tidLow;                     ///< Lower 32 bits of title ID
	PtmPlayEventType type   : 4;    ///< Event type
	u32 minutesSince2000    : 28;   ///< Number of minutes elapsed since 2000-01-01 00:00
} PtmPlayEvent;

/// Get TitleID from PtmPlayEvent.
static inline u64 ptmGetPlayEventTitleId(PtmPlayEvent event) {
	return ((u64)(event.tidHigh) << 32) | event.tidLow;
}

/// Initializes PTMPLAYS.
Result ptmPlaysInit(void);

/// Exits PTMPLAYS.
void ptmPlaysExit(void);

/**
 * @brief Gets a pointer to the current ptm:plays session handle.
 * @return A pointer to the current ptm:plays session handle.
 */
Handle *ptmPlaysGetSessionHandle(void);

/**
 * @brief Gets the PlayEvent history (ring buffer).
 * @param[out] outEndIndex The pointer to write the end index to (ring buffer).
 * @param[out] outEvents The pointer to write the event entries to.
 * @param[in] startIndex The start index (offset) to read from (ring buffer).
 * @param[in] maxEvents The maximum number of events to write to outEvents.
 */
Result PTMPLAYS_GetPlayHistory(s32 *outEndIndex, PtmPlayEvent *outEvents, s32 startIndex, s32 maxEvents);

/**
 * @brief Gets the start index of the PlayEvent history (ring buffer).
 * @param[out] outStartIndex The pointer to write the start index to (ring buffer).
 */
Result PTMPLAYS_GetPlayHistoryStart(s32 *outStartIndex);

/**
 * @brief Gets the number of entries of the PlayEvent history (ring buffer).
 * @param[out] outNumEvents The pointer to write the start index to (ring buffer).
 */
Result PTMPLAYS_GetPlayHistorySize(s32 *outNumEvents);

/**
 * @brief Computes (start + numEvents) % PTM_MAX_PLAY_EVENTS with positive remainder.
 * @param[out] outIndex Result of the index computation.
 */
Result PTMPLAYS_CalcPlayHistoryStart(s32 *outIndex, s32 start, s32 numEvents);
