/*
 * QMK Firmware for Dumbpad iRacing Macropad
 * Layout: 4x4, OLED, Rotary Encoder
 */

#include QMK_KEYBOARD_H

// Layer aliases
#define _DRIVE 0
#define _PIT   1
#define _PC    2

// Custom keycodes
enum custom_keycodes {
    KC_CYCLE = SAFE_RANGE,   // cycles layers DRIVE → PIT → PC → DRIVE
    KC_JIGGLE                // mouse jiggler toggle
};

// Layer names for OLED display
const char *layer_names[] = {
    "Driving",
    "Pit",
    "PC"
};

// Variables for jiggler
bool jiggler_active = false;
int8_t jiggler_dir = 1;
uint16_t last_jiggle = 0;

// Define layers (row-major layout rotated 90° counterclockwise)
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

/* Layer 0: Driving */
[_DRIVE] = LAYOUT(
    KC_F5,       KC_F6,        KC_S,        LCTL(KC_K),         // Row 0
    LALT(KC_R),  KC_VOLU,      KC_VOLD,     LSFT(KC_W),      // Row 1  (L = Lights)
    KC_PGUP,      KC_H,         KC_DOWN,      KC_UP,        // Row 2  (H = Headlights, Gear Down/Up)
    KC_CYCLE,     KC_MINS,      KC_EQL,       KC_ESC,       // Row 3  (Wipers, Pit Limiter Off)
    KC_NO                                               // encoder placeholder
),

/* Layer 1: Pit Strategy */
[_PIT] = LAYOUT(
    LCTL(KC_A),   LCTL(KC_F),   LCTL(KC_A),   KC_F7,      // Row 0 (Camera / Menu navigation)
    KC_TAB,       KC_PLUS,      KC_DEL,       KC_DOWN,      // Row 1 (Fuel +, Delete, Down)
    KC_UP,        KC_MINUS,     KC_0,         KC_P,         // Row 2 (Fuel -, 0, Pit limiter toggle)
    KC_CYCLE,     KC_4,         KC_3,         KC_2,         // Row 3 (Quick presets)
    KC_NO                                               // encoder placeholder
),

/* Layer 2: PC Utility */
[_PC] = LAYOUT(
    LGUI(LSFT(KC_S)), KC_COPY,   KC_CUT,    KC_VOLU,   // Row 0
    KC_UNDO,          KC_VOLD,   KC_MPLY,   KC_MUTE,   // Row 1
    KC_JIGGLE,        KC_HOME,   KC_END,    KC_NO,     // Row 2
    KC_CYCLE,         KC_DOWN,   KC_LEFT,   KC_RGHT,   // Row 3
    KC_NO                                         // encoder placeholder
)
};

// Encoder rotation behavior
bool encoder_update_user(uint8_t index, bool clockwise) {
    switch (get_highest_layer(layer_state)) {
        case _DRIVE:
            tap_code(clockwise ? KC_LBRC : KC_RBRC); // Brake Bias -/+ ( [ ] )
            break;
        case _PIT:
            tap_code(clockwise ? KC_TAB : S(KC_TAB)); // Black-box next/prev
            break;
        case _PC:
            tap_code(clockwise ? KC_VOLU : KC_VOLD); // Volume control
            break;
    }
    return true;
}

// Encoder press triggers layer cycle
bool encoder_button_update_user(bool pressed) {
    if (pressed) {
        uint8_t current = get_highest_layer(layer_state);
        uint8_t next = (current + 1) % 3;
        layer_move(next);
        if (next != _PC && jiggler_active) jiggler_active = false;
    }
    return true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;

    switch (keycode) {
        case KC_CYCLE: {
            uint8_t current = get_highest_layer(layer_state);
            uint8_t next = (current + 1) % 3;
            layer_move(next);
            if (next != _PC && jiggler_active) jiggler_active = false;
            return false;
        }
        case KC_JIGGLE:
            if (get_highest_layer(layer_state) == _PC) {
                jiggler_active = !jiggler_active;
                if (jiggler_active) {
                    last_jiggle = timer_read();
                }
            }
            return false;
    }
    return true;
}

void matrix_scan_user(void) {
    if (jiggler_active && get_highest_layer(layer_state) == _PC && timer_elapsed(last_jiggle) > 100) {
        tap_code(jiggler_dir > 0 ? KC_MS_RIGHT : KC_MS_LEFT);
        jiggler_dir = -jiggler_dir;
        last_jiggle = timer_read();
    }
}

#ifdef OLED_ENABLE

// Dim the OLED instead of turning it off
__attribute__ ((weak))
uint32_t oled_task_interval(void) {
    return 1000; // update every 1000ms
}

__attribute__ ((weak))
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    oled_on(); // Ensure it's on when keyboard powers up
    return rotation;
}

void oled_render_layer_state(void) {
    oled_write_P(PSTR("Layer: "), false);
    oled_write_ln(layer_names[get_highest_layer(layer_state)], false);
}

bool oled_task_user(void) {
    static bool initialized = false;
    if (!initialized) { oled_clear(); initialized = true; }
    oled_render_layer_state();
    return false;
}
#endif
