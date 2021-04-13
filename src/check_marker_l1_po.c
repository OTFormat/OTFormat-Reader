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
 * @file check_marker_l1_po.c
 * 
 * Functions to check if a Packed Object complies with OTFormat.
 */

#include "ltos_format_checker.h"

/* Move to an appropriate header if needed            */
/* Naming rule: ERR_MSG_ + OBJECT_NAME + _ERR_CONTENT */
#define ERR_MSG_UUID_FORMAT                  "Complying with UUID v4 format such as RRRRRRRR-RRRR-4RRR-rRRR-RRRRRRRRRRRR."
#define ERR_MSG_NUM_OF_OBJ_NEGATIVE_VALUE    "Objects number should be greater than 0."
#define ERR_MSG_OBJ_OFFSET_RELATIVE_VALUE    "Object Data Offset should be greater than Meta Data Offset."
#define ERR_MSG_META_OFFSET_RELATIVE_VALUE   "Meta Data Offset should be equal or greater than Object Data Offset."
#define ERR_MSG_OFFSET_ORDER                 "Any Offset should increase in the order."
#define ERR_MSG_OFFSET_IN_LAST_OBJ_DIR       "Object Data Offset = Meta Data Offset = Last Data Offset."
#define MAX_KEY_LENGTH_IN_META               (1024)
#ifdef FORMAT_031
#define ERR_MSG_PO_SIZE_OVER_LIMIT           "Packed Object size should be <= 10 GiB."
#define MAX_PO_SIZE                          (10UL * 1024 * 1024 * 1024)              // 10 GiB
#else
#define ERR_MSG_PO_SIZE_OVER_LIMIT           "Packed Object size should be <= 5 TiB."
#define MAX_PO_SIZE                          (5UL * 1024 * 1024 * 1024 * 1024)        // 5 TiB
#endif

#ifdef OBSOLETE
/**
 * Check if po identifier is correct.
 * @param [in]  (data_buf)         Pointer of a packed object.
 * @param [out] (current_position) Current position of a pointer.
 * @return      (OK/NG)            If a po identifier format is correct or not.
 */
static int cpof_po_identifier(unsigned char* const data_buf, int* const current_position) {
  char po_identifier[PO_IDENTIFIER_SIZE + 1] = { '\0' };
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "start:cpof_po_identifier\n");

  memcpy(po_identifier, data_buf + *current_position, PO_IDENTIFIER_SIZE);
  *current_position += PO_IDENTIFIER_SIZE;

  if (strncmp(po_identifier, PO_IDENTIFIER_ASCII_CODE , PO_IDENTIFIER_SIZE)) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                              "PO Identifier is not correct.\n"
                              "%sActual value  : %s\n"
                              "%sExpected value: %s\n", INDENT, po_identifier, INDENT, PO_IDENTIFIER_ASCII_CODE);
  } else {
    ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4321_INFO,
                              "PO Identifier is correct.\n"
                              "%sPO Identifier: %s\n", INDENT, po_identifier);
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "end  :cpof_po_identifier\n");
  return ret;
}
#endif

/**
 * Check if po header is correct.
 * @param [in]  (data_buf)         Pointer of a packed object.
 * @param [out] (current_position) Current position of a pointer.
 * @param [out] (out_num_of_obj)   Number of objects on the tape.
 * @return      (OK/NG)            If a PO Header format is correct or not.
 */
static int cpof_po_header(unsigned char* const data_buf, uint64_t* const current_position,
                          uint64_t* const out_num_of_obj, uint64_t* const po_header_length) {
  uint64_t directoryoffset                    = 0;
  uint64_t dataoffset                         = 0;
  uint64_t num_of_obj                         = 0;
  uint32_t header_offset                      = 0;
  int ret                                     = output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "start:cpof_po_header\n");
  char pack_id[UUID_SIZE + 1]                 = { '\0' };
  char bucket_id[UUID_SIZE + 1]               = { '\0' };
#ifdef FORMAT_031
  static const uint64_t po_header_size        = DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE + NUMBER_OF_OBJECTS_SIZE +
                                                PACK_ID_SIZE + BUCKET_ID_SIZE; // 56 bytes
