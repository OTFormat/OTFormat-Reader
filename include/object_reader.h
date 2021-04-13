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
 * @file  object_reader.h
 * @brief Function declaration to read a data complying with OTFormat.
 */

#ifndef INCLUDE_OBJECT_READER_H_
#define INCLUDE_OBJECT_READER_H_

#include "ltos_format_checker.h"

#define UUID_SIZE                                 (36)
#define UUID_BIN_SIZE                             (16)
#define UTC_LENGTH                                (27)
#define MAX_KEY_SIZE                              (1024)
#define MD5_SIZE                                  (32)
#define MD5_BIN_SIZE                              (16)
#define MAX_OBJ_SIZE_LENGTH                       (13) //5TB = 5 * 10^12
#define MAX_BLOCK_ADDRESS_LENGTH                  (9)  //1PB = 1 * 10^15 / 1M = 1 * 10^9
#define MAX_LINE_LENGTH                           (2048) //JSON descriptor + MAX_KEY_SIZE + MAX_OBJ_SIZE_LENGTH +UTC_LENGTH + UUID_SIZE + MD5_SIZE + UUID_SIZE + MAX_BLOCK_ADDRESS_LENGTH)
#define MAX_LEVEL_OPT_VALUE                       (1)
#define MAX_PO_OUTPUT_PATH                        (OUTPUT_PATH_SIZE + UUID_SIZE + PO_EXTENSION_SIZE + 1) // +1 means a separator. "\" will appear once.
#define MAX_OBJ_OUTPUT_PATH                       (OUTPUT_PATH_SIZE + BUCKET_LIST_BUCKETNAME_MAX_SIZE + MAX_KEY_SIZE + UUID_SIZE + META_EXTENSION_SIZE + 3) // +3 means separator. "\" will appear three times.
#define MAX_META_OUTPUT_PATH                      (MAX_OBJ_OUTPUT_PATH) // same length as object data.
#define VERSION_OPT_LATEST                        "latest";
#define VERSION_OPT_ALL                           "all";
#define OUTPUT_OBJECT                             (0)
#define OUTPUT_PACKED_OBJECT                      (1)
#define MAX_NUMBER_OF_OBJECTS                     (10000)
#define MAX_NUMBER_OF_LISTS                       (1000)
#define TEMP_PATH                                 "/tmp/object_reader/"
#define DF_CMD_LOG_PATH                           "/tmp/df_cmd_result.tmp"
#define MD5SUM_CMD_LOG_PATH                       "/tmp/md5sum_cmd_result.tmp"
#define DATA_EXTENSION                            ".data"
#define DATA_EXTENSION_SIZE                       (5)
#define META_EXTENSION                            ".meta"
#define META_EXTENSION_SIZE                       (5)
#define PO_EXTENSION                              ".pac"
#define PO_EXTENSION_SIZE                         (4)
#define DEFAULT_MODE                              (1) // used for saving data from tape in function of "save_data_from_tape"
#define OBJECT_MODE                               (2) // used for saving Object data and metadata from tape in function of "save_data_from_tape"
#define PO_MODE                                   (3) // used for saving PO from tape in function of "save_data_from_tape"
#define DEFAULT_BARCODE                           "null"
#define BARCODE_SIZE                              (8)
#define DEFAULT_HISTORY_INTERVAL                  (60 * 60)      // 1 hour
#define MIN_HISTORY_INTERVAL                      (60)           // 1 minute
#define MAX_HISTORY_INTERVAL                      (24 * 60 * 60) // 1 day
#define MAX_HISTORY_INTERVAL_SIZE                 (5)            // 1 day = 86400 = 5 digits
#define MIN_REQUIRED_DISK_SPACE_GiB               (100UL)     // 100 GiB = 100 * 1024^3
#define MAX_DISK_SPACE_LENGTH                     (1 + 21)   // 1 * 10^21 * 1024 ~ 1 YB, where 1024(KiB) is a unit of df command.
#define DISK_SPACE_COMMAND_SIZE                   (64)       // 38 bytes + margin. To be used in get_disk_space function.
#define OBJ_READER_MODE_LENGTH                    (20)

/* Nested 5 structures for storing all meta data formatted in OTFormat. */
typedef struct L4{
  char tape_barcode_id[BARCODE_SIZE + 1];               // Fixed at 8 bytes characters. Please pad SPACE if length is not long enough.
  uint64_t end_rcm_block_pba;                           // Physical block address (PBA) of the last RCM
  uint64_t vcr;                                         // Value of Volume_Change_Ref stored in MAM, used for checking if any data will be added into this tape.
  char pool_id[UUID_BIN_SIZE + 1];                      // UUID of pool_id which this tape belongs to.
  struct L3 *first_pr;
} L4;

