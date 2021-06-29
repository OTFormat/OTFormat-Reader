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
 * @file check_label.c
 * 
 * Functions to check if a Label structure complies with OTFormat.
 */

#include "ltos_format_checker.h"

#ifdef FORMAT_031
#define FIRST_LAYER_KEY_IN_LABEL    "LTOSLabel"
#define VERSION_KEY_IN_LABEL        "LTOSVersion"
#else
#define FIRST_LAYER_KEY_IN_LABEL    (IMPLEMENTATION_IDENTIFIER "Label")
#define VERSION_KEY_IN_LABEL        "Version"
#endif

#define FORMATTIME_KEY_IN_LABEL     "FormatTime"

#ifdef FORMAT_031
#define VOLUME_UUID_KEY_IN_LABEL    "VolumeUUID"
#else
#define VOLUME_UUID_KEY_IN_LABEL    "VolumeUuid"
#endif

#define CREATOR_KEY_IN_LABEL        "Creator"
#define MAX_CREATOR_LENGTH_IN_LABEL (1024)
#define COMPRESSION_KEY_IN_LABEL    "Compression"
#define BLOCKSIZE_KEY_IN_LABEL      "BlockSize"
#define MIN_BLOCKSIZE_IN_LABEL      (4096)

static int initialize_ltos_label(LTOSLabel* const ltos_label);
static void destroy_ltos_label(LTOSLabel* const ltos_label);

/**
 * Check Vol1 label format.
 * @param [in]  (which_partition)  Data Partition:0, Reference Partition:1
 * @param [in]  (verbose_level)    How much information to display.
 * @param [in]  (continue_mode)    Whether continue checking or not when a error is found.
 * @return      (OK/NG)            If the format is correct or not.
 */
int clf_vol1_label() {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_INFO, "start:clf_vol1_label\n");

  uint64_t current_position            = 0;
  char vol1_label[VOL1_LABEL_SIZE + 1] = { '\0' };

#ifndef NO_TAPE
  FILE* fp =clf_open_file(VOL1_LABEL_PATH, "rb");
  uint8_t vol1_label_buffer[VOL1_LABEL_SIZE] = { '\0' };
  const size_t read_byte = clf_read_file(vol1_label_buffer, 1, VOL1_LABEL_SIZE, fp);
  clf_check_read_data(read_byte, sizeof(vol1_label_buffer), "Vol1 Label", VOL1_LABEL_PATH);

  if (VOL1_LABEL_SIZE != read_byte) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Vol1 label size is not correct.\n"
                              "%sActual value  :%d\n"
                              "%sExpected value:%d\n", INDENT, read_byte , INDENT, VOL1_LABEL_SIZE );
  }
#else
  char vol1_label_buffer[] = "VOL1FF0012              LTOS                                                   4";