#else
  char system_id[UUID_SIZE + 1]               = { '\0' };
  static const uint64_t po_header_size        = DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE + NUMBER_OF_OBJECTS_SIZE +
                                                PACK_ID_SIZE + BUCKET_ID_SIZE + SYSTEM_ID_SIZE; // 72 bytes
#endif
  static const uint64_t object_directory_size = OBJECT_ID_SIZE + META_DATA_OFFSET_SIZE + OBJECT_DATA_OFFSET_SIZE;

  uint8_t* po_header                          = (uint8_t*)clf_allocate_memory(po_header_size, "PO Header");
  memmove(po_header, data_buf + *current_position, po_header_size);
  *current_position += po_header_size;

  /* Read Directory offset. */
  r64(BIG, po_header + header_offset, &directoryoffset, 1);
  *po_header_length = directoryoffset;
  header_offset += DIRECTORY_OFFSET_SIZE;
  /* Read Data offset. */
  r64(BIG, po_header + header_offset, &dataoffset, 1);
  header_offset += DATA_OFFSET_SIZE;
  /* Read Number of Objects. */
  r64(BIG, po_header + header_offset, &num_of_obj, 1);
  *out_num_of_obj = num_of_obj;
  header_offset += NUMBER_OF_OBJECTS_SIZE;
  /* Read Pack ID. */
  uuid_unparse(po_header + header_offset, pack_id);
  header_offset += PACK_ID_SIZE;
  /* Read Bucket ID. */
  uuid_unparse(po_header + header_offset, bucket_id);
  header_offset += BUCKET_ID_SIZE;
#ifndef FORMAT_031
  /* Read System ID. */
  uuid_unparse(data_buf + *current_position, system_id);
  header_offset += SYSTEM_ID_SIZE;
#endif
  free(po_header);
  po_header = NULL;

  /* Check PO Header. */
  if (directoryoffset != po_header_size) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                              "Data length of Directory Offset is not correct.\n"
                              "%sActual value  : %lu\n"
                              "%sExpected value: %lu\n", INDENT, directoryoffset, INDENT, po_header_size);
  } else if (dataoffset != (((num_of_obj + 1) * object_directory_size) + po_header_size)) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                              "Length of Data Offset is not correct.\n"
                              "%sActual value  : %lu\n"
                              "%sExpected value: Length of Data Offset is corresponding to (PO Header + Objects Directory)\n"
                              "%s              : where, PO Header should be %lu bytes,\n"
                              "%s              : Objects Directory should be (Number of Objects + 1) * %lu bytes\n",
                              INDENT, dataoffset, INDENT, INDENT, po_header_size, INDENT, object_directory_size);
  } else if (num_of_obj == 0) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                              "Number of Objects is not correct.\n"
                              "%sActual value  : %lu\n"
                              "%sExpected value: %s\n", INDENT, num_of_obj, INDENT, ERR_MSG_NUM_OF_OBJ_NEGATIVE_VALUE);
  } else if (check_uuid_format(pack_id, "Packed Object", "PO Header") != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                              "Pack ID is not correct.\n"
                              "%sActual value  : %s\n"
                              "%sExpected value: %s\n", INDENT, pack_id, INDENT, ERR_MSG_UUID_FORMAT);
  } else if (check_uuid_format(bucket_id, "Bucket", "PO Header") != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                              "Bucket ID is not correct.\n"
                              "%sActual value  : %s\n"
                              "%sExpected value: %s\n", INDENT, bucket_id, INDENT, ERR_MSG_UUID_FORMAT);
#ifndef FORMAT_031
  } else if (check_uuid_format(system_id, "System ID", "PO Header") != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "SYSTEM ID is not correct.\n"
                              "%sActual value  : %s\n"
                              "%sExpected value: %s\n", INDENT, system_id, INDENT, ERR_MSG_UUID_FORMAT);
#endif
  } else {

#ifdef FORMAT_031
    ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4321_INFO,
                              "PO Header is correct.\n"
                              "%sDirectory offset  : %lu\n"
                              "%sData offset       : %lu\n"
                              "%sNumber of Objects : %lu\n"
                              "%sPack ID           : %s\n"
                              "%sBucket ID         : %s\n", INDENT, directoryoffset, INDENT, dataoffset,
                              INDENT, num_of_obj, INDENT, pack_id, INDENT, bucket_id);
