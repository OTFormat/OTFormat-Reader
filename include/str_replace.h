/*
 * Copyright 2021 FUJIFILM Corporation
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file str_replace.h
 * @brief Function declaration to replace string.
 */

#ifndef STRREPLACE_H_
#define STRREPLACE_H_

int   str_replace(const char* src, const char* target, char* replace, char* result);
char* str_substring(const char* const str1, const int start, const int end);

#endif /* STRREPLACE_H_ */
