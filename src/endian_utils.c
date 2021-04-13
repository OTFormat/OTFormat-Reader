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
 * @file  endian_utils.c
 * @brief Functions to convert a data between BIG and LITTLE endian you want to read/write.
 */

#include "endian_utils.h"

/**
 * Read a file with a specified endian.
 * Expected size = 2 bytes x count.
 *
 * NOTE: Specify an endian you want to read from a file.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of a file
 * @param [out] (buffer) Converted data will be stored.
 * @param [in]  (count)  Number of objects expectedly read.
 * @param [in]  (fp)     File pointer.
 * @return same as fread(). i.e. return a number of objects actually read,
 *         or 0 when an error occurred.
 */
size_t fread16( Endian endian, uint16_t *buffer, size_t count, FILE *fp )
{
  size_t i, read_count;
  uint8_t read_buf[ 2 ];

  for ( i = 0; i < count; i++ ) {
    read_count = fread( read_buf, 2, 1, fp );
    if ( read_count != 1 ) {
      break;
    }

    r16( endian, read_buf, buffer, 1 );
  }

  return i;
}

/**
 * Read a file with a specified endian.
 * Expected size = 4 bytes x count.
 *
 * NOTE: Specify an endian you want to read from a file.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of a file
 * @param [out] (buffer) Converted data will be stored.
 * @param [in]  (count)  Number of objects expectedly read.
 * @param [in]  (fp)     File pointer.
 * @return same as fread(). i.e. return a number of objects actually read,
 *         or 0 when an error occurred.
 */
size_t fread32( Endian endian, uint32_t *buffer, size_t count, FILE *fp )
{
  size_t i, read_count;
  uint8_t read_buf[ 4 ];

  for ( i = 0; i < count; i++ ) {
    read_count = fread( read_buf, 4, 1, fp );
    if ( read_count != 1 ) {
      break;
    }

    r32( endian, read_buf, buffer, 1 );
  }

  return i;
}

/**
 * Read a file with a specified endian.
 * Expected size = 8 bytes x count.
 *
 * NOTE: Specify an endian you want to read from a file.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of a file
 * @param [out] (buffer) Converted data will be stored.
 * @param [in]  (count)  Number of objects expectedly read.
 * @param [in]  (fp)     File pointer.
 * @return same as fread(). i.e. return a number of objects actually read,
 *         or 0 when an error occurred.
 */
size_t fread64( Endian endian, uint64_t *buffer, size_t count, FILE *fp )
{
  size_t i, read_count;
  uint8_t read_buf[ 8 ];

  for ( i = 0; i < count; i++ ) {
    read_count = fread( read_buf, 8, 1, fp );
    if ( read_count != 1 ) {
      break;
    }

    r64( endian, read_buf, buffer, 1 );
  }

  return i;
}

/**
 * Write a buffer into a file with a specified endian.
 * Expected size = 2 bytes x count.
 *
 * NOTE: Specify an endian you want to write in a file.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of a file
 * @param [in]  (buffer) Original data
 * @param [in]  (count)  Number of objects expectedly write.
 * @param [out] (fp)     File pointer to a file.
 * @return same as fwrite(). i.e. return a number of objects actually write,
 *         or 0 when an error occurred.
 */
size_t fwrite16( Endian endian, uint16_t *buffer, size_t count, FILE *fp )
{
  size_t i, write_count;
  uint8_t write_buf[ 2 ];

  for ( i = 0; i < count; i++ ) {
    w16( endian, buffer, write_buf, 1 );

    write_count = fwrite( write_buf, 2, 1, fp );
    if ( write_count != 1 ) {
      break;
    }
  }

  return i;
}

/**
 * Write a buffer into a file with a specified endian.
 * Expected size = 4 bytes x count.
 *
 * NOTE: Specify an endian you want to write in a file.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of a file
 * @param [in]  (buffer) Original data
 * @param [in]  (count)  Number of objects expectedly write.
 * @param [out] (fp)     File pointer to a file.
 * @return same as fwrite(). i.e. return a number of objects actually write,
 *         or 0 when an error occurred.
 */
size_t fwrite32( Endian endian, uint32_t *buffer, size_t count, FILE *fp )
{
  size_t i, write_count;
  uint8_t write_buf[ 4 ];

  for ( i = 0; i < count; i++ ) {
    w32( endian, buffer, write_buf, 1 );

    write_count = fwrite( write_buf, 4, 1, fp );
    if ( write_count != 1 ) {
      break;
    }
  }

  return i;
}

/**
 * Write a buffer into a file with a specified endian.
 * Expected size = 8 bytes x count.
 *
 * NOTE: Specify an endian you want to write in a file.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of a file
 * @param [in]  (buffer) Original data
 * @param [in]  (count)  Number of objects expectedly write.
 * @param [out] (fp)     File pointer to a file.
 * @return same as fwrite(). i.e. return a number of objects actually write,
 *         or 0 when an error occurred.
 */
size_t fwrite64( Endian endian, uint64_t *buffer, size_t count, FILE *fp )
{
  size_t i, write_count;
  uint8_t write_buf[ 8 ];

  for ( i = 0; i < count; i++ ) {
    w64( endian, buffer, write_buf, 1 );

    write_count = fwrite( write_buf, 8, 1, fp );
    if ( write_count != 1 ) {
      break;
    }
  }

  return i;
}

/**
 * Convert from a data with a specified endian.
 * Expected size = 2 bytes x count.
 *
 * Specify an endian you want to read.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of the target file
 * @param [in]  (from)   Original data
 * @param [out  (to)     Converted data will be stored.
 * @param [in]  (count)  Number of objects expectedly read.
 * @return       A number of objects actually read, or 0 when an error occurred.
 */
size_t r16(const Endian endian, const uint8_t *from,  uint16_t *to, const size_t count)
{
  uint16_t c1, c2;
  size_t i;

  for ( i = 0; i < count; i++ ) {
    c1 = (uint16_t)(*from++);
    c2 = (uint16_t)(*from++);

    switch ( endian ) {
    case ( BIG ):
      *to++ = (c1 << 8) | (c2 & 0x00ff);
      break;

    case ( LITTLE ):
      *to++ = (c2 << 8) | (c1 & 0x00ff);
      break;

    default:
      return 0;
    }
  }

  return i;
}

/**
 * Convert from a data with a specified endian.
 * Expected size = 4 bytes x count.
 *
 * Specify an endian you want to read.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of the target file
 * @param [in]  (from)   Original data
 * @param [out  (to)     Converted data will be stored.
 * @param [in]  (count)  Number of objects expectedly read.
 * @return       A number of objects actually read, or 0 when an error occurred.
 */
size_t r32(const Endian endian, const uint8_t *from, uint32_t *to, const size_t count)
{
  unsigned long c1, c2, c3, c4;
  size_t i;

  for ( i = 0; i < count; i++ ) {
    c1 = (uint32_t)(*from++);
    c2 = (uint32_t)(*from++);
    c3 = (uint32_t)(*from++);
    c4 = (uint32_t)(*from++);

    switch ( endian ) {
    case ( BIG ):
      *to++ =
        ( c1 << 24) |
        ((c2 << 16) & 0x00ff0000UL) |
        ((c3 <<  8) & 0x0000ff00UL) |
        ( c4        & 0x000000ffUL);
      break;

    case ( LITTLE ):
      *to++ =
        ( c4 << 24) |
        ((c3 << 16) & 0x00ff0000UL) |
        ((c2 <<  8) & 0x0000ff00UL) |
        ( c1        & 0x000000ffUL);
      break;

    default:
      return 0;
    }
  }

  return i;
}

/**
 * Convert from a data with a specified endian.
 * Expected size = 8 bytes x count.
 *
 * Specify an endian you want to read.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of the target file
 * @param [in]  (from)   Original data
 * @param [out  (to)     Converted data will be stored.
 * @param [in]  (count)  Number of objects expectedly read.
 * @return       A number of objects actually read, or 0 when an error occurred.
 */
size_t r64(const Endian endian, const uint8_t *from, uint64_t *to, const size_t count)
{
  uint8_t  c1, c2, c3, c4, c5, c6, c7, c8;
  size_t i;

  for ( i = 0; i < count; i++ ) {
    c1 = *from++;
    c2 = *from++;
    c3 = *from++;
    c4 = *from++;
    c5 = *from++;
    c6 = *from++;
    c7 = *from++;
    c8 = *from++;

    switch ( endian ) {
    case ( BIG ):
      *to++ =
        ( ((uint64_t)c1)<<56) |
        ((((uint64_t)c2)<<48) & 0x00ff000000000000ULL) |
        ((((uint64_t)c3)<<40) & 0x0000ff0000000000ULL) |
        ((((uint64_t)c4)<<32) & 0x000000ff00000000ULL) |
        ((((uint64_t)c5)<<24) & 0x00000000ff000000ULL) |
        ((((uint64_t)c6)<<16) & 0x0000000000ff0000ULL) |
        ((((uint64_t)c7)<< 8) & 0x000000000000ff00ULL) |
        ( ((uint64_t)c8)      & 0x00000000000000ffULL);
      break;

    case ( LITTLE ):
      *to++ =
        ( ((uint64_t)c8)<<56) |
        ((((uint64_t)c7)<<48) & 0x00ff000000000000ULL) |
        ((((uint64_t)c6)<<40) & 0x0000ff0000000000ULL) |
        ((((uint64_t)c5)<<32) & 0x000000ff00000000ULL) |
        ((((uint64_t)c4)<<24) & 0x00000000ff000000ULL) |
        ((((uint64_t)c3)<<16) & 0x0000000000ff0000ULL) |
        ((((uint64_t)c2)<< 8) & 0x000000000000ff00ULL) |
        ( ((uint64_t)c1)      & 0x00000000000000ffULL);
      break;

    default:
      return 0;
    }
  }

  return i;
}

/**
 * Convert to a data with a specified endian.
 * Expected size = 8 bytes x count.
 *
 * Specify an endian you want to write.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of the target file
 * @param [in]  (from)   Original data
 * @param [out  (to)     Converted data will be stored.
 * @param [in]  (count)  Number of objects expectedly write.
 * @return       A number of objects actually write, or 0 when an error occurred.
 */
size_t w16(const Endian endian, const uint16_t *from, uint8_t *to, const size_t count)
{
  uint8_t c1, c2;
  size_t i;

  for ( i = 0; i < count; i++) {
    c1 = (uint8_t)(*from >> 8);
    c2 = (uint8_t)(*from     );
    from++;

    switch(endian) {
    case ( BIG ):
      *to++ = c1;
      *to++ = c2;
      break;

    case ( LITTLE ):
      *to++ = c2;
      *to++ = c1;
      break;

    default:
      return 0;
    }
  }

  return i;
}

/**
 * Convert to a data with a specified endian.
 * Expected size = 4 bytes x count.
 *
 * Specify an endian you want to write.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of the target file
 * @param [in]  (from)   Original data
 * @param [out  (to)     Converted data will be stored.
 * @param [in]  (count)  Number of objects expectedly write.
 * @return       A number of objects actually write, or 0 when an error occurred.
 */
size_t w32(const Endian endian, const uint32_t *from, uint8_t *to, const size_t count)
{
  uint8_t c1, c2, c3, c4;
  size_t i;

  for ( i = 0; i < count; i++) {
    c1 = (uint8_t)(*from >> 24);
    c2 = (uint8_t)(*from >> 16);
    c3 = (uint8_t)(*from >>  8);
    c4 = (uint8_t)(*from      );
    from++;

    switch(endian) {
    case ( BIG ):
      *to++ = c1;
      *to++ = c2;
      *to++ = c3;
      *to++ = c4;
      break;

    case ( LITTLE ):
      *to++ = c4;
      *to++ = c3;
      *to++ = c2;
      *to++ = c1;
      break;

    default:
      return 0;
    }
  }

  return i;
}

/**
 * Convert to a data with a specified endian.
 * Expected size = 8 bytes x count.
 *
 * Specify an endian you want to write.
 * i.e. You do NOT need to consider an endian under your environment.
 *
 * @param [in]  (endian) Endian of the target file
 * @param [in]  (from)   Original data
 * @param [out  (to)     Converted data will be stored.
 * @param [in]  (count)  Number of objects expectedly write.
 * @return       A number of objects actually write, or 0 when an error occurred.
 */
size_t w64(const Endian endian, const uint64_t *from, uint8_t *to, const size_t count)
{
  uint8_t c1, c2, c3, c4, c5, c6, c7, c8;
  size_t i;

  for ( i = 0; i < count; i++) {
    c1 = (uint8_t)(*from >> 56);
    c2 = (uint8_t)(*from >> 48);
    c3 = (uint8_t)(*from >> 40);
    c4 = (uint8_t)(*from >> 32);
    c5 = (uint8_t)(*from >> 24);
    c6 = (uint8_t)(*from >> 16);
    c7 = (uint8_t)(*from >>  8);
    c8 = (uint8_t)(*from      );
    from++;

    switch(endian) {
    case ( BIG ):
      *to++ = c1;
      *to++ = c2;
      *to++ = c3;
      *to++ = c4;
      *to++ = c5;
      *to++ = c6;
      *to++ = c7;
      *to++ = c8;
      break;

    case ( LITTLE ):
      *to++ = c8;
      *to++ = c7;
      *to++ = c6;
      *to++ = c5;
      *to++ = c4;
      *to++ = c3;
      *to++ = c2;
      *to++ = c1;
      break;

    default:
      return 0;
    }
  }

  return i;
}

/**
 * Get an endian under your environment
 *
 * @return either BIG or LITTLE
 */
Endian get_my_endian()
{
  int check = 1;
  if ( *((char *) &check) ) return LITTLE;
  else                      return BIG;
}
