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
 * @file check_marker_l4_rcm.c
 * 
 * Functions to check if a Reference Commit Marker (RCM) complies with OTFormat.
 */

#include "ltos_format_checker.h"

static int clf_rcm_bucket_list(const char* const reference_commit_marker_buffer,
                               const uint64_t* const current_position, const uint64_t rcm_data_length);

/**
 * Check number of partial reference format of header.
 * @param [in]  (which_rcm)                       Which rcm to be checked.(FIRST/LAST)
 * @param [in]  (rcm_number_of_partial_reference) Pointer of a total number of partial reference in this tape.
 * @return      (OK/NG)
 */
static int check_num_of_pr(const int which_rcm, const uint64_t* const rcm_number_of_partial_reference) {
  int ret = OK;
  if ((which_rcm != FIRST && which_rcm != LAST) || rcm_number_of_partial_reference == NULL) {
    ret = output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT,
                             "Invalid arguments at check_num_of_pr: which_rcm = %d, rcm_number_of_partial_reference = %p\n",
                             which_rcm, rcm_number_of_partial_reference);
  }

  if (which_rcm == FIRST) {
    if (*rcm_number_of_partial_reference != ZERO) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Number of partial reference of first rcm is not correct.\n"
                                "%sActual value  :%lu\n"
                                "%sExpected value:%d\n", INDENT, *rcm_number_of_partial_reference, INDENT, ZERO);
    }
  }
  return ret;
}

/**
 * Check if data length defined at rcm header is the same as actual size of bucket list.
 * @param [in]  (which_rcm)        Which rcm to be checked.(FIRST/LAST)
 * @param [in]  (rcm_data_length)  Byte size of a bucket list.(defined at rcm header)
 * @param [in]  (current_position) Current position of rcm buffer pointer.(just before reading bucket list)
 * @param [in]  (dat_size)         Byte size of rcm.
 * @return      (OK/NG)
 */
static int check_data_length_of_bucket_list(const int which_rcm, const uint64_t rcm_data_length,
                                            const uint64_t current_position, const uint64_t dat_size) {
  int ret = OK;
  if (which_rcm != FIRST && which_rcm != LAST) {
    ret = output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT,
                             "Invalid arguments at check_data_length_of_bucket_list: which_rcm = %d\n", which_rcm);
  }

  if (rcm_data_length != (dat_size - current_position)) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Data length defined at rcm header is not the same as actual size of System info.\n"
                              "%sActual size(data length) :%ld\n"
                              "%sActual size(bucket list) :%ld\n", INDENT, rcm_data_length, INDENT, dat_size - current_position);
  }
  return ret;
}

/**
 * Get RCM name
 * @param [in]  (rcm) RCM id
 * @return      ("First"/"Last"/"Unknown")
 */
static char* get_rcm_name(const int rcm) {
  return rcm == FIRST ? "First" : (rcm == LAST ? "Last" : "Unknown");
}

/**
 * Check reference commit marker format.
 * @param [in]  (which_rcm)                       Which rcm to be checked.(FIRST/LAST)
 * @param [in]  (mamvci)                          Pointer of a volume coherency information. 
 * @param [in]  (mamhta)                          Pointer of a host-type attributes.
 * @param [in]  (pr_number)                          Number of partial reference.
 * @return      (OK/NG)                           If the format is correct or not.
 */
int clf_reference_commit_marker(int which_rcm, const MamVci* const mamvci, const MamHta* const mamhta, const uint64_t pr_number) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:clf_reference_commit_marker: Level 4 (%s)\n",
                               get_rcm_name(which_rcm));
  set_top_verbose(DISPLAY_HEADER_AND_L4_INFO);
  uint64_t current_position                = 0;
  uint64_t rcm_data_length                 = 0;
  uint64_t length_byte                     = 0; // dont't have meanings in case of rcm
  uint64_t rcm_number_of_partial_reference = 0;

  if (mamvci == NULL || mamhta == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at clf_reference_commit_marker");
  }
  uint64_t* rcm_pr_blocks = clf_allocate_memory(sizeof(uint64_t) * pr_number, "rcm_pr_blocks");
