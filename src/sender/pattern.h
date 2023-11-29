/*
 * SPDX-FileCopyrightText: Copyright (c) DELTACAST.TV. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at * * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
/*!
   @file pattern.h
   @brief This file contains functions to generate a video pattern.
*/

#ifdef __GNUC__
#include <stdint-gcc.h>
#else
#include <stdint.h>
#endif

#define PIXELSIZE_10BIT 5/2

/*!
   @brief Structure representing yuv color object.
*/

struct Yuv10Bit
{
   uint16_t y;
   uint16_t u;
   uint16_t v;
};

/*!
   @brief Creates a colorbar pattern that supports only yuv 4:2:2 10bits specific ST2110 format.
*/
void create_color_bar_pattern(uint8_t* user_buffer /*!< [in] pointer to the user-instantiated buffer that will be filled.*/
   , uint32_t frame_height /*!< [in] Frame height. */
   , uint32_t frame_width /*!< [in] Frame width.*/
);
/*!
   @brief Draw an horizontal white line in a buffer. Only available for a yuv 4:2:2 10bits specific ST2110 buffer.
*/
void draw_white_line(uint8_t* buffer /*!< [in] Buffer in which the white line will be drawn*/
   , uint32_t line /*!< [in] line position (in pixel)*/
   , uint32_t frame_height /*!< [in] Frame height */
   , uint32_t frame_width /*!< [in] Frame width*/
   , bool interlaced /*!< [in] Is the frame interlaced or not.*/
);