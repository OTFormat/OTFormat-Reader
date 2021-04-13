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
 * @file str_replace.c
 * 
 * Replace string.
 */

#include <stdio.h>
#include <string.h>
#include <json/json.h>
#include "output_level.h"
#include "ltos_format_checker.h"

/**
 * Replace character.
 *  @param src     String to be evaluated.
 *  @param target  String to be replaced.
 *  @param replace String to replace.
 *  @param result  The character string after substitution.
 */
int str_replace(const char* src, const char* target, char* replace, char* result) {
  int ret = OK;

  char* temp = (char *)clf_allocate_memory(strlen(src) * 2, "replace string");

  // Copy the source data to "result" variable for data operation.
  //strncpy(result, src, strlen(src));
  sprintf(result, "%s", src);

  char *target_pointer;
  while ((target_pointer = strstr(result, target)) != NULL) {
    // Insert a null terminate at the end of detected position.
    *target_pointer = EOS;

    // Move a pointer to (detected position + number of target characters)
    target_pointer += strlen(target);

    // Save the target characters in "temp" variable temporarily.
    strcpy(temp, target_pointer);

    // Concatenate the first half characters and replaced characters.
    strcat(result, replace);
    strcat(result, temp);
  }
  free(temp);
  temp = NULL;
  return ret;
}

/**
 * Generate substring from binary data. Typically, this is used for logging.
 * @param str   Pointer to a buffer.
 * @param start Offset of the substring.
 * @param cnum  length of the substring.
 * @return      Substring is returned. Caller should free the pointer after use.
 */
char* str_substring(const char* const str, const int start, const int c_num) {
  char *substring = (char *)clf_allocate_memory(c_num + 1, "creating substring");
  strncpy(substring, str + start, c_num);
  substring[c_num] = EOS;

  return substring;
}
