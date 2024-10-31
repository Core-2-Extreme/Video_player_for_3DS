#if !defined(DEF_HW_CONFIG_TYPES_H)
#define DEF_HW_CONFIG_TYPES_H
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t Hw_config_wakeup_bit;
#define HW_CONFIG_WAKEUP_BIT_NONE				(Hw_config_wakeup_bit)(0 << 0)	//No wake up event.
#define HW_CONFIG_WAKEUP_BIT_PRESS_HOME_BUTTON	(Hw_config_wakeup_bit)(1 << 2)	//Wake up if home button is pressed.
#define HW_CONFIG_WAKEUP_BIT_OPEN_SHELL			(Hw_config_wakeup_bit)(1 << 5)	//Wake up if shell is opened.

#endif //!defined(DEF_HW_CONFIG_TYPES_H)