#endif

  memmove(vol1_label, vol1_label_buffer + current_position, VOL1_LABEL_SIZE);
  if (strncmp((char*)vol1_label_buffer, LABEL_IDENTIFIER, LABEL_IDENTIFIER_SIZE)) {
    char* p = str_substring((char*)vol1_label_buffer, current_position, LABEL_IDENTIFIER_SIZE);
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Label identifier format is not correct.\n"
                              "%sActual format  :%s\n"
                              "%sExpected format:%s\n", INDENT, p ? p : "", INDENT, LABEL_IDENTIFIER);
    free(p);
    p = NULL;
  }
  current_position += LABEL_IDENTIFIER_SIZE;
  if (strncmp((char*)vol1_label_buffer + current_position, LABEL_NUMBER, LABEL_NUMBER_SIZE)) {
    char* p = str_substring((char*)vol1_label_buffer, current_position, LABEL_NUMBER_SIZE);
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Label number format is not correct.\n"
                              "%sActual format  :%s\n"
                              "%sExpected format:%s\n", INDENT, p ? p : "", INDENT, LABEL_NUMBER);
    free(p);
    p = NULL;
  }
  current_position += LABEL_NUMBER_SIZE;
  for (int vol_id_offset = 0; vol_id_offset < VOLUME_IDENTIFIER_SIZE; vol_id_offset++) {
    if (!((isupper(*(vol1_label_buffer + current_position + vol_id_offset))) || (isdigit(*(vol1_label_buffer + current_position + vol_id_offset))))){
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Volume identifier format is not correct.\n"
                                "%sYou can not use character '%c'.\n", INDENT, *(vol1_label_buffer + current_position + vol_id_offset));
    }
    if (ret == NG) {
      break;
    }
  }
  current_position += VOLUME_IDENTIFIER_SIZE;
  if (strncmp((char*)vol1_label_buffer + current_position, VOLUME_ACCESSIBILITY, VOLUME_ACCESSIBILITY_SIZE)) {
    char* p = str_substring((char*)vol1_label_buffer, current_position, VOLUME_ACCESSIBILITY_SIZE);
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Volume accessibility format is not correct.\n"
                              "%sActual format  :%s\n"
                              "%sExpected format:%s\n", INDENT, p ? p : "", INDENT, VOLUME_ACCESSIBILITY);
    free(p);
    p = NULL;
  }
  current_position += VOLUME_ACCESSIBILITY_SIZE;
  if (strncmp((char*)vol1_label_buffer + current_position, RESERVED_13_SPACES, RESERVED_13_SPACES_SIZE)) {
    char* p = str_substring((char*)vol1_label_buffer, current_position, RESERVED_13_SPACES_SIZE);
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Reserved spaces format is not correct.\n"
                              "%sActual format  :%s\n"
                              "%sExpected format:%s\n", INDENT, p ? p : "", INDENT, RESERVED_13_SPACES);
    free(p);
    p = NULL;
  }  
  current_position += RESERVED_13_SPACES_SIZE;
  if (strncmp((char*)vol1_label_buffer + current_position, IMPLEMENTATION_IDENTIFIER, IMPLEMENTATION_IDENTIFIER_SIZE)) {
    char* const p = str_substring((char*)vol1_label_buffer, current_position, IMPLEMENTATION_IDENTIFIER_SIZE);
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Implementation identifier format is not correct.\n"
                              "%sActual format  :%s\n"
                              "%sExpected format:%s\n", INDENT, p ? p : "", INDENT, IMPLEMENTATION_IDENTIFIER);
    free(p);
  }
  if (strncmp((char*)vol1_label_buffer + current_position + IMPLEMENTATION_IDENTIFIER_SIZE, IMPLEMENTATION_IDENTIFIER_SPCE,
              IMPLEMENTATION_IDENTIFIER_BUF_SIZE - IMPLEMENTATION_IDENTIFIER_SIZE)) {
    char* p = str_substring((char*)vol1_label_buffer, current_position + IMPLEMENTATION_IDENTIFIER_SIZE,
                                  IMPLEMENTATION_IDENTIFIER_BUF_SIZE - IMPLEMENTATION_IDENTIFIER_SIZE);
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Implementation identifier format is not correct.\n"
                              "%sActual format  :%s\n"
                              "%sExpected format:%s\n", INDENT, p ? p : "", INDENT, IMPLEMENTATION_IDENTIFIER_SPCE);
    free(p);
    p = NULL;
  }
  current_position += IMPLEMENTATION_IDENTIFIER_BUF_SIZE;
  for (int owner_id_offset = 0; owner_id_offset < OWNER_IDENTIFIER_SIZE; owner_id_offset++) {
    if (!isprint(*(vol1_label_buffer + current_position + owner_id_offset))){
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Owner identifier format is not correct.\n"
                                "%sYou can not use character '%c'.\n", INDENT, *(vol1_label_buffer + current_position + owner_id_offset));
    }
    if (ret == NG) {
      break;
    }
  }
  current_position += OWNER_IDENTIFIER_SIZE;
  if (strncmp((char*)vol1_label_buffer + current_position, RESERVED_28_SPACES, RESERVED_28_SPACES_SIZE)) {
    char* p = str_substring((char*)vol1_label_buffer, current_position, RESERVED_28_SPACES_SIZE);
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Reserved spaces format is not correct.\n"
                              "%sActual format  :%s\n"
                              "%sExpected format:%s\n", INDENT, p ? p : "", INDENT, RESERVED_28_SPACES);
    free(p);
    p = NULL;
  }
  current_position += RESERVED_28_SPACES_SIZE;
  if (strncmp((char*)vol1_label_buffer + current_position, LABEL_STANDARD_VERSION, LABEL_STANDARD_VERSION_SIZE)) {
    char* p = str_substring((char*)vol1_label_buffer, current_position, LABEL_STANDARD_VERSION_SIZE);
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Label standard version format is not correct.\n"
                              "%sActual format  :%s\n"
                              "%sExpected format:%s\n", INDENT, p ? p : "", INDENT, LABEL_STANDARD_VERSION);
    free(p);
    p = NULL;
  }
  current_position += LABEL_STANDARD_VERSION_SIZE;

  clf_close_file(fp);
  ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_INFO, "%s\n", vol1_label);
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_INFO, "end  :clf_vol1_label\n");
  return ret;
}

/**
 * Check block size format in OTFormat label.
 * @param [in]  (str_block_size)   Block size in string.
 * @param [out] (block_size)       Block size in integer.
 * @return      (OK/NG)            If the format is correct or not.
 */
static int check_block_size(const char* const str_block_size, uint32_t* const block_size) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_INFO, "start:check_block_size\n");;
  char is_valid = TRUE;

  for (uint32_t i = 0; i < strlen(str_block_size); i++) {
    if (!isdigit(str_block_size[i])) {
      is_valid = FALSE;
      break;
    }
  }

  if (is_valid && MIN_BLOCKSIZE_IN_LABEL <= atoi(str_block_size)) {
    *block_size = atoi(str_block_size);
  } else {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO,
                              "BlockSize of OTFormat Label is not correct.\n"
                              "%sActual format  : %s\n"
                              "%sExpected format: %s\n", INDENT, str_block_size,
                              INDENT, "Integer (4096 or greater)");
  }

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_INFO, "end  :check_block_size\n");
  return ret;
}

/**
 * Check Ltos label format.
 * @param [in]  (mamvci)           Pointer of a volume coherency information.
 * @param [in]  (mamhta)           Pointer of a host-type attributes.
 * @param [in]  (verbose_level)    How much information to display.
 * @param [in]  (continue_mode)    Whether continue checking or not when a error is found.
 * @param [out] (block_size)       Block size.(default size is 1048576)
 * @return      (OK/NG)            If the format is correct or not.
 */
int clf_ltos_label(const MamVci* const mamvci, const MamHta* const mamhta, uint32_t* block_size) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_INFO, "start:clf_ltos_label\n");;

  LTOSLabel ltos_label                            = { 0 };
  json_object* jobj_from_string                   = NULL;
  json_object* json_obj_from_ltos_label           = NULL;

  if (mamvci == NULL || mamhta == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_INFO, "Null pointer is detected at clf_ltos_label");
  }
#ifndef NO_TAPE
  FILE* fp =clf_open_file(OTF_LABEL_PATH, "rb");
  uint8_t tmp_ltos_label_buffer[LTOS_BLOCK_SIZE] = { '\0' };
  const size_t read_byte = clf_read_file(tmp_ltos_label_buffer, 1, LTOS_BLOCK_SIZE, fp);

  char ltos_label_buffer[read_byte + 1];
  memmove(ltos_label_buffer, (char*)tmp_ltos_label_buffer, read_byte);
  ltos_label_buffer[read_byte] = EOS;
