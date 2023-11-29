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

#include "pattern.h"

#include <iostream>
#include <vector>

void create_color_bar_pattern(uint8_t* user_buffer, uint32_t frame_height, uint32_t frame_width)
{
   const Yuv10Bit white = {0x2D0,0x200,0x200};
   const Yuv10Bit yellow = {0x2A0,0xB0,0x220};
   const Yuv10Bit cyan = {0x244,0x24c,0xB0};
   const Yuv10Bit green = {0x214,0xfc,0xD0};
   const Yuv10Bit magenta = {0xfc,0x304,0x330};
   const Yuv10Bit red = {0xcc,0x184,0x350};
   const Yuv10Bit blue = {0x70,0x350,0x1e0};
   const Yuv10Bit black = {0x40,0x200,0x200};
   std::vector<Yuv10Bit> color_list = {white,yellow,cyan,green,magenta,red,blue,black};

   uint32_t color_id=0;
   int j=0;
   
   if (user_buffer == nullptr)
   {
      std::cout << std::endl << "The  user_buffer pointer is invalid" << std::endl;
      return;
   }

   for(uint32_t pixel_y = 0; pixel_y < frame_height; pixel_y++)
   {
      for (uint32_t pixel_x = 0; pixel_x < frame_width; pixel_x+=8)
      {
         color_id = pixel_x/(frame_width/static_cast<uint32_t>(color_list.size()));
         for(int i=0; i<4; i++)
         {
            user_buffer[(j*20)+(i*5)+0] = (color_list[color_id].u >> 2);
            user_buffer[(j*20)+(i*5)+1] = ((color_list[color_id].u & 0x3)<<6) + ((color_list[color_id].y & 0x3f0)>>4);
            user_buffer[(j*20)+(i*5)+2] = ((color_list[color_id].v & 0x3c0)>>6) + ((color_list[color_id].y & 0xf)<<4);
            user_buffer[(j*20)+(i*5)+3] = ((color_list[color_id].v & 0xcf)<<2) + ((color_list[color_id].y & 0x300)>>8);
            user_buffer[(j*20)+(i*5)+4] = (color_list[color_id].y & 0xff);
         }
         j++;
      }
   }
}

void draw_white_line(uint8_t* buffer, uint32_t line, uint32_t frame_height, uint32_t frame_width,
   bool interlaced)
{
   uint8_t* temp;
   int j=0;
   const Yuv10Bit white = {0x3AC,0x200,0x200};

   if (buffer == nullptr)
   {
      std::cout << std::endl << "The  buffer pointer is invalid" << std::endl;
      return;
   }

   if (interlaced)
   {
      if (line % 2 == 0)
         temp = (buffer + (line / 2) * frame_width * PIXELSIZE_10BIT);
      else
         temp = (buffer + (((frame_height + 1) / 2) + (line / 2)) * frame_width * PIXELSIZE_10BIT);
   }
   else
      temp = (buffer + line * frame_width * PIXELSIZE_10BIT);

   for (uint32_t pixel_x = 0; pixel_x < frame_width; pixel_x+=8)
   {
      for(int i=0; i<4; i++)
      {
         temp[(j*20)+(i*5)+0] = (white.u >> 2);
         temp[(j*20)+(i*5)+1] = ((white.u & 0x3)<<6) + ((white.y & 0x3f0)>>4);
         temp[(j*20)+(i*5)+2] = ((white.v & 0x3c0)>>6) + ((white.y & 0xf)<<4);
         temp[(j*20)+(i*5)+3] = ((white.v & 0xcf)<<2) + ((white.y & 0x300)>>8);
         temp[(j*20)+(i*5)+4] = (white.y & 0xff);
      }
      j++;
   }
}