#else
    ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4321_INFO,
                              "PO Header is correct.\n"
                              "%sDirectory offset  : %lu\n"
                              "%sData offset       : %lu\n"
                              "%sNumber of Objects : %lu\n"
                              "%sPack ID           : %s\n"
                              "%sBucket ID         : %s\n"
                              "%sSYSTEM ID         : %s\n", INDENT, directoryoffset, INDENT, dataoffset,
                              INDENT, num_of_obj, INDENT, pack_id, INDENT, bucket_id, INDENT, system_id);
#endif
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "end  :cpof_po_header\n");
  return ret;
}

/**
 * Check if objects directory is correct.
 * @param [in]  (data_buf)         Pointer of a packed object.
 * @param [in]  (num_of_obj)       Number of objects on the tape.
 * @param [in]  (poid_length)      Byte size of Packed Object Info Length described in PO Info Directory.
 * @param [out] (current_position) Current position of a pointer.
 * @param [out] (objects)          Pointer of a LTOS object.
 * @return      (OK/NG)            If an object directory format is correct or not.
 */
static int cpof_object_directory(unsigned char* const data_buf, uint64_t* const current_position, const uint64_t poid_length,
                                 const uint64_t po_header_length, const uint64_t num_of_obj, LTOSObject* const objects) {
  uuid_t object_id_uuid;
  LTOSObject* current_object;
  int ret                          = output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "start:cpof_object_directory\n");
  uint64_t actual_size_of_po_info  = 0;			          // Check if actual data size is the same as [Packed Object Info Length] described in [PO Info Directory].
  uint32_t object_directory_offset = 0;
  uint64_t object_directory_size   = ((num_of_obj + 1) * (OBJECT_ID_SIZE + META_DATA_OFFSET_SIZE + OBJECT_DATA_OFFSET_SIZE));

  uint8_t* object_directory         = (uint8_t*)clf_allocate_memory(object_directory_size, "Object_directory");
  memmove(object_directory, data_buf + *current_position, object_directory_size);
  *current_position += object_directory_size;

  /* Read Objects Directory. */
  for (uint64_t objnum = 0; objnum < num_of_obj + 1; objnum++) {
    current_object = objects + objnum;
    /* Read Object ID. */
    memmove(object_id_uuid, object_directory + object_directory_offset, OBJECT_ID_SIZE);
    object_directory_offset += OBJECT_ID_SIZE;
    uuid_unparse(object_id_uuid, current_object->object_id);
    /* Read Meta Data Offset. */
    r64(BIG, object_directory + object_directory_offset, &(current_object->meta_data_offset), 1);
    object_directory_offset += META_DATA_OFFSET_SIZE;
    /* Read Object Data Offset. */
    r64(BIG, object_directory + object_directory_offset, &(current_object->object_data_offset), 1);
    object_directory_offset += OBJECT_DATA_OFFSET_SIZE;
  }
  actual_size_of_po_info = po_header_length + object_directory_offset;   //PO header already confirmed in cpof_po_header, so add a size of [Object Directory] only.

  free(object_directory);
  object_directory = NULL;

  /* Check Objects Directory. (Object ID is complying with UUID v4, Meta/Object Data Offset should increase in the order of object) */
  for (uint64_t objnum = 0; objnum < num_of_obj; objnum++) {
    current_object = objects + objnum;

    if (check_uuid_format(current_object->object_id, "Object", "Object Directory") != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                                "Object ID is not correct.\n"
                                "%sActual value  : %s\n"
                                "%sExpected value: %s\n", INDENT, current_object->object_id, INDENT, ERR_MSG_UUID_FORMAT);
    } else if (current_object->object_data_offset <= current_object->meta_data_offset) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                                "Object/Meta Data Offset is not correct.\n"
                                "%sActual value  : Object Data Offset = %lu, Meta Data Offset = %lu\n"
                                "%sExpected value: %s\n", INDENT, current_object->object_data_offset,
                                current_object->meta_data_offset, INDENT, ERR_MSG_OBJ_OFFSET_RELATIVE_VALUE);
    } else if ((current_object + 1)->meta_data_offset < current_object->object_data_offset) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                                "Object/Meta Data Offset is not correct.\n"
                                "%sActual value  : Next Meta Data Offset = %lu, Current Object Data Offset = %lu\n"
                                "%sExpected value: %s\n", INDENT, (current_object + 1)->meta_data_offset,
                                current_object->object_data_offset, INDENT, ERR_MSG_META_OFFSET_RELATIVE_VALUE);
    } else if ((current_object + 1)->meta_data_offset <= current_object->meta_data_offset) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                                "Meta Data Offset is not correct.\n"
                                "%sActual value  : Current = %lu, Next = %lu\n"
                                "%sExpected value: %s\n", INDENT, current_object->meta_data_offset,
                                (current_object + 1)->meta_data_offset, INDENT, ERR_MSG_OFFSET_ORDER);
    } else if ((current_object + 1)->object_data_offset < current_object->object_data_offset) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                                "Object Data Offset is not correct.\n"
                                "%sActual value  : Current = %lu, Next = %lu\n"
                                "%sExpected value: %s\n", INDENT, current_object->object_data_offset,
                                (current_object + 1)->object_data_offset, INDENT, ERR_MSG_OFFSET_ORDER);
    }
    actual_size_of_po_info += current_object->object_data_offset - current_object->meta_data_offset;
  }

  /* Check if actual data size is the same as [Packed Object Info Length] described in [PO Info Directory]. */
  if (actual_size_of_po_info != poid_length) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                              "Byte size of Packed Object Info is not correct.\n"
                              "%sActual value  : %lu\n"
                              "%sExpected value: %lu\n"
                              "%s              : It should be the same as [Packed Object Info Length] described in [Packed Object Info Directory].\n",
                              INDENT, actual_size_of_po_info, INDENT, poid_length);
  }

  /* Check the last content in Objects Directory (Should be UUID = UUID_LAST_OBJECT = "00000000-0000-0000-0000-000000000000",
   * Meta Data Offset = Object Data Offset = Last Data Offset, and Last Data Offset <= MAX_PO_SIZE */
  current_object = objects + num_of_obj;

  if (strcasecmp((current_object->object_id), UUID_LAST_OBJECT)) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                              "Object ID of the last Objects Directory is not correct.\n"
                              "%sActual value  : %s\n"
                              "%sExpected value: %s\n", INDENT, current_object->object_id, INDENT, UUID_LAST_OBJECT);
  } else if (current_object->meta_data_offset != current_object->object_data_offset) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                              "Both Data Offset of the last Objects Directory are not correct.\n"
                              "%sActual value  : Object Data Offset = %lu, Meta Data Offset = %lu\n"
                              "%sExpected value: %s\n", INDENT, current_object->object_data_offset,
                              current_object->meta_data_offset, INDENT, ERR_MSG_OFFSET_IN_LAST_OBJ_DIR);
  }
  else if (MAX_PO_SIZE < current_object->object_data_offset) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO,
                              "Packed Object size is not correct.\n"
                              "%sActual value  : Packed Object size = %lu\n"
                              "%sExpected value: %s\n", INDENT, current_object->object_data_offset,
                              INDENT, ERR_MSG_PO_SIZE_OVER_LIMIT);
  }

  if (ret == OK) {
    ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4321_INFO,
                              "Objects Directory is correct.\n"
                              "%sLast Data Offset  : %lu\n", INDENT, current_object->object_data_offset);
  }
  return ret;
}