#else // NO_TAPE
#ifdef FORMAT_031
  char ltos_label_buffer[] = " { \n    \"LTOSLabel\": { \n        \"BlockSize\": 4096,\n        \"VendorSpecial\": \"abc\",\n        \"LTOSVersion\": \"1.0.0\",\n        \"FormatTime\": \"2018-03-01T18:35:47.866846222Z\",\n        \"Barcode\": \"AAG812L7\",\n        \"VolumeUUID\": \"c96bc83e-1790-41e8-8f35-408bcd9db5c6\",\n        \"Creator\": \"FujiFilm LTOS 0.0.1 - Linux - ltosd\",\n        \"Compression\": true\n    } \n } \n";
#else // FORMAT_031
  char ltos_label_buffer[] = " { \n    \"LTOSLabel\": { \n        \"BlockSize\": 4096,\n        \"VendorSpecial\": \"abc\",\n        \"LTOSVersion\": \"1.0.0\",\n        \"FormatTime\": \"2018-03-01T18:35:47.866846222Z\",\n        \"Barcode\": \"AAG812L7\",\n        \"VolumeUuid\": \"c96bc83e-1790-41e8-8f35-408bcd9db5c6\",\n        \"Creator\": \"FujiFilm LTOS 0.0.1 - Linux - ltosd\",\n        \"Compression\": true\n    } \n } \n";
#endif // FORMAT_031
#endif // NO_TAPE

  jobj_from_string = json_tokener_parse(ltos_label_buffer);

  initialize_ltos_label(&ltos_label);

  json_object_object_foreach(jobj_from_string, key, val) {
    output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_INFO, "%s\n", key);
    if (strcmp(key, FIRST_LAYER_KEY_IN_LABEL)) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "First layer key of OTFormat Label is invalid.\n"
                                "%sActual format  :%s\n"
                                "%sExpected format:%s\n", INDENT, key , INDENT, FIRST_LAYER_KEY_IN_LABEL);
    }
    output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_INFO, "%s\n", json_object_to_json_string(val));

    json_obj_from_ltos_label = json_tokener_parse(json_object_to_json_string(val));
    json_object_object_foreach(json_obj_from_ltos_label, ltos_label_key, ltos_label_val) {
      if (strcmp(ltos_label_key, VERSION_KEY_IN_LABEL) == 0) {
        ltos_label.ltos_version = (char*)clf_allocate_memory(strlen(json_object_get_string(ltos_label_val)) + 1,
                                                             "Version in OTFormat Label");
        strcpy(ltos_label.ltos_version, json_object_get_string(ltos_label_val));
        ltos_label.exists_ltos_version = TRUE;
        if (strcmp(ltos_label.ltos_version, VERSION_IN_LABEL)) {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Version format is not correct.\n"
                                    "%sActual format  :%s\n"
                                    "%sExpected format:%s\n", INDENT, ltos_label.ltos_version, INDENT, VERSION_IN_LABEL);
         }
        // compare with MAM Host-type attribute
        if (mamhta->is_valid && strncmp(ltos_label.ltos_version, mamhta->Data.application_version,
                                        MIN(strlen(ltos_label.ltos_version), MAM_HTA_VERSION_SIZE))) {
          ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_HEADER_INFO,
                                    "Version in OTFormat Label is different from Application version in MAM Host-type Attributes.\n"
                                    "%sVersion in the Label:  %s\n"
                                    "%sApplication version in MAM HTA: %s\n",
                                    INDENT, ltos_label.ltos_version, INDENT, mamhta->Data.application_version);
         }
        // padding check
        if (mamhta->is_valid) {
          for (int i = strlen(ltos_label.ltos_version); i < MAM_HTA_VERSION_SIZE; i++) {
            if (mamhta->Data.application_version[i] != ' ') {
              ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_HEADER_INFO,
                                        "Application version \"%s\" should be padded by space from offset %d and later.\n",
                                        mamhta->Data.application_version, strlen(ltos_label.ltos_version));
              break;
            }
           }
         }
      } else if (strcmp(ltos_label_key, FORMATTIME_KEY_IN_LABEL) == 0) {
        ltos_label.format_time = (char*)clf_allocate_memory(strlen(json_object_get_string(ltos_label_val)) + 1,
                                                            "Format Time in OTFormat Label");
        strcpy(ltos_label.format_time, json_object_get_string(ltos_label_val));

        // Check "FormatTime" format.
        if (check_utc_format(ltos_label.format_time) != OK) {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "UTC format is not correct.\n"
                                    "%sActual format  :%s\n"
                                    "%sExpected format:YYYY-MM-DDThh:mm:ss.mmmmmmZ\n", INDENT, ltos_label.format_time, INDENT);
         }
        ltos_label.exists_format_time  = TRUE;
      } else if (strcmp(ltos_label_key, VOLUME_UUID_KEY_IN_LABEL) == 0) {
        strncpy(ltos_label.volume_uuid, json_object_get_string(ltos_label_val), UUID_SIZE);
        if (check_uuid_format(ltos_label.volume_uuid, "Volume", FIRST_LAYER_KEY_IN_LABEL) != OK) {
#ifdef FORMAT_031
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "VolumeUUID of OTFormat Label is not correct.\n"
#else
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "VolumeUuid of OTFormat Label is not correct.\n"
#endif
                                    "%sActual format  :%s\n", INDENT, ltos_label.volume_uuid);
        }
        if (mamvci->is_valid && strcasecmp(ltos_label.volume_uuid, mamvci->Data.uuid) != 0) {
#ifdef FORMAT_031
          ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_HEADER_INFO, "VolumeUUID of OTFormat Label is not same as the MAM data.\n"
#else
          ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_HEADER_INFO, "VolumeUuid of OTFormat Label is not same as the MAM data.\n"
#endif
                                    "%sActual format(Label) :%s\n"
                                    "%sActual format(MAM)   :%s\n", INDENT, ltos_label.volume_uuid, INDENT, mamvci->Data.uuid);
        }
        ltos_label.exists_volume_uuid  = TRUE;
      } else if (strcmp(ltos_label_key, CREATOR_KEY_IN_LABEL) == 0) {
        if (MAX_CREATOR_LENGTH_IN_LABEL < strlen(json_object_get_string(ltos_label_val))) {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO,
                                    "Creator of OTFormat Label whose length is longer than 1024 bytes.\n"
                                    "%sCreator of OTFormat Label: %s\n", INDENT, ltos_label.creator);
        }
        ltos_label.creator = (char*)clf_allocate_memory(strlen(json_object_get_string(ltos_label_val)) + 1,
                                                        "Creator in OTFormat Label");
        strcpy(ltos_label.creator, json_object_get_string(ltos_label_val));
        ltos_label.exists_creator      = TRUE;

        char ff_creator[STR_MAX] = { 0 };
        sprintf(ff_creator, "FUJIFILM SDT System %s", VERSION_IN_CREATOR);
