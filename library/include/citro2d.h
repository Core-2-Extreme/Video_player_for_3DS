/**
 * @file citro2d.h
 * @brief Central citro2d header. Includes all others.
 */
#pragma once

#ifdef CITRO2D_BUILD
#error "This header file is only for external users of citro2d."
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if __GNUC__
//We don't want to see warnings in 3rd party headers.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif //__GNUC__

#include <citro3d.h>
#include <tex3ds.h>

#include "c2d/base.h"
#include "c2d/spritesheet.h"
#include "c2d/sprite.h"
#include "c2d/text.h"
#include "c2d/font.h"

#if __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__

#ifdef __cplusplus
}
#endif