#ifdef OBSOLETE
/**
 * Check if Meta data of each Object is correct.
 * @param [in]  (data_buf)         Pointer of a packed object.
 * @param [out] (current_position) Current position of a pointer.
 * @param [in]  (num_of_obj)       Number of Objects in a Packed Object.
 * @param [in]  (objects)          Pointer of a LTOS object.
 * @return      (OK/NG)            If Object Meta data is correct or not.
 */
static int cpof_obj_meta(unsigned char* const data_buf, int* const current_position, const int num_of_obj, const LTOSObject* const objects) {
  int ret               = OK;
  int metadata_size     = 0;
  int64_t binary_size   = 0;
  uint32_t residual_cnt = 0;

  set_top_verbose(DISPLAY_ALL_INFO);
  ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_ALL_INFO, "LEVEL 0\n");

  // Check if Meta data of each Object is complying with JSON format.
  for (int objnum = 0; objnum < num_of_obj; objnum++ ) {
    int ret_sub                    = OK;
    enum json_tokener_error error;
    struct json_object *value      = NULL;
    struct json_object *root_json  = NULL;
    metadata_size                  = (objects + objnum)->object_data_offset - (objects + objnum)->meta_data_offset;
    binary_size                    = (objects + objnum + 1)->meta_data_offset - (objects + objnum)->object_data_offset;
    unsigned char *metadata        = (unsigned char *)clf_allocate_memory(metadata_size, "metadata");
    ret |= read_data_from_multi_blocks(data_buf, current_position, metadata, metadata_size);
    // Parse KEY and VALUE in the meta data
    root_json = json_tokener_parse_verbose((char*)metadata, &error);
    if (json_tokener_success != error) {
    	ret_sub |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "Meta data in Number %d Object is not complying with JSON format.\n", objnum);
     }
    if (!json_object_object_get_ex(root_json, "object_key", &value)) {
      ret_sub |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "\"object_key\" was not found in Number %d Object.\n", objnum);
     }
    ret_sub |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_ALL_INFO,
                                  "Object Meta data is correct.\n"
                                  "%sSequence ID : %d\n" // # of Object will start from 1 but not 0.
                                  "%sObject Key  : %s\n", INDENT, objnum + 1, INDENT, json_object_get_string(value));
    // Drop a binary data of the current Object until a start position of the next Object is found.
    while (LTOS_BLOCK_SIZE < binary_size + *current_position) {       // Start position of the next Object = current Object binary size + *current_position
      set_hexspeak(data_buf, LTOS_BLOCK_SIZE);
      ret_sub |= read_data(LTOS_BLOCK_SIZE, data_buf, &residual_cnt);
      binary_size -= LTOS_BLOCK_SIZE;                                 // Relatively, the remaining binary size should decrease after reading (or pointer moves on)
     }
    *current_position += binary_size;                                 // Set the pointer to a start position of the Next Meta data.

    free(metadata);
    metadata  = NULL;
    json_object_put(value);
    value     = NULL;
    json_object_put(root_json);
    root_json = NULL;
    ret |= ret_sub;
  }
  set_top_verbose(DEFAULT);
  return ret;
}
#endif // OBSOLETE

#ifdef OBSOLETE
/**
 * Check if the remaining area is padded with ZEROs.
 * @param [in]  (data_buf)         Pointer of a packed object.
 * @param [out] (current_position) Current position of a pointer.
 * @return      (OK/NG)            Return OK if process is complete correctly.
 */
static int cpof_po_padding(unsigned char* const data_buf, int* const current_position) {
  int ret      = OK;
  set_top_verbose(DISPLAY_HEADER_AND_L4321_INFO);

  for (int position = *current_position; position < LTOS_BLOCK_SIZE; position++ ) {
    if ( data_buf[position] != 0x00 ) {
      output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4321_INFO,
               "PO has an invalid value 0x%02X at %d in the last block.\n", data_buf[position], position);
      break;
    }
  }
  set_top_verbose(DEFAULT);
  return ret;
}
#endif // OBSOLETE

#ifdef OBSOLETE
/**
 * Check if objects were packed correctly.
 * @param [in]  (unpackedobjpath) Pointer of Folder path to place unpacked objects.
 *                                If it's empty, unpacked objects will not be made.
 * @param [in]  (data_buf)        Pointer of a packed object.
 * @param [in]  (poid_length)     Byte size of Packed Object Info Length described in PO Info Directory.
 * @return      (OK/NG)           If a packed object format is correct or not.
 */
static int check_packed_object_format(const char* const unpackedobjpath, unsigned char* const data_buf, const uint64_t poid_length) {
  int current_position = 0;
  int num_of_obj       = 0;
  int po_header_length = 0;
  int ret              = OK;

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L4321_INFO, "check_packed_object_format()\n");

  /* Check PO Identifier. */
  if (cpof_po_identifier(data_buf, &current_position) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "PO identifier format is not correct.\n");
  }
  /* Check PO Header. */
  if (cpof_po_header(data_buf, &current_position, &num_of_obj, &po_header_length) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "PO header format is not correct.\n");
  }
  /* Check Object Directory. */
  LTOSObject objects[num_of_obj + 1];
  if (cpof_object_directory(data_buf, &current_position, poid_length, po_header_length, num_of_obj, objects) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "Objects Directory format is not correct.\n");
  }
  /* Check Meta data in each Object. */
  if (cpof_obj_meta(data_buf, &current_position, num_of_obj, objects) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "At least one of Objects has an invalid Meta data.\n");
  }
  /* Check if the remaining area is padded with ZEROs. */
  if (cpof_po_padding(data_buf, &current_position) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "PO was not padded with ZERO(s) in the last block.\n");
  }
  /* Write objects to files. */
  if (strcmp(unpackedobjpath, EMPTY) != 0) {
    if (cpof_write_objects_to_files(unpackedobjpath, data_buf, &current_position, &num_of_obj, objects) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "Can't write objects to file.\n");
    }
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L4321_INFO, "check_packed_object_format()\n");

  return ret;
}
#endif // OBSOLETE

#ifdef OBSOLETE
/**
 * Check packed objects format.
 * @param [in]  (unpackedobjpath)   Save path of unpacked object.
 * @param [in]  (po_top_block_num)  Block address of the first PO in each Object Series.
 * @param [in]  (poid_length)       Byte size of Packed Object Info Length described in PO Info Directory.
 * @param [in]  (poid_block_offset) Relative block number of Packed object described in PO Info Directory.
 * @return      (OK/NG)             If the format is correct or not.
 */
int clf_packed_objects(const char* unpackedobjpath, const int po_top_block_num,
                       const uint64_t poid_length, const uint64_t poid_block_offset) {
  unsigned char po_buffer[LTOS_BLOCK_SIZE + 1] = { 0 };
  unsigned int dat_size                        = 0;
  int ret                                      = OK;
  set_top_verbose(DISPLAY_HEADER_AND_L4321_INFO);

  if (unpackedobjpath == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at clf_packed_objects");
  }
#ifndef NO_TAPE
  if (locate_to_tape(po_top_block_num + poid_block_offset) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "Failed to locate to 'Packed Objects'.\n");
  }
  if (read_data(LTOS_BLOCK_SIZE, po_buffer, &dat_size) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "Failed to read at 'Packed Objects'.\n");
  }
#else
  output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4321_INFO, "Can't check Packed Objects format without tape.\n");
  return OK;
#endif

  ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L4321_INFO, "LEVEL 1\n");
  if (check_packed_object_format(unpackedobjpath, po_buffer, poid_length) != 0) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "Packed object format is not correct.\n");
  }

  set_top_verbose(DEFAULT);
  return ret;
}
#endif // OBSOLETE

/**
 * Check if Meta data of each Object is correct.
 * @param [in]     (data_buf)         Pointer of a packed object info (It means that there is no object data).
 * @param [in/out] (current_position) Current position of a pointer.
 * @param [in]     (num_of_obj)       Number of Objects in a Packed Object.
 * @param [in]     (objects)          Pointer of a LTOS object.
 * @return         (OK/NG)            If Object Meta data is correct or not.
 */
static int cpof_only_meta(unsigned char* const data_buf, uint64_t* const current_position, const int num_of_obj, const LTOSObject* const objects) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "start:cpof_only_meta\n");

  ret |= output_accdg_to_vl(OUTPUT_INFO, DEFAULT, "LEVEL 0\n");

  // Check if Meta data of each Object is complying with JSON format.
  for (int objnum = 0; objnum < num_of_obj; objnum++ ) {
    enum json_tokener_error error;
    //struct json_object *value     = NULL; //this variable will be used when either FORMAT_031 or FORMAT_1_0_0 is defined.
    struct json_object *root_json = NULL;
    const int metadata_size       = (objects + objnum)->object_data_offset - (objects + objnum)->meta_data_offset;
    unsigned char *metadata       = (unsigned char *)clf_allocate_memory(metadata_size + 1, "metadata");
    ret |= output_accdg_to_vl(OUTPUT_DEBUG, DEFAULT, "metadata_size=%d, data_offset=%lu, metadata_offset=%lu\n",
                              metadata_size, (objects + objnum)->object_data_offset,
                              (objects + objnum)->meta_data_offset);
    memmove(metadata, data_buf + *current_position, metadata_size);
    *current_position += metadata_size;
    // Parse KEY and VALUE in the meta data
    root_json = json_tokener_parse_verbose((char*)metadata, &error);
    if (json_tokener_success != error) {
    	ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT,
    	                          "Meta data in Number %d Object is not complying with JSON format.\n", objnum);
    }
/*
#if defined(FORMAT_031) || defined(FORMAT_1_0_0)
    if (!json_object_object_get_ex(root_json, "object_key", &value)) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DEFAULT, "\"object_key\" was not found in Number %d Object.\n", objnum);
    }
    ret |= output_accdg_to_vl(OUTPUT_INFO, DEFAULT,
                                "Object Meta data is %scorrect.\n"
                                "%sSequence ID : %d\n" // # of Object will start from 1 but not 0.
                                "%sObject Key  : %s\n", ret ? "not " : "", INDENT, objnum + 1, INDENT, json_object_get_string(value));
    json_object_put(value);
#else
*/
    struct json_object *metadata_version_metadata   = NULL;
    struct json_object *key_metadata                = NULL;
    struct json_object *size_metadata               = NULL;
    struct json_object *last_modified_time_metadata = NULL;

    if (!json_object_object_get_ex(root_json, "MetadataVersion", &metadata_version_metadata)) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"MetadataVersion\" was not found(Number %d Object).\n", objnum);
    } else if (json_object_get_type(metadata_version_metadata) != json_type_int) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"MetadataVersion\" is not an integer(Number %d Object).\n", objnum);
    } else if (json_object_get_int(metadata_version_metadata) != 1) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"MetadataVersion\" is not 1(Number %d Object).\n", objnum);
    }

    if (!json_object_object_get_ex(root_json, "Key", &key_metadata)) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"Key\" was not found(Number %d Object).\n", objnum);
    } else if (MAX_KEY_LENGTH_IN_META < strlen(json_object_get_string(key_metadata))) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"Key\" format is not correct(Number %d Object).\n", objnum);
    }

    if (!json_object_object_get_ex(root_json, "Size", &size_metadata)) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"Size\" was not found(Number %d Object).\n", objnum);
    } else if (json_object_get_type(size_metadata) != json_type_int) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"Size\" is not an integer(Number %d Object).\n", objnum);
    } else if (json_object_get_int(size_metadata) < 0) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"Size\" is less than 0(Number %d Object).\n", objnum);
    }

    if (!json_object_object_get_ex(root_json, "LastModifiedTime", &last_modified_time_metadata)) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"LastModifiedTime\" was not found(Number %d Object).\n", objnum);
    } else if (check_utc_format(json_object_get_string(last_modified_time_metadata)) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"LastModifiedTime\" format is not correct(Number %d Object).\n", objnum);
    }

    json_object_object_foreach(root_json, key, val) {
      if (strcmp(key,"IsDeleted") == 0) {
        if (json_object_get_type(val) != json_type_boolean) {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"IsDeleted\" is not a boolean(Number %d Object).\n", objnum);
        }
        break;
      }
      if (strcmp(key,"UserMetadata") == 0) {
        json_object_object_foreach(val, user_metadata_key, user_metadata_val) {
          if (json_object_get_type(user_metadata_val) != json_type_string) {
            ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"%s\" is not a string(Number %d Object).\n", user_metadata_key, objnum);
          }
        }
        break;
      }
      if ((strcmp(key,"ContentEncoding") == 0) || (strcmp(key,"ContentType") == 0) ||
          (strcmp(key,"ContentMd5") == 0) || (strcmp(key,"ContentLanguage") == 0) ||
          (strcmp(key,"CreationTime") == 0) || (strcmp(key,"ServerSideCompression") == 0) ||
          (strcmp(key,"ServerSideEncryption") == 0) || (strcmp(key,"ServerSideEncryptionKeyId") == 0) ||
          (strcmp(key,"ServerSideEncryptionCustomer") == 0) || (strcmp(key,"Version") == 0)) {
        if (json_object_get_type(val) != json_type_string) {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "\"%s\" is not a string(Number %d Object).\n", key, objnum);
        }
        break;
      }
      if ((strcmp(key,"MetadataVersion") == 0) || (strcmp(key,"Key") == 0) ||
          (strcmp(key,"Size") == 0) || (strcmp(key,"LastModifiedTime") == 0)) {
        break;
      }
      if (strncmp(key, "Vendor", 6) != 0) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "The key(\"%s\") is unusable(Number %d Object).\n", key, objnum);
      }
    }

    ret |= output_accdg_to_vl(OUTPUT_INFO, DEFAULT,
                                "Object Meta data is %scorrect.\n"
                                "%sSequence ID : %d\n" // # of Object will start from 1 but not 0.
                                "%sMetadataVersion  : %d\n"
                                "%sKey  : %s\n"
                                "%sSize  : %d\n"
                                "%sLastModifiedTime  : %s\n", ret ? "not " : "",
                                INDENT, objnum + 1,
                                INDENT, json_object_get_int(metadata_version_metadata),
                                INDENT, json_object_get_string(key_metadata),
                                INDENT, json_object_get_int(size_metadata),
                                INDENT, json_object_get_string(last_modified_time_metadata));
    json_object_put(metadata_version_metadata);
    metadata_version_metadata   = NULL;
    json_object_put(key_metadata);
    key_metadata                = NULL;
    json_object_put(size_metadata);
    size_metadata               = NULL;
    json_object_put(last_modified_time_metadata);
    last_modified_time_metadata = NULL;
