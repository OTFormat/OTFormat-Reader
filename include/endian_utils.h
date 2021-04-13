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
 * @file  endian_utils.h
 * @brief Function declaration to convert a data between BIG and LITTLE endian you want to read/write.
 */

#pragma once

#include <stdio.h>
#include <stdint.h>

typedef enum _Endian_ {
  BIG,
  LITTLE
} Endian;


#ifdef  __cplusplus
extern "C"
{
#endif

  size_t fread16( Endian endian, uint16_t *buffer, size_t count, FILE *fp );
  size_t fread32( Endian endian, uint32_t *buffer, size_t count, FILE *fp );
  size_t fread64( Endian endian, uint64_t *buffer, size_t count, FILE *fp );

  size_t fwrite16( Endian endian, uint16_t *buffer, size_t count, FILE *fp );
  size_t fwrite32( Endian endian, uint32_t *buffer, size_t count, FILE *fp );
  size_t fwrite64( Endian endian, uint64_t *buffer, size_t count, FILE *fp );

  size_t r16(const Endian endian, const uint8_t *from, uint16_t *to, const size_t count);
  size_t r32(const Endian endian, const uint8_t *from, uint32_t *to, const size_t count);
  size_t r64(const Endian endian, const uint8_t *from, uint64_t *to, const size_t count);

  size_t w16(const Endian endian, const uint16_t *from, uint8_t *to, const size_t count);
  size_t w32(const Endian endian, const uint32_t *from, uint8_t *to, const size_t count);
  size_t w64(const Endian endian, const uint64_t *from, uint8_t *to, const size_t count);

  Endian get_my_endian();

#ifdef  __cplusplus
}
#endif