// comment out for tapes other than those made by Fujifilm.
//        if (strncmp(ff_creator, ltos_label.creator, strlen(ff_creator))) {
//          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO,
//                                    "Creator of OTFormat Label is wrong.\n"
//                                    "%sActual format:   %s\n"
//                                    "%sExpected format: %s...\n", INDENT, ltos_label.creator, INDENT, ff_creator);
//        }

        if (mamhta->is_valid && strncmp(ltos_label.creator, mamhta->Data.application_vendor, MIN(strlen(ltos_label.creator), MAM_HTA_VENDOR_SIZE))) {
          ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_HEADER_INFO,
                                    "Creator of OTFormat Label is different from APPLICATION VENDOR in MAM Host-type Attributes.\n"
                                    "%sCreator of OTFormat Label: %s\n"
                                    "%sAPPLICATION VENDOR in MAM: %s\n", INDENT, ltos_label.creator, INDENT, mamhta->Data.application_vendor);
        }
      } else if (strcmp(ltos_label_key, COMPRESSION_KEY_IN_LABEL) == 0) {
        // Check "Compression" format.
        if (json_object_get_type(ltos_label_val) != json_type_boolean) {
            ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO,
                                    "Compression of OTFormat Label is not correct.\n"
                                    "%sExpected format:%s\n", INDENT, "Boolean (true or false)");
        }
      } else if (strcmp(ltos_label_key, BLOCKSIZE_KEY_IN_LABEL) == 0) {
        char* str_block_size = (char*)clf_allocate_memory(strlen(json_object_get_string(ltos_label_val)) + 1,
                                                          "BlockSize in OTFormat Label");
        strcpy(str_block_size, json_object_get_string(ltos_label_val));
        check_block_size(str_block_size, block_size);
        free(str_block_size);
        str_block_size = NULL;
      } else if (strncmp(ltos_label_key, "Vendor", 6) == OK) {
        ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_INFO, "Vendor defined KEY and VALUE are found.\n"
                                    "%sKEY    : %s\n"
                                    "%sVALUE  : %s\n", INDENT, ltos_label_key, INDENT, json_object_get_string(ltos_label_val));
        // Check if VendorDefinedKey contains only digits and alphabets.
        // ([TODO] it follows UpperCamelCase if needed.
        for (uint32_t offset = 0; offset < strlen(ltos_label_key); offset++) {
          const char key_ascii = *(ltos_label_key + offset);
          if (!isdigit(key_ascii) && !isalpha(key_ascii)) {
            ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO,
                                    "Vendor defined KEY of OTFormat Label is including an invalid character(s).\n"
                                    "%sActual format  : 0x%02X\n"
                                    "%sExpected format: %s\n", INDENT, key_ascii, INDENT, "Digits or Alphabets");
            break;
           }
         }
      } else { // Undefined Key
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Undefined KEY and VALUE are found.\n"
                                    "%sKEY    : %s\n"
                                    "%sVALUE  : %s\n", INDENT, ltos_label_key, INDENT, json_object_get_string(ltos_label_val));
      } // end of if (strcmp(ltos_label_key, VERSION_KEY_IN_LABEL) == 0)
    } // end of 2nd "json_object_object_foreach"
    json_object_put(json_obj_from_ltos_label);
    json_obj_from_ltos_label = NULL;
  } // end of 1st "json_object_object_foreach"
  json_object_put(jobj_from_string);
  jobj_from_string = NULL;
  if ((ltos_label.exists_creator      == FALSE) ||
      (ltos_label.exists_format_time  == FALSE) ||
      (ltos_label.exists_ltos_version == FALSE) ||
      (ltos_label.exists_volume_uuid  == FALSE)) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "At least one of required elements is not described in OTFormat Label.\n");
  }
  destroy_ltos_label(&ltos_label);
  clf_close_file(fp);
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_INFO, "end  :clf_ltos_label\n");;
  return ret;
}

