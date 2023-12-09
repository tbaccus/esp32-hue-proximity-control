#include <stdint.h>
#include <stdbool.h>

/** @brief Enum for how a value in a hue json structure should be interpreted */
typedef enum {
    HUE_ACTION_NONE = 0, /**< Don't take any action */
    HUE_ACTION_SET,      /**< Set hue's value */
    HUE_ACTION_ADD,      /**< Add to hue's value */
    HUE_ACTION_SUBTRACT  /**< Subtract from hue's value */
} hue_action_t;

/** @brief Settings for Philips Hue light resources */
typedef struct {
    const char* id;
    bool on : 1;                        /**< Light on (true) or off (false) */
    hue_action_t brightness_action : 2; /**< How brightness should be adjusted */
    uint8_t brightness : 7;             /**< [0-100] Amount brightness should be adjusted by or set to */
    hue_action_t color_temp_action : 2; /**< How color temp should be adjusted */
    uint16_t color_temp : 9;            /**< Amount color temp should be adjusted by [0-347] or set to [153-500] */
    bool set_color : 1;                 /**< If color_gamut values should be used */
    uint16_t color_gamut_x : 14;        /**< CIE X gamut position decimal value (e.g. 123 = 0.0123, >=10000 = 1) */
    uint16_t color_gamut_y : 14;        /**< CIE Y gamut position decimal value (e.g. 123 = 0.0123, >=10000 = 1) */
} hue_light_data_t;

/** @brief Settings for Philips Hue light group resources */
typedef hue_light_data_t hue_grouped_light_data_t;

/** @brief Settings for Philips Hue smart scene resources */
typedef struct {
    bool active : 1; /**< Activate (true) or deactivate (false) smart scene */
} hue_smart_scene_data_t;

// TODO: Implement hue_scene_data_t structure
/** @brief Settings for Philips Hue scene resources */
/* typedef struct { } hue_scene_data_t; */