typedef struct L3{
  uint32_t pr_id;                                       // Sequence ID of this L3 Partial Reference (Start from 1).
  uint64_t pr_block_offset;                             // Block offset relative to the upper level marker.
  struct L4 *current_tape;                              // Pointer to upper level marker which this L3 belongs to.
  struct L3 *next_pr;                                   // Pointer to next L3 in the current L4 (logically but not physically).
  struct L2 *first_ocm;                                 // Pointer to the beginning of a lower level marker in this L3.
} L3;

typedef struct L2{
  uint32_t ocm_id;                                      // Sequence ID of this L2 Object Commit Marker (Start from 1).
  uint64_t ocm_block_offset;                            // Block offset relative to the upper level marker.
  struct L3 *current_pr;                                // Pointer to upper level marker which this L2 belongs to.
  struct L2 *next_ocm;                                  // Pointer to next L2 in the current L3 (logically but not physically).
  struct L1 *first_po;                                  // Pointer to the beginning of a lower level marker in this L2.
} L2;

typedef struct L1{
  char po_id[UUID_BIN_SIZE + 1];                        // UUID of this L1 Packed Object (PO).
  char bucket_id[UUID_BIN_SIZE + 1];                    // UUID of Bucket_ID which this PO belongs to.
  uint64_t po_block_offset;                             // Block offset relative to the upper level marker.
  struct L2 *current_ocm;                               // Pointer to upper level marker which this L1 belongs to.
  struct L1 *next_po;                                   // Pointer to next L1 in the current L2 (logically but not physically).
  struct L0 *first_obj;                                 // Pointer to the beginning of a lower level marker in this L1.
} L1;

typedef struct L0{
  char id[UUID_BIN_SIZE + 1];                           // UUID of this L0 Object
  char key[MAX_KEY_SIZE + 1];                           // Key value of this Object
  char verson_id[UUID_BIN_SIZE + 1];                    // Version of this L0 complying with UUID Format.
  uint64_t size;                                        // Data size of this L0
  char last_mod_date[UTC_LENGTH + 1];                   // Last modified date (ISO8601 extended)
  char md5[MD5_BIN_SIZE + 1];                           // Hash Value on 128 bits, which is the same as "Etag" in Object's Header.
  BOOL is_delete_marker;                                // Whether this object is Delete Marker or not.
  struct L1 *current_po;                                // Pointer to upper level marker which this L0 belongs to.
  struct L0 *next_obj;                                  // Pointer to next L0 in the current L1 (logically but not physically).
} L0;

typedef struct object_list{
  char key[MAX_KEY_SIZE + 1];                           // Object key of this L0
  char id[UUID_SIZE + 1];                               // UUID of this L0 Object
  char verson_id[UUID_SIZE + 1];                        // Version ID of this L0 complying with UUID Format.
  uint64_t size;                                        // Data size of this L0
  uint64_t metadata_size;                               // Metadata size of this L0
  uint64_t meta_offset;                                 // Offset from the beginning of PO Header to the object metadata (NOTE: 32 bytes Identifier is prepared just before PO header.
  uint64_t data_offset;                                 // Offset from the beginning of PO Header to the object data (NOTE: 32 bytes Identifier is prepared just before PO header.
  char last_mod_date[UTC_LENGTH + 1];                   // Last modified date (ISO8601 extended)
  char md5[MD5_SIZE + 1];                               // Hash Value on 32 bytes, which is the same as "Etag" in Object's Header.
  BOOL is_delete_marker;                                // Whether this object is Delete Marker or not.
  char po_id[UUID_SIZE + 1];                            // UUID of L1 Packed Object (PO) which has this object.
  uint64_t block_address;                               // Block address at which PO is stored in tape.
  struct L0 *next_obj;                                  // Pointer to next L0.
  struct object_list* next;                             // Pointer to next object.
} object_list;

//int           add_L0_obj(L0* const current, L0* const next);
//int           add_L1_po(L1* const current, L1* const next);
//int           add_L2_ocm(L2* const current, L2* const next);
//int           add_L3_pr(L3* const current, L3* const next);
//int           search_object_by_key(const L0* const obj, const char* const key, uint64_t* const seq_id);
//uint64_t      identify_object_by_key(const L1* const po, const char* key);
//int           search_object_by_uuid(const L0* const obj, const char* const uuid, uint64_t* const seq_id);
//uint64_t      identify_object_by_uuid(const L1* const po, const char* uuid);
int           check_file(const char* const filename);
int           get_object_info_in_list(const char* const object_key, const char* const object_id, const char* const list_path, object_list** objects);
void          set_force_flag(int is_force_enabled);
int           check_disk_space(const char* const path, const uint64_t data_size);
int           comlete_list_files(const char* const list_dir);
#endif /* INCLUDE_OBJECT_READER_H_ */