//#endif  //if defined(FORMAT_031) || defined(FORMAT_1_0_0)

    json_object_put(root_json);
    root_json = NULL;
    free(metadata);
    metadata  = NULL;
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "end  :cpof_only_meta\n");
  return ret;
}

/**
 * Check packed objects info format.
 * @param [in]  (unpackedobjpath)   Save path of unpacked object.
 * @param [in]  (data_buf)          (Char* / Unsigned Char*) Pointer to the beginning of Packed Objects Info
 * @param [in]  (current_position)  Byte offset of current Block.
 * @param [in]  (po_info_length)    Byte size of Packed Object Info Length described in PO Info Directory.
 * @return      (OK/NG)             If the format is correct or not.
 */
int clf_packed_objects_info(const char* unpackedobjpath, unsigned char* const data_buf,
                            uint64_t* const current_position, const uint64_t po_info_length) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "start:clf_packed_objects_info\n");

#ifdef NO_TAPE
  output_accdg_to_vl(OUTPUT_INFO, DEFAULT, "Can't check Packed Object Info format without tape.\n");
#else
  if (unpackedobjpath == NULL || data_buf == NULL || current_position == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at clf_packed_objects_info");
  }
  ret |= output_accdg_to_vl(OUTPUT_INFO, DEFAULT, "LEVEL 2\n");

  /* Check PO Header. */
  uint64_t num_of_obj       = 0;
  uint64_t po_header_length = 0;
  if (cpof_po_header(data_buf, current_position, &num_of_obj, &po_header_length) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "PO header format is not correct.\n");
  }
  /* Check Object Directory. */
  LTOSObject objects[num_of_obj + 1];
  if (cpof_object_directory(data_buf, current_position, po_info_length, po_header_length, num_of_obj, objects) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Objects Directory format is not correct.\n");
  }
  /* Check Meta data in each Object. */
  if (cpof_only_meta(data_buf, current_position, num_of_obj, objects) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "At least one of Objects has an invalid Meta data.\n");
  }
#endif
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DEFAULT, "end  :clf_packed_objects_info\n");
  return ret;
}
