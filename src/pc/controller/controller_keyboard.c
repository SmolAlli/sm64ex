#include <stdbool.h>
#include <ultra64.h>
#include <stdio.h>

#include "controller_api.h"

#ifdef TARGET_WEB
#include "controller_emscripten_keyboard.h"
#endif

#include "../configfile.h"
#include "controller_keyboard.h"

static int keyboard_buttons_down;

#define MAX_KEYBINDS 64
static int keyboard_mapping[MAX_KEYBINDS][2];
static int num_keybinds = 0;

static u32 keyboard_lastkey = VK_INVALID;

static int keyboard_map_scancode(int scancode) {
    int ret = 0;
    for (int i = 0; i < num_keybinds; i++) {
        if (keyboard_mapping[i][0] == scancode) {
            ret |= keyboard_mapping[i][1];
        }
    }
    return ret;
}

bool keyboard_on_key_down(int scancode) {
    int mapped = keyboard_map_scancode(scancode);
    keyboard_buttons_down |= mapped;
    keyboard_lastkey = scancode;
    return mapped != 0;
}

bool keyboard_on_key_up(int scancode) {
    int mapped = keyboard_map_scancode(scancode);
    keyboard_buttons_down &= ~mapped;
    if (keyboard_lastkey == (u32) scancode)
        keyboard_lastkey = VK_INVALID;
    return mapped != 0;
}

void keyboard_on_all_keys_up(void) {
    keyboard_buttons_down = 0;
}

static void keyboard_add_binds(int mask, unsigned int *scancode) {
    for (int i = 0; i < MAX_BINDS && num_keybinds < MAX_KEYBINDS; ++i) {
        if (scancode[i] < VK_BASE_KEYBOARD + VK_SIZE) {
            keyboard_mapping[num_keybinds][0] = scancode[i];
            keyboard_mapping[num_keybinds][1] = mask;
            num_keybinds++;
        }
    }
}

static void keyboard_bindkeys(void) {
    bzero(keyboard_mapping, sizeof(keyboard_mapping));
    num_keybinds = 0;

    keyboard_add_binds(STICK_UP, configKeyStickUp);
    keyboard_add_binds(STICK_LEFT, configKeyStickLeft);
    keyboard_add_binds(STICK_DOWN, configKeyStickDown);
    keyboard_add_binds(STICK_RIGHT, configKeyStickRight);
    keyboard_add_binds(A_BUTTON, configKeyA);
    keyboard_add_binds(B_BUTTON, configKeyB);
    keyboard_add_binds(Z_TRIG, configKeyZ);
    keyboard_add_binds(U_CBUTTONS, configKeyCUp);
    keyboard_add_binds(L_CBUTTONS, configKeyCLeft);
    keyboard_add_binds(D_CBUTTONS, configKeyCDown);
    keyboard_add_binds(R_CBUTTONS, configKeyCRight);
    keyboard_add_binds(L_TRIG, configKeyL);
    keyboard_add_binds(R_TRIG, configKeyR);
    keyboard_add_binds(START_BUTTON, configKeyStart);
    keyboard_add_binds(SPEEDKICK, configKeySpeedkick);
    keyboard_add_binds(QUICKTURN, configKeyQuickturn);
}

static void keyboard_init(void) {
    keyboard_bindkeys();

#ifdef TARGET_WEB
    controller_emscripten_keyboard_init();
#endif
}

static void keyboard_read(OSContPad *pad) {
    const s8 defaultStick = 127;
    const s8 speedkickModifier = 41;
    const s8 speedkickModifierDiagonal = 29;
    const s8 quickturnModifier = 25;
    const s8 quickturnModifierDiagonal = 17;

    pad->button |= keyboard_buttons_down;
    const u32 speedkick = keyboard_buttons_down & SPEEDKICK;
    const u32 quickturn = keyboard_buttons_down & QUICKTURN;

    const u32 xstick = keyboard_buttons_down & STICK_XMASK;
    const u32 ystick = keyboard_buttons_down & STICK_YMASK;

    // Modifiers for Speedkicks and Quickturns
    s8 stick;
    if (speedkick == SPEEDKICK) {
        if (xstick != 0 && ystick != 0) {
            stick = speedkickModifierDiagonal;
        } else {
            stick = speedkickModifier;
        }
    } else if (quickturn == QUICKTURN) {
        if (xstick != 0 && ystick != 0) {
            stick = quickturnModifierDiagonal;
        } else {
            stick = quickturnModifier;
        }
    } else {
        stick = defaultStick;
    }

    // Chooses left if both l+r are pressed
    if ((xstick == STICK_LEFT) || (xstick == STICK_LEFT + STICK_RIGHT))
        pad->stick_x = -1 * stick;
    else if (xstick == STICK_RIGHT)
        pad->stick_x = stick;
    // Chooses down if both d+u are pressed
    if ((ystick == STICK_DOWN) || (ystick == STICK_DOWN + STICK_UP))
        pad->stick_y = -1 * stick;
    else if (ystick == STICK_UP)
        pad->stick_y = stick;
}

static u32 keyboard_rawkey(void) {
    const u32 ret = keyboard_lastkey;
    keyboard_lastkey = VK_INVALID;
    return ret;
}

static void keyboard_shutdown(void) {
}

struct ControllerAPI controller_keyboard = {
    VK_BASE_KEYBOARD,  keyboard_init,    keyboard_read, keyboard_rawkey, NULL, NULL,
    keyboard_bindkeys, keyboard_shutdown
};
