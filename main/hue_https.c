#include "hue_https.h"

static const char* tag = "hue_https";

extern const char hue_signify_root_cert_pem_start[] asm("_binary_hue_signify_root_cert_pem_start");
extern const char hue_signify_root_cert_pem_end[] asm("_binary_hue_signify_root_cert_pem_end");

/* TODO: Hue Resource JSON construction
 *  [ ] light
 *       [x] on:{on: bool}
 *       [x] dimming:{brightness: int[1-100]}
 *       [x] dimming_delta:{action: str[up, down, stop],
 *                          brightness_delta: int[0-100]}
 *       [x] color_temperature:{mirek: int[153-500]}
 *       [x] color_temperature_delta:{action: str[up, down, stop],
 *                                    mirek_delta: int[0-347]}
 *       [x] color:{xy:{x: float[0-1],
 *                      y: float[0-1]}}
 *  [ ] scene
 *       [ ] recall:{action: str[active, static],
 *                   duration: uint,
 *                   dimming:{brightness: int[1-100]}}
 *  [ ] grouped_light
 *       [x] on:{on: bool}
 *       [x] dimming:{brightness: int[1-100]}
 *       [x] dimming_delta:{action: str[up, down, stop],
 *                          brightness_delta: int[0-100]}
 *       [x] color_temperature:{mirek: int[153-500]}
 *       [x] color_temperature_delta:{action: str[up, down, stop],
 *                                    mirek_delta: int[0-347]}
 *       [x] color:{xy:{x: float[0-1],
 *                      y: float[0-1]}}
 *  [ ] smart_scene
 *       [x] recall:{action: str[activate, deactivate]}
 */