#ifndef NO_TAPE
  char rcm_file_path_prefix[100] = FILE_PATH SEPARATOR RCM_FILE_PREFIX;
  char rcm_file_path[100]        = { 0 };
  sprintf(rcm_file_path, "%s%d", rcm_file_path_prefix, which_rcm);
  FILE* fp =clf_open_file(rcm_file_path, "rb");
  uint8_t reference_commit_marker_buffer[LTOS_BLOCK_SIZE] = { '\0' };
  const size_t read_byte = clf_read_file(reference_commit_marker_buffer, 1, LTOS_BLOCK_SIZE, fp);
#else
  ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4_INFO, "Can't check reference commit marker without tape.\n");
  return ret;
#endif
  /* Check RCM Header */
  if (clf_header(RCM_IDENTIFIER, (char*)reference_commit_marker_buffer, mamhta, OFF, &current_position,
                 &rcm_number_of_partial_reference, &rcm_data_length) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Reference commit marker header format is not correct.\n");
  }
  if (which_rcm == LAST && mamvci->is_valid && mamvci->Data.pr_count != rcm_number_of_partial_reference) {
    ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_HEADER_AND_L4_INFO,
                              "Number of Partial Reference is inconsistent. RCM: %d, MAM: %d\n",
                              rcm_number_of_partial_reference, mamvci->Data.pr_count);
  }
  ret |= check_num_of_pr(which_rcm, &rcm_number_of_partial_reference);

  /* Read partial reference directory. */
  if (clf_directory(RCM_IDENTIFIER, (char*)reference_commit_marker_buffer, &current_position,
                    rcm_number_of_partial_reference, &length_byte, rcm_pr_blocks) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Partial reference directory format is not correct.\n");
  }

  ret |= check_data_length_of_bucket_list(which_rcm, rcm_data_length, current_position, read_byte);

  /* Check bucket list. */
  if (clf_rcm_bucket_list((char*)reference_commit_marker_buffer, &current_position, rcm_data_length) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Bucket list format is not correct.\n");
  }

  set_top_verbose(DEFAULT);
  if (ret == OK) {
    ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4_INFO, "[SUCCESS] Reference commit marker is correct.\n");
  }
  clf_close_file(fp);
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :clf_reference_commit_marker: Level 4 (%s)\n",
                            get_rcm_name(which_rcm));
  free(rcm_pr_blocks);
  rcm_pr_blocks = NULL;
  return ret;
}

/**
 * Check if the bucket name is formated as an IP address.
 * @param [in]  (bucket_name) Target to check.
 * @return      (TRUE/FALSE)  If the bucket name is formated as an IP address, return TRUE. Otherwise, return FALSE.
 */
static int is_ip_address_format(const char* const bucket_name) {
  int ret = TRUE;

  // copy bucket_name to buffer because strtok break input value.
  const int bucket_name_len = strlen(bucket_name);
  char* buffer = clf_allocate_memory(bucket_name_len + 1, "bucket name");
  strcpy(buffer, bucket_name);

  char* octet = strtok(buffer, ".");
  static const int number_of_octet = 4;
  for (int i = 1; i < number_of_octet; i++) { // because strtok with buffer is called just before this loop, loop counter starts with 1 instead of 0.
    if (octet == NULL) { // This means that the number of octet in bucket_name is less than 4.
      ret = FALSE;
      break;
    }

    static const uint32_t max_octet_length = 3;
    if (max_octet_length < strlen(octet)) { // The octet is too long. This check is required to avoid overflow issue at atoi.
      ret = FALSE;
      break;
    }

    for (uint32_t j = 0; j < strlen(octet); j++) {
      if (!isdigit(*(octet + j))) { // The octet contains invalid character.
        ret = FALSE;
        break;
      }
    }
    if (ret == FALSE) { // gimmick to break nested loop.
      break;
    }

    const int octet_value = atoi(octet);
    static const int minimum_octet = 0;
    static const int maximum_octet = 255;
    if (octet_value < minimum_octet || maximum_octet < octet_value) {
      ret = FALSE;
      break;
    }

    octet = strtok(NULL, ".");
  }

  if (strtok(NULL, ".")) { // This means that the number of octet in bucket_name is more than 4.
    ret = FALSE;
  }

  free(buffer);
  buffer = NULL;
  return ret;
}

/**
 * Check bucket name format.
 * @param [in]  (bucket_name) Bucket name.
 * @return      (OK/NG)
 */
int check_bucket_name(const char* const bucket_name) {
  int ret             = OK;
  int bucket_name_len = strlen(bucket_name);

  // Bucket names must be at least 3 and no more than 63 characters long.
  if (bucket_name_len < BUCKET_LIST_BUCKETNAME_MIN_SIZE || BUCKET_LIST_BUCKETNAME_MAX_SIZE < bucket_name_len) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Word count of bucket name is not correct.\n"
                              "%sActual value  :%d\n"
                              "%sExpected value:%d~%d\n", INDENT, bucket_name_len,
                              INDENT, BUCKET_LIST_BUCKETNAME_MIN_SIZE, BUCKET_LIST_BUCKETNAME_MAX_SIZE);
  }

  // Bucket name must start with a lower-case letter or a number.
  if (!islower(bucket_name[0]) && !isdigit(bucket_name[0])) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Bucket name format is not correct.\n"
                              "%sBucket name must start with lowercase alphanumeric.\n"
                              "%sActual value  :%c\n", INDENT, INDENT, *bucket_name);
  }
  // Bucket name must end with a lower-case letter or a number.
  if (!islower(bucket_name[bucket_name_len - 1]) && !isdigit(bucket_name[bucket_name_len - 1])) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Bucket name format is not correct.\n"
                              "%sBucket name must end with lowercase alphanumeric.\n"
                              "%sActual value  :%c\n", INDENT, INDENT, *(bucket_name + bucket_name_len - 1));
  }

  // Bucket name cannot have consecutive periods.
  if (strstr(bucket_name, "..")) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Bucket name format is not correct.\n"
                              "%sBucket name cannot have consecutive periods.\n"
                              "%sActual name   :%s\n", INDENT, INDENT, bucket_name);
  }

  // Bucket name cannot use dashes adjacent to periods.
  if (strstr(bucket_name, ".-") || strstr(bucket_name, "-.")) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Bucket name format is not correct.\n"
                              "%sBucket name cannot use dashes adjacent to periods.\n"
                              "%sActual name   :%s\n", INDENT, INDENT, bucket_name);
  }

  // Bucket name can contain only lower-case letters, numbers, periods and dashes.
  for (int bucket_name_offset = 0; bucket_name_offset < bucket_name_len; bucket_name_offset++) {
    const char bucket_name_ascii = *(bucket_name + bucket_name_offset);
    if (!((islower(bucket_name_ascii)) || (isdigit(bucket_name_ascii)) || (bucket_name_ascii == HYPHEN_ASCII) || (bucket_name_ascii == DOT_ASCII))) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Bucket name format is not correct.\n"
                                "%sBucket name must consist of lowercase alphanumeric, hyphen or dot.\n"
                                "%sYou can not use '%c'.\n", INDENT, INDENT, bucket_name_ascii);
    }
  }

  // Bucket name cannot be formatted as an IP address.
  if (is_ip_address_format(bucket_name)) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Bucket name format is not correct.\n"
                              "%sBucket name cannot be formatted as an IP address.\n"
                              "%sActual name   :%s\n", INDENT, INDENT, bucket_name);
  }

  return ret;
}

/**
 * Check pool group name format.
 * @param [in]  (pool_group_name) pool group name
 * @return      (OK/NG)           If the format is correct or not.
 */
static int check_pool_group_name(const char* const pool_group_name) {
  int ret             = OK;
  int pool_group_name_len = strlen(pool_group_name);

  // A pool group name consists of 1 or more characters but not exceeding 63 characters.
  if (pool_group_name_len < SYSTEMINFO_POOLGROUPNAME_MIN_SIZE || SYSTEMINFO_POOLGROUPNAME_MAX_SIZE < pool_group_name_len) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Word count of pool group name is not correct.\n"
                              "%sActual value  :%d\n"
                              "%sExpected value:%d~%d\n", INDENT, pool_group_name_len,
                              INDENT, SYSTEMINFO_POOLGROUPNAME_MIN_SIZE, SYSTEMINFO_POOLGROUPNAME_MAX_SIZE);
  }
  // The beginning of a pool group name is limited to alphabets (numbers and hyphens cannot be used).
  if (!isalpha(pool_group_name[0])) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Pool group name format is not correct.\n"
                              "%sPool group name must start with lowercase alphabets.\n"
                              "%sActual value  :%c\n", INDENT, INDENT, *pool_group_name);
  }
  // The end of a pool group name is limited to alphabets and numbers (hyphens cannot be used).
  if (!isalnum(pool_group_name[pool_group_name_len - 1])) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Pool group name format is not correct.\n"
                              "%sPool group name must end with alphabets or numbers.\n"
                              "%sActual value  :%c\n", INDENT, INDENT, *(pool_group_name + pool_group_name_len - 1));
  }
  // The types of permitted characters are lower-case alphabets, upper-case alphabets, numbers and hyphens.
  for (int pool_group_name_offset = 0; pool_group_name_offset < pool_group_name_len; pool_group_name_offset++) {
    const char pool_group_name_ascii = *(pool_group_name + pool_group_name_offset);
    if (!(isalnum(pool_group_name_ascii) || pool_group_name_ascii == HYPHEN_ASCII)) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Pool group name format is not correct.\n"
                                "%sPool group name must consist of lower-case alphabets, upper-case alphabets, numbers or hyphens.\n"
                                "%sYou can not use '%c'.\n", INDENT, INDENT, pool_group_name_ascii);
    }
  }

  return ret;
}

/**
 * Check bucket list format.
 * @param [in]  (reference_commit_marker_buffer) Buffer pointer of a rcm marker.
 * @param [in]  (current_position)               Current position of rcm buffer pointer.
 * @param [in]  (rcm_data_length)                Byte size of a bucket list.
 * @return      (OK/NG)                          If the format is correct or not.
 */
