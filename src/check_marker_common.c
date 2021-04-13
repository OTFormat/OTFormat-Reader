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
 * @file check_marker_common.c
 * 
 * Common functions for checking a marker relating to OTFormat.
 */

#include "ltos_format_checker.h"

/**
 * Get expected value of identifier size.
 * @param [in]  (identifier)     Pointer of defined identifier of each markers.
 * @param [out] (expected_value) Expected value of identifier size.
 * @return      (OK/NG)
 */
static int get_expected_value_of_header_size(const char* const identifier, uint64_t* const expected_value) {
  int ret = OK;
  if (!strncmp(identifier, RCM_IDENTIFIER, IDENTIFIER_SIZE)) {
    *expected_value = RCM_HEADER_SIZE;
  } else if (!strncmp(identifier, PR_IDENTIFIER, IDENTIFIER_SIZE)) {
    *expected_value = PR_HEADER_SIZE;
  } else if (!strncmp(identifier, OCM_IDENTIFIER, IDENTIFIER_SIZE)) {
    *expected_value = OCM_HEADER_SIZE;
  } else {
    ret = NG;
  }
  return ret;
}

/**
 * Get expected value of a directory size.
 * @param [in]  (identifier)     Pointer of defined identifier of each markers.
 * @param [out] (expected_value) Expected value of a directory size.
 * @return      (OK/NG)
 */
static int get_expected_value_of_dir_size(const char* const identifier, uint64_t* const expected_value) {
  int ret = OK;
  if (!strncmp(identifier, RCM_IDENTIFIER, IDENTIFIER_SIZE)) {
    *expected_value = RCM_DIR_SIZE;
  } else if (!strncmp(identifier, PR_IDENTIFIER, IDENTIFIER_SIZE)) {
    *expected_value = PR_DIR_SIZE;
  } else if (!strncmp(identifier, OCM_IDENTIFIER, IDENTIFIER_SIZE)) {
    *expected_value = OCM_DIR_SIZE;
  } else {
    ret = NG;
  }
  return ret;
}

/**
 * Check directory offset format of header.
 * @param [in]      (identifier)       Pointer of defined identifier of each markers.
 * @param [in]      (buffer)           Buffer pointer of markers on this tape.
 * @param [in/out]  (current_position) Current position of the buffer pointer.
 * @param [out]     (directory_offset) Directory offset of header.
 * @return          (OK/NG)            If the format is correct or not.
 */
static int check_dir_offset_of_header(const char* const identifier, const char* const buffer,
                                      uint64_t* const current_position, uint64_t* const directory_offset) {
  int ret                 = OK;
  uint64_t expected_value = 0;
  r64(BIG, (unsigned char *)(buffer + *current_position), directory_offset, 1);
  ret |= get_expected_value_of_header_size(identifier, &expected_value);
  if (*directory_offset != expected_value) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Directory_offset is not correct.\n"
                              "%sActual value  :%lu\n"
                              "%sExpected value:%lu\n", INDENT, *directory_offset, INDENT, expected_value);
  }
  *current_position += DIRECTORY_OFFSET_SIZE;
  return ret;
}

/**
 * Check data offset format of header.
 * @param [in]  (directory_offset) Directory offset of header.
 * @param [in]  (data_offset)      Data offset of header.
 * @param [in]  (num_of_marker)    Number of /partial reference/ocm/packed objects/ of header.
 * @return      (OK/NG)            If the format is correct or not.
 */
static int check_integrity_of_dir_offset(const char* const identifier, const uint64_t* const directory_offset, const uint64_t* const data_offset, const uint64_t* const num_of_marker) {
  int ret                = OK;
  uint64_t expected_value = 0;
  get_expected_value_of_dir_size(identifier, &expected_value);
  if (*data_offset != (*directory_offset + (*num_of_marker * expected_value))) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Data offset, directory_offset and number of markers are not consistent.\n"
                              "%sActual value(Data offset)       :%lu\n"
                              "%sActual value(Directory offset)  :%lu\n"
                              "%sActual value(Number of markers) :%lu\n"
                              "%sExpected value:(Data offset) = (Directory offset) + %i * (Number of markers)\n",
                              INDENT, *directory_offset, INDENT, *data_offset, INDENT, *num_of_marker, INDENT, expected_value);
  }
  return ret;
}

/**
 * Check buffer is null filled or not.
 * @param[in]  (buffer)     Buffer to check.
 * @param[in]  (length)     Check buffer from offset 0 to length-1.
 * @return     (TRUE/FALSE) If buffer is filled by NULL i.e. 0, return TRUE. Otherwise, return FALSE.
 */
