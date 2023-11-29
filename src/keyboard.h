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

/**
 * @brief Initializes the keyboard for input.
 * 
 * This function sets up the keyboard for input by configuring the necessary
 * hardware and software components. It should be called before any keyboard
 * input is expected.
 */
void init_keyboard();

/**
 * @brief Closes the keyboard and frees any resources used by it.
 * 
 * This function should be called when the keyboard is no longer needed.
 * It will release any resources used by the keyboard, such as file descriptors
 * or memory allocations.
 */
void close_keyboard();

/**
 * Returns true if at least a character is available to be read from the keyboard.
 * @return 1 if a character is available, 0 otherwise.
 */
int _kbhit();

/**
 * @brief Gets a character from the keyboard input buffer.
 * 
 * @return The character read from the keyboard input buffer.
 */
int _getch();
void userQueryTxtDisplayON_keyboard();
void userQueryTxtDisplayOFF_keyboard();