static int clf_rcm_bucket_list(const char* const reference_commit_marker_buffer,
                               const uint64_t* const current_position, const uint64_t rcm_data_length) {
  int ret                                = OK;
  char* rcm_bucket_list                  = (char*)clf_allocate_memory(rcm_data_length + 1, "bucket list");
  char* rcm_bucket_list_reformed         = (char*)clf_allocate_memory(rcm_data_length + 1, "bucket list");
  char* char_obj_from_rcm_reformed       = NULL;
  char* char_obj_from_rcm                = NULL;
  char* bucket_info_reformed             = NULL;
  json_object* json_obj_from_string      = NULL;
  json_object* json_obj_from_bucket_list = NULL;
  json_object* obj_bucket                = NULL;
  json_object* json_obj_bucket_info      = NULL;
  int bucket_count                       = 0;
  int bucket_num                         = 0;
  BucketList bucket_list                 = {OFF, OFF};

  if (rcm_data_length == 0) {
    ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4_INFO, "Bucket list is empty.\n");
  } else {
    memmove(rcm_bucket_list, reference_commit_marker_buffer + *current_position, rcm_data_length);

    str_replace(rcm_bucket_list, DOUBLE_QUART, SINGLE_QUART, rcm_bucket_list_reformed);
    json_obj_from_string = json_tokener_parse(rcm_bucket_list_reformed);

    json_object_object_foreach(json_obj_from_string, key, val) {
      ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4_INFO, "%s: %s\n", key, json_object_to_json_string(val));
      const size_t char_obj_from_rcm_length = strlen(json_object_to_json_string(val)) + 1;
      char_obj_from_rcm = (char*)clf_allocate_memory(char_obj_from_rcm_length + 1, "a JSON value");
      snprintf(char_obj_from_rcm, char_obj_from_rcm_length, "%s", json_object_to_json_string(val));
      char_obj_from_rcm_reformed = (char*)clf_allocate_memory(char_obj_from_rcm_length + 1, "a reformed JSON value");
      str_replace(char_obj_from_rcm, DOUBLE_QUART, SINGLE_QUART, char_obj_from_rcm_reformed);
      json_obj_from_bucket_list = json_tokener_parse(char_obj_from_rcm_reformed);

      if (strcmp(key, "PoolGroupName") == 0) {
        ret |= check_pool_group_name(json_object_get_string(val));
      }

      if (json_object_get_type(json_obj_from_bucket_list) == json_type_array) {
        bucket_num = json_object_array_length(json_obj_from_bucket_list);

        for (bucket_count = 0; bucket_count < bucket_num; bucket_count++) {
          bucket_list.bucket_id_flag = OFF;
          bucket_list.bucket_name_flag = OFF;
          obj_bucket = json_object_array_get_idx(json_obj_from_bucket_list, bucket_count);
          snprintf(char_obj_from_rcm, UNPACKED_OBJ_PATH_SIZE + 1, "%s", json_object_get_string(obj_bucket));
          if (json_object_get_type(obj_bucket) == json_type_object) {
            bucket_info_reformed = clf_allocate_memory(strlen(json_object_get_string(obj_bucket)) + 1,
                                                       "bucket information");
            str_replace(json_object_get_string(obj_bucket), DOUBLE_QUART, SINGLE_QUART, bucket_info_reformed);
            json_obj_bucket_info = json_tokener_parse(bucket_info_reformed);
            free(bucket_info_reformed);
            bucket_info_reformed = NULL;
            json_object_object_foreach(json_obj_bucket_info, key, val) {
              ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4_INFO, "%s: %s\n", key, json_object_to_json_string(val));
              if (strcmp(key,"BucketID") == 0) {
                bucket_list.bucket_id_flag = ON;
                ret |= check_uuid_format(json_object_get_string(val), "Bucket", "Reference Commit Marker Bucket List");
#ifdef FORMAT_031
              } else if (strcmp(key,"Bucket") == 0) {
#else
              } else if (strcmp(key,"BucketName") == 0) {
#endif
                bucket_list.bucket_name_flag = ON;
                ret |= check_bucket_name(json_object_get_string(val));
              } else if (strncmp(key,"Vendor", 6)) {
                ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Undefined key :%s\n",key);
                }
            }
            json_object_put(json_obj_bucket_info);
            json_obj_bucket_info = NULL;
          }
          if (rcm_data_length != 0) {
            if ((bucket_list.bucket_id_flag  == OFF) ||
              (bucket_list.bucket_name_flag  == OFF)) {
              ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Required elements of the bucket list are missing.\n");
            }
          }
        }
      }
      free(char_obj_from_rcm);
      char_obj_from_rcm          = NULL;
      free(char_obj_from_rcm_reformed);
      char_obj_from_rcm_reformed = NULL;
      json_object_put(json_obj_from_bucket_list);
      json_obj_from_bucket_list  = NULL;
    }
    json_object_put(json_obj_from_string);
    json_obj_from_string = NULL;
  }

  free(rcm_bucket_list);
  rcm_bucket_list            = NULL;
  free(rcm_bucket_list_reformed);
  rcm_bucket_list_reformed   = NULL;
  free(char_obj_from_rcm);
  char_obj_from_rcm          = NULL;
  free(char_obj_from_rcm_reformed);
  char_obj_from_rcm_reformed = NULL;
  free(bucket_info_reformed);
  bucket_info_reformed       = NULL;

  return ret;
}