/**
 * Initialize "LTOSLabel" structure.
 * @param [in]  (ltos_label) Pointer of a ltos label.
 * @return      (OK/NG)      If the structure is initialized or not.
 */
static int initialize_ltos_label(LTOSLabel* const ltos_label) {
  int ret = OK;
  memset(ltos_label, 0, sizeof(LTOSLabel));
  ltos_label->ltos_version        = NULL; // there is no way to know the size to allocate for LTOS version.
  ltos_label->format_time         = NULL; // there is no way to know the size to allocate for Format Time.
  ltos_label->creator             = NULL; // there is no reason to allocate fixed size for Creator.
  ltos_label->exists_creator      = FALSE;
  ltos_label->exists_format_time  = FALSE;
  ltos_label->exists_ltos_version = FALSE;
  ltos_label->exists_volume_uuid  = FALSE;
  memset(ltos_label->volume_uuid, EOS, sizeof(ltos_label->volume_uuid));
  return ret;
}

/**
 * Destroy "LTOSLabel" structure.
 * @param [in]  (ltos_label) Pointer of a ltos label.
 */
static void destroy_ltos_label(LTOSLabel* const ltos_label) {
  if (ltos_label) {
    free(ltos_label->ltos_version);
    ltos_label->creator      = NULL;
    free(ltos_label->format_time);
    ltos_label->format_time  = NULL;
    free(ltos_label->creator);
    ltos_label->ltos_version = NULL;
  }
}