int is_null_filled(const char* const buffer, const uint64_t length) {
  int ret = TRUE;
  for (uint64_t i = 0; i < length; i++) {
    if (buffer[i] != 0) {
      ret = FALSE;
      break;
    }
  }
  return ret;
}

/**
 * Check header format.
 * @param [in]      (identifier)       Pointer of defined identifier of each markers.
 * @param [in]      (buffer)           Buffer pointer of markers on this tape.
 * @param [in]      (mamhta)           Pointer of a host-type attributes. NULL pointer is acceptable.
 * @param [in]      (info_flag)        ON:(OCM Info or PO Info) OFF:(OCM or PO)
 * @param [in/out]  (current_position) Current position of the buffer pointer.
 * @param [out]     (num_of_marker)    Pointer of the numbers of markers.
 * @param [out]     (data_length)      Pointer of byte size of a bucket list.
 * @return          (OK/NG)            If the format is correct or not.
 */
int clf_header(const char* const identifier, const char* const buffer,
               const MamHta* const mamhta, int info_flag, uint64_t* const current_position,
               uint64_t* const num_of_marker, uint64_t* const data_length) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "start:clf_header: %s\n", identifier);

  uint64_t directory_offset         = 0;
  uint64_t data_offset              = 0;
  char system_id[UUID_SIZE + 1]     = { '\0' };
  char pool_id[UUID_SIZE + 1]       = { '\0' };
#ifndef FORMAT_031
  char pool_group_id[UUID_SIZE + 1] = { '\0' };
#endif
  int get_data_length_flag          = OFF;
  int get_id_flag                   = OFF;

  if (identifier == NULL || buffer == NULL || current_position == NULL
   || num_of_marker == NULL || data_length == NULL) {
    ret = output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT,
                             "Invalid arguments at clf_header: identifier = %p, buffer = %p,"
                             "mamhta = %p, current_position = %p, num_of_marker = %p, data_length = %p\n",
                             identifier, buffer, mamhta, current_position, num_of_marker, data_length);
  }

  if (info_flag == OFF && strncmp(buffer, identifier, IDENTIFIER_SIZE) != EQUAL) {
    char* p = str_substring(buffer, 0, IDENTIFIER_SIZE);
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Identifier format is not correct.\n"
                              "%sActual format  :%s\n"
                              "%sExpected format:%s\n", INDENT, p, INDENT, identifier);
    free(p);
    p = NULL;
  } else {
    ret |= output_accdg_to_vl(OUTPUT_INFO, DEFAULT, "%s\n",buffer);
  }
  if (info_flag == ON) {
    *current_position -= IDENTIFIER_SIZE;
  } else if (!strncmp(buffer, RCM_IDENTIFIER, IDENTIFIER_SIZE)) {
    if (!strcmp(get_top_verbose(), DISPLAY_HEADER_AND_L4_INFO)) {
      get_data_length_flag = ON;
      get_id_flag          = ON;
    } else {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Identifier format is not correct.\n"
                                "%sActual format  :%s\n"
                                "%sExpected format:%s\n", INDENT, identifier, INDENT, RCM_IDENTIFIER);
    }
  } else if (!strncmp(buffer, PR_IDENTIFIER, IDENTIFIER_SIZE)) {
#ifdef FORMAT_031
    get_id_flag          = ON;
#endif
    *data_length         = ZERO;
  } else if (!strncmp(buffer, OCM_IDENTIFIER, IDENTIFIER_SIZE)) {
    *data_length         = ZERO;
  } else {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Identifier format is not correct.\n"
                              "%sActual format  :%s\n"
                              "%sExpected format:UNKNOWN\n", INDENT, identifier);
  }
  *current_position += IDENTIFIER_SIZE;

  check_dir_offset_of_header(identifier, buffer, current_position, &directory_offset);

  r64(BIG, (unsigned char *)(buffer + *current_position), &data_offset, 1);
  *current_position += DATA_OFFSET_SIZE;
  if (get_data_length_flag == ON) {
    r64(BIG, (unsigned char *)(buffer + *current_position), data_length, 1);
    *current_position += DATA_LENGTH_SIZE;
  }
  r64(BIG, (unsigned char *)(buffer + *current_position), num_of_marker, 1);
  *current_position += NUMBER_OF_PARTIAL_REFERENCE_SIZE;

  check_integrity_of_dir_offset(identifier, &directory_offset, &data_offset, num_of_marker);

  if (get_id_flag == ON) {
    uuid_unparse((unsigned char *)buffer + *current_position, system_id);
    *current_position += SYSTEM_ID_SIZE;
    uuid_unparse((unsigned char *)buffer + *current_position, pool_id);
    *current_position += POOL_ID_SIZE;
#ifndef FORMAT_031
    //memset(buffer + *current_position, 0, sizeof(pool_group_id)); //for Debug
    int is_valid_pool_group_id = !is_null_filled(buffer + *current_position, POOL_GROUP_ID_SIZE);
    if (is_valid_pool_group_id) {
      uuid_unparse((unsigned char *)buffer + *current_position, pool_group_id);
    } else {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "The format of pool group ID at RCM is not valid.\n");
    }
    *current_position += POOL_GROUP_ID_SIZE;
#endif
    if (strncmp(buffer, RCM_IDENTIFIER, IDENTIFIER_SIZE) == EQUAL) {
      *current_position += directory_offset - RCM_HEADER_SIZE;
    } else if (strncmp(buffer, PR_IDENTIFIER, IDENTIFIER_SIZE) == EQUAL) {
      *current_position += directory_offset - PR_HEADER_SIZE;
    }
    if (check_uuid_format(system_id, "System", "RCM Header") != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "The format of system ID at RCM is not correct.\n");
    }
    if (mamhta && mamhta->is_valid && strcasecmp(system_id, (char*)mamhta->Data.system_id)) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DEFAULT, "System ID in header is different from one in MAM Host-type Attributes.\n"
                                "%sSystem ID in header:  %s\n"
                                "%sSystem ID in MAM HTA: %s\n", INDENT, system_id, INDENT, mamhta ? mamhta->Data.system_id : "(null)");
    }
    if (check_uuid_format(pool_id, "Pool", "RCM Header") != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "The format of pool ID at RCM is not correct.\n");
    }
#ifndef FORMAT_031
     //memset(pool_group_id, 0, sizeof(pool_group_id)); //for Debug
     //strcpy(pool_group_id, ZERO_FILLED_UUID); //for Debug
    if (is_valid_pool_group_id) {
      if (check_optional_uuid_format(pool_group_id, "Pool Group", "RCM Header") != OK) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "The format of pool group ID at RCM is not correct.\n");
      }
      if (mamhta && mamhta->is_valid && strcmp(mamhta->Data.pool_group_id, ZERO_FILLED_UUID)
       && strcasecmp(mamhta->Data.pool_group_id, pool_group_id)) {
        ret |= output_accdg_to_vl(OUTPUT_WARNING, DEFAULT, "Pool Group ID in header is different from one in MAM Host-type Attributes.\n"
                                  "%sPool Group ID in header:  %s\n"
                                  "%sPool Group ID in MAM HTA: %s\n", INDENT, pool_group_id, INDENT, mamhta ? mamhta->Data.pool_group_id : "(null)");
      }
    }
#endif
    if (mamhta && mamhta->is_valid && strcasecmp(pool_id, mamhta->Data.pool_id)) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DEFAULT, "Pool ID in header is different from one in MAM Host-type Attributes.\n"
                                "%sPool ID in header:  %s\n"
                                "%sPool ID in MAM HTA: %s\n", INDENT, pool_id, INDENT, mamhta ? mamhta->Data.pool_id : "(null)");
    }
  }
  ret |= output_accdg_to_vl(OUTPUT_INFO, DEFAULT, "directory offset:%lu\n"
                            "%sdata offset:%lu\n"
                            "%sdata_length:%lu\n"
                            "%snum of marker:%lu\n"
                            "%ssystem id:%s\n"
                            "%spool id:%s\n", directory_offset, INDENT, data_offset, INDENT, *data_length,
                            INDENT, *num_of_marker, INDENT, system_id, INDENT, pool_id);
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "end  :clf_header: %s, number of markers=%d\n",
                            identifier, *num_of_marker);
  return ret;
}

/**
 * Check integrity of block offset.
 * @param [in]  (pre_block_offset)  Relative block number of previous marker.
 * @param [in]  (block_offset)      Relative block number of current marker.
 * @return      (OK/NG)
 */
static int check_block_offset(const uint64_t pre_block_offset, const uint64_t block_offset) {
  int ret = OK;
#ifdef FORMAT_031
  if (pre_block_offset >= block_offset) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Block offsets must be in ascending order.\n"
                              "%sPrevious block offset :%lu\n"
                              "%sPresent block offset  :%lu\n", INDENT, pre_block_offset, INDENT, block_offset);
  }
#else
  if (pre_block_offset <= block_offset) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Block offsets must be in descending order.\n"
                              "%sPrevious block offset :%lu\n"
                              "%sPresent block offset  :%lu\n", INDENT, pre_block_offset, INDENT, block_offset);
  }
#endif

  return ret;
}

/**
 * Check if the difference between block offsets of adjacent markers is bigger than the size of the marker.
 * @param [in]  (pre_block_offset)  Relative block number of previous marker.
 * @param [in]  (block_offset)      Relative block number of current marker.
 * @param [in]  (pre_length_byte)   Byte length of previous marker.
 * @return      (OK/NG)
 */
static int check_offset_diff(const uint64_t pre_block_offset, const uint64_t block_offset, const uint64_t pre_length_byte) {
  int ret = OK;

#ifdef FORMAT_031
  if ((block_offset - pre_block_offset) * LTOS_BLOCK_SIZE <= pre_length_byte) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Adjacent markers overlap.\n"
                              "%sDifference between block offsets of adjacent markers must bigger than the size of the marker.\n"
                              "%sDifference between block offsets:%lu\n"
                              "%sThe size of the marker          :%lu\n"
                              ,INDENT, INDENT, (block_offset - pre_block_offset) * LTOS_BLOCK_SIZE , INDENT, pre_length_byte);
  }
#else
  if ((pre_block_offset - block_offset) * LTOS_BLOCK_SIZE <= pre_length_byte) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Adjacent markers overlap.\n"
                              "%sDifference between block offsets of adjacent markers must bigger than the size of the marker.\n"
                              "%sDifference between block offsets:%lu\n"
                              "%sThe size of the marker          :%lu\n"
                              ,INDENT, INDENT, (pre_block_offset - block_offset) * LTOS_BLOCK_SIZE , INDENT, pre_length_byte);
  }
#endif
  return ret;
}

/**
 * Check directory format.
 * @param [in]  (identifier)        Pointer of defined identifiler of each markers.
 * @param [in]  (buffer)            Buffer pointer of markers on this tape.
 * @param [in]  (current_position)  Current position of the buffer pointer.
 * @param [in]  (num_of_marker)     Pointer of the numbers of markers.
 * @param [in]  (length)            Byte size of each markers.
 * @param [in]  (block_offset)      Relative block number of each markers.
 * @return      (OK/NG)             If the format is correct or not.
 */
int clf_directory(const char* const identifier, const char* const buffer, uint64_t* const current_position,
                  const uint64_t num_of_marker, uint64_t* const length, uint64_t* const block_offset) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "start:clf_directory: %s, number of markers=%d\n",
                               identifier, num_of_marker);

  uint64_t num_of_marker_counter = 0;
  uint64_t length_byte           = 0;
  uint64_t b_offset              = 0;
  int get_length_flag            = OFF;
  uint64_t pre_block_offset      = 0;
  uint64_t pre_length_byte       = 0;

  if ((strncmp(identifier, PR_IDENTIFIER, IDENTIFIER_SIZE) == EQUAL)
     || (strncmp(identifier, OCM_IDENTIFIER, IDENTIFIER_SIZE) == EQUAL)) {
    get_length_flag = ON;
  } else if ((strncmp(identifier, RCM_IDENTIFIER, IDENTIFIER_SIZE) == EQUAL)) {
  } else {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Unknown identifier.\n");
  }

  for (num_of_marker_counter = 0; num_of_marker_counter < num_of_marker; num_of_marker_counter++) {
    if (get_length_flag == ON) {
      r64(BIG, (unsigned char *)(buffer + *current_position), &length_byte, 1);
      length[num_of_marker_counter] = length_byte;
      *current_position            += LENGTH_DIRECTORY;
      ret |= output_accdg_to_vl(OUTPUT_DEBUG, DEFAULT, "length:%lu\n", length_byte);
    }
    r64(BIG, (unsigned char *)(buffer + *current_position), &b_offset, 1);
    block_offset[num_of_marker_counter] = b_offset;
    *current_position                  += BLOCK_OFFSET_DIRECTORY;
    ret |= output_accdg_to_vl(OUTPUT_DEBUG, DEFAULT, "block offset:%lu\n", b_offset);

    if (num_of_marker_counter) {
      check_block_offset(pre_block_offset, b_offset);
      check_offset_diff(pre_block_offset, b_offset, pre_length_byte);
    }
    pre_block_offset = b_offset;
    pre_length_byte  = length_byte;
  }

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "end  :clf_directory: %s\n", identifier);
  return ret;
}
