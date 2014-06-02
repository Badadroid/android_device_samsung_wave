#include <linux/input.h>

#include "recovery_ui.h"
#include "common.h"
#include "extendedcommands.h"

int device_handle_key(int key_code, int visible) {
    if (visible) {
        switch (key_code) {
            case KEY_CAPSLOCK:
            case KEY_DOWN:
            case KEY_VOLUMEDOWN:
                return HIGHLIGHT_DOWN;
				break;

            case KEY_LEFTSHIFT:
            case KEY_UP:
            case KEY_VOLUMEUP:
                return HIGHLIGHT_UP;
				break;

            case KEY_MENU:
            case KEY_SLEEP:
                return SELECT_ITEM;
				break;

            case KEY_BACK:
                return GO_BACK;
                break;
            default:
                return NO_ACTION;
				break;
        }
    }

    return NO_ACTION;
}
