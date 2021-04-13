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
 * @file  ltos_format_checker.h
 * @brief Function declaration to check if a data written in a tape complies with OTFormat.
 */

#ifndef LTOS_FORMAT_CHECKER_H
#define LTOS_FORMAT_CHECKER_H

#include <uuid/uuid.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <getopt.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <json/json.h>
#include <ctype.h>
#include <stdarg.h>
#include <dirent.h>
#include <math.h>
#include "spti_lib.h"
#include "endian_utils.h"
#include "str_replace.h"
#include "output_level.h"
#include "scsi_util.h"
#include "object_reader.h"

#define OK                                        (0)
#define NG                                        (1)
#define EQUAL                                     (0)
#define FIRST                                     (0)
#define LAST                                      (1)
#define OFF                                       (0)
#define ON                                        (1)
#define ZERO                                      (0)
#define ONE_BEFORE                                (-1)
#define STOP                                      (11)
#define EMPTY                                     ""
#define DOUBLE_QUART                              "\""
#define SINGLE_QUART                              "'"
#define MAX_PATH                                  (PATH_MAX)
#define DATA_PARTITION                            (1)
#define REFERENCE_PARTITION                       (0)
#define DP                                        "dp"
#define RP                                        "rp"
#define ALL                                       "all"
#define CONT                                      "cont"
#define EXIT                                      "exit"
#define ANSI_LABEL_SIZE                           (80)
#define DEVICE_NAME_SIZE                          (PATH_MAX)
#define PACKED_OBJ_PATH_SIZE                      (PATH_MAX)
#define OUTPUT_PATH_SIZE                          (PATH_MAX)
#define UNPACKED_OBJ_PATH_SIZE                    (PATH_MAX)
#define UNSIGNED_INT_SIZE                         (4)
#define DECIMAL_NUMBER                            (10)
#define FORMAT_CHECKER_VERSION                    "FORMAT_CHECKER_1.0.0"
#define EOS                                       '\0'
#define ERROR                                     (-1)
#define UUID_SIZE                                 (36)
#define ZERO_FILLED_UUID                          "00000000-0000-0000-0000-000000000000"
#define UUID_LAST_OBJECT                          ZERO_FILLED_UUID
#define JSON_EXT                                  ".json"
#define ARRAY_KEY                                 "ObjectList"
#define DEVICE_CHECK_COMMAND                      "lsscsi -g | grep tape | grep %s > /dev/null"
#define COMMAND_SIZE                              (PATH_MAX)
#define OBJECT_SERIES_FILE_MARK_NUM               (-2)
#define UTC_LENGTH                                (27)
#define UTC_HYPHEN1                               (4)
#define UTC_HYPHEN2                               (7)
#define UTC_HYPHEN_NUM                            (2)
#define UTC_T                                     (10)
#define UTC_COLON1                                (13)
#define UTC_COLON2                                (16)
#define UTC_DOT                                   (19)
#define UTC_Z                                     (26)
#define STR_MAX                                   (256)
/* ScsiLib Argument. */
#define SPACE_BLOCK_MODE                          (0x00)
#define SPACE_FILE_MARK_MODE                      (0x01)
#define SPACE_EOD_MODE                            (0x03)
/* Relating to Partition. */
#define NUMBER_OF_PARTITIONS                      (2)
/* Relating to LTOS Label. */
#define LTOS_BLOCK_SIZE                           (1024 * 1024)
#define VOL1_LABEL_SIZE                           (80)
#define LABEL_IDENTIFIER                          "VOL"
#define LABEL_IDENTIFIER_SIZE                     (3)
#define LABEL_NUMBER                              "1"
#define LABEL_NUMBER_SIZE                         (1)
#define VOLUME_IDENTIFIER                         "FF0013"
#define VOLUME_IDENTIFIER_SIZE                    (6)
#define VOLUME_ACCESSIBILITY                      " "        /* LTFS:L, LTOS:Space */
#define VOLUME_ACCESSIBILITY_SIZE                 (1)
#define RESERVED_13_SPACES                        "             "
#define RESERVED_13_SPACES_SIZE                   (13)

#ifdef FORMAT_031
#define IMPLEMENTATION_IDENTIFIER                 "LTOS"
#define IMPLEMENTATION_IDENTIFIER_SPCE            "         "
#define IMPLEMENTATION_IDENTIFIER_SIZE            (4)
#elif FORMAT_1_0_0
#define IMPLEMENTATION_IDENTIFIER                 "AFHKMSTY"
#define IMPLEMENTATION_IDENTIFIER_SPCE            "     "
#define IMPLEMENTATION_IDENTIFIER_SIZE            (8)
#else
#define IMPLEMENTATION_IDENTIFIER                 "OTFormat"
#define IMPLEMENTATION_IDENTIFIER_SPCE            "     "
#define IMPLEMENTATION_IDENTIFIER_SIZE            (8)
#endif

#define IMPLEMENTATION_IDENTIFIER_BUF_SIZE        (13)
#define OWNER_IDENTIFIER                          "              "
#define OWNER_IDENTIFIER_SIZE                     (14)
#define RESERVED_28_SPACES                        "                            "
#define RESERVED_28_SPACES_SIZE                   (28)
#define LABEL_STANDARD_VERSION                    "4"
#define LABEL_STANDARD_VERSION_SIZE               (1)

#define SPACE                                     " "

#if defined(IDENTIFIER_1_0)
#define VERSION_IN_IDENTIFIER                     "1.0"
#else
#define VERSION_IN_IDENTIFIER                     "0.0"
#endif

#if defined(SYSTEM_1_0_0)
#define VERSION_IN_LABEL            "1.0.0"
#elif defined(SYSTEM_1_1_0)
#define VERSION_IN_LABEL            "1.1.0"
#elif defined(SYSTEM_2_0_0)
#define VERSION_IN_LABEL            "2.0.0"
#else
#define VERSION_IN_LABEL            "0.0.0"
#endif

#if defined(OBJ_ARCHIVE_1_0_0)
#define VERSION_IN_CREATOR            "1.0.0"
#elif defined(OBJ_ARCHIVE_1_0_0)
#define VERSION_IN_CREATOR            "1.1.0"
#elif defined(OBJ_ARCHIVE_2_0_0)
#define VERSION_IN_CREATOR            "2.0.0"
#else
#define VERSION_IN_CREATOR            "0.0.0"
#endif

/* Relating to PO Identifier. */
#ifdef FORMAT_031
#define PO_IDENTIFIER_ASCII_CODE                  { IMPLEMENTATION_IDENTIFIER "1.0.0 Level1" };
#else
#define PO_IDENTIFIER_ASCII_CODE                  ( IMPLEMENTATION_IDENTIFIER SPACE VERSION_IN_IDENTIFIER SPACE "Level1             " )
#endif

#define META_MAX_SIZE                             (2 * 1024 *1024)

#define PO_IDENTIFIER_SIZE                        (sizeof(PO_IDENTIFIER_ASCII_CODE) - 1)
/* Relating to PO Header. */
#define DIRECTORY_OFFSET_SIZE                     (8)
#define DATA_OFFSET_SIZE                          (8)
#define NUMBER_OF_OBJECTS_SIZE                    (8)
#define PACK_ID_SIZE                              (16)
#define BUCKET_ID_SIZE                            (16)
#ifdef FORMAT_031
#define PO_HEADER_SIZE   (56)
#else
#define PO_HEADER_SIZE   (72)
#endif
/* Relating to Objects Directory. */
#define OBJECT_ID_SIZE                            (16)
#define META_DATA_OFFSET_SIZE                     (8)
#define OBJECT_DATA_OFFSET_SIZE                   (8)
#define PO_DIR_SIZE      (OBJECT_ID_SIZE + META_DATA_OFFSET_SIZE + OBJECT_DATA_OFFSET_SIZE)
/* Relating to Object Commit Marker. */
#ifdef FORMAT_031
#define OCM_IDENTIFIER                            { IMPLEMENTATION_IDENTIFIER "1.0.0 Level2" };
#else
#define OCM_IDENTIFIER                            ( IMPLEMENTATION_IDENTIFIER SPACE VERSION_IN_IDENTIFIER SPACE "Level2             " )
#endif

#define OCM_HEADER_SIZE                           (24)
#define OCM_DIR_SIZE                              (16) // This is size of POInfo dir in OCM instead of OCM dir in PR.
/* Relating to Partial Reference. */
#ifdef FORMAT_031
#define PR_IDENTIFIER                             { IMPLEMENTATION_IDENTIFIER "1.0.0 Level3" };
#else
#define PR_IDENTIFIER                             ( IMPLEMENTATION_IDENTIFIER SPACE VERSION_IN_IDENTIFIER SPACE "Level3             " )
#endif

#define LENGTH_DIRECTORY                          (8) //FIXME Equal to OCM_INFO_LENGTH_SIZE
#define BLOCK_OFFSET_DIRECTORY                    (8)
#define OCM_INFO_LENGTH_SIZE                      (8)

#ifdef FORMAT_031
#define PR_HEADER_SIZE                            (56)
#else
#define PR_HEADER_SIZE                            (24)
#endif

#define PR_DIR_SIZE                               (16) // This is size of OCM Dir in PR instead of PR dir in RCM.
/* Relating to Reference Commit Marker. */
#ifdef FORMAT_031
#define RCM_IDENTIFIER                            { IMPLEMENTATION_IDENTIFIER "1.0.0 Level4" };
#else
#define RCM_IDENTIFIER                            ( IMPLEMENTATION_IDENTIFIER SPACE VERSION_IN_IDENTIFIER SPACE "Level4             " )
#endif

#define IDENTIFIER_SIZE                           (sizeof(RCM_IDENTIFIER) - 1)
#define DATA_LENGTH_SIZE                          (8)
#define NUMBER_OF_PARTIAL_REFERENCE_SIZE          (8)
#define SYSTEM_ID_SIZE                            (16)
#define POOL_ID_SIZE                              (16)

#ifndef FORMAT_031
#define POOL_GROUP_ID_SIZE                        (16)
#endif

#define PR_BLOCK_SIZE                             (8)

#ifdef FORMAT_031
#define RCM_HEADER_SIZE                           (64)
#else
#define RCM_HEADER_SIZE                           (80)
#endif

#define SYSTEMINFO_POOLGROUPNAME_MIN_SIZE         (3)
#define SYSTEMINFO_POOLGROUPNAME_MAX_SIZE         (63)

#define MAX_NUM_OF_PR                             (100000) // FIXME: No maximum number of the partial reference defined in the specification.
#define RCM_DIR_SIZE                              (8) // This is size of PR dir in RCM. There is no RCM directory.
#define BUCKET_LIST_BUCKETNAME_MAX_SIZE           (63)
#define BUCKET_LIST_BUCKETNAME_MIN_SIZE           (3)

#define MAM_HEADER_SIZE                           (0x4)
#define MAM_PAGE_HEADER_SIZE                      (0x5)
#define MAM_PAGE_COHERENCY                        (0x080C)
#define MAM_PAGE_COHERENCY_SIZE                   (52)

#define MAM_HTA_VENDOR_SIZE                       (8)
#define MAM_HTA_NAME_SIZE                         (32)
#define MAM_HTA_VERSION_SIZE                      (8)
#define MAM_HTA_BARCODE_SIZE                      (32)
#define COMMA_ASCII                               (44)
#define HYPHEN_ASCII                              (45)
#define DOT_ASCII                                 (46)
#define MIN(A, B)                                 ((A) < (B) ? (A) : (B))

#define MAM_VCI_ACSI_VERSION_SIZE                 (1)

/* Relating to marker file path. */
#define FILE_PATH        "reference_partition"
#define SEPARATOR        "/"
#define PR_FILE_PREFIX   "PR_"
#define PR_PATH_PREFIX   (FILE_PATH SEPARATOR PR_FILE_PREFIX)
#define RCM_FILE_PREFIX  "RCM_"
#define LAST_RCM_FILE    "RCM_1"
#define LAST_RCM_PATH    (FILE_PATH SEPARATOR LAST_RCM_FILE)
#define FIRST_RCM_PATH   (FILE_PATH SEPARATOR "RCM_0")
#define VOL1_LABEL_PATH  (FILE_PATH SEPARATOR "VOL1Label")
#define OTF_LABEL_PATH   (FILE_PATH SEPARATOR "OTFLabel")

typedef enum { false, true } Bool;

#define OBJ_READER_MAX_SAVE_NUM      (1000)
#define LARGE_OBJ_SIZE               (1)
/* Bucket info for Object Reader */
typedef struct BucketInfo4ObjReader {
  char bucket_name[BUCKET_LIST_BUCKETNAME_MAX_SIZE + 1];
  int  savepath_dir_number;
  int  savepath_sub_dir_number;
  int  obj_reader_saved_counter;
  struct BucketInfo4ObjReader* next;
} BucketInfo4ObjReader;

/* Structure for LTOS object(Level 0). */ 
typedef struct LTOSObject {
  char object_id[UUID_SIZE + 1];
  uint64_t  meta_data_offset;
  uint64_t object_data_offset;
  char* object_name;
  char* meta_name;
} LTOSObject;

/* Structure for LTOS Label. */
typedef struct LTOSLabel {
  char* ltos_version;
  char* format_time;
  char volume_uuid[UUID_SIZE+1];
  char* creator;
  Bool exists_ltos_version;
  Bool exists_format_time;
  Bool exists_volume_uuid;
  Bool exists_creator;
} LTOSLabel;

/* Structure for bucket list. */
typedef struct BucketList {
  int bucket_name_flag;
  int bucket_id_flag;
} BucketList;

/* Structure for Medium Auxiliary Memory Host-type Attributes. */
typedef struct {
  char is_valid; // When data has valid info, value is TRUE.
  struct {
    char application_vendor[MAM_HTA_VENDOR_SIZE + 1];   // APPLICATION VENDOR
    char application_name[MAM_HTA_NAME_SIZE + 1];       // APPLICATION NAME
    char application_version[MAM_HTA_VERSION_SIZE + 1]; // APPLICATION VERSION
    char system_id[UUID_SIZE + 1];     // First 16 bytes of MEDIUM GLOBALLY UNIQUE IDENTIFIER
    char volume_id[UUID_SIZE + 1];     // Second 16 bytes of MEDIUM GLOBALLY UNIQUE IDENTIFIER
    char pool_id[UUID_SIZE + 1];       // First 16 bytes of MEDIUM POOL GLOBALLY UNIQUE IDENTIFIER
    char pool_group_id[UUID_SIZE + 1]; // Second 16 bytes of MEDIUM POOL GLOBALLY UNIQUE IDENTIFIER
    char barcode[MAM_HTA_BARCODE_SIZE + 1]; // BARCODE formatted in 32 bytes ASCII
  } Data;
} MamHta;

/* Structure for Medium Auxiliary Memory Volume Coherency Information. */
typedef struct MamVci {
  char is_valid;              /* When data has valid info, value is TRUE. */
  struct Data {
    uint64_t volume_change_ref; /* VCR */
    uint64_t pr_count;          /* Partial Reference Count */
    uint64_t rcm_block;         /* Reference Commit Marker block number   */
    char uuid[UUID_SIZE + 1];   /* Volume UUID */
    unsigned char version;      /* Version id */
  } Data;
} MamVci;

typedef enum {
  VOL1_LABEL,
  OTF_LABEL,
  RCM,
  PR,
  OCM,
  PO,
  META,
} MARKER_TYPE;

extern uint64_t pr_num;
extern uint64_t dp_rcm_block_number;

/* Function prototypes */
//void          print_usage(char *appname);
int           check_integrity(MamVci* const mamvci, MamHta* const mamhta, ...);
int           read_marker_file(const uint64_t str_size, const uint64_t str_offset, const char* filepath, char* str);
int           get_pr_num(uint64_t* pr_num);
int           get_ocm_po_meta_num(const int pr_num, uint64_t* ocm_num, uint64_t* po_num, uint64_t* meta_num);
int           get_address_of_marker(MARKER_TYPE m_type, const uint64_t marker_num,
                                    uint64_t* block_number, uint64_t* offset, uint64_t* pr_file_num,
                                    uint64_t* pr_file_offset, uint64_t* marker_len);
int           check_verbose_level(char* verbose_level, char* vorbose);
void          set_obj_save_path(char save_path[OUTPUT_PATH_SIZE + 1]);
void          set_lap_start(time_t lap_s);
void          set_history_interval(uint32_t history_i);
int           get_interval(long int* interval);
int           add_key_value_pairs_to_array_in_json_file(int new_list_flag, FILE* fp_list, const char* json_path, const char* key_value_pairs);
int           make_key_str_value_pairs(char** json_obj, const char* key, const char* value);
int           make_key_ulong_int_value_pairs(char** json_obj, const char* key, const uint64_t value);
int           read_property(const char* file_path, const char* key, char** value);
int           output_history(const char* tape_id, const uint64_t pr_cnt, const uint64_t ocm_cnt, const uint64_t po_cnt, const uint64_t obj_cnt);
int           get_history(const char* tape_id, uint64_t* pr_cnt, uint64_t* ocm_cnt, uint64_t* po_cnt, uint64_t* obj_cnt);
int           initialize_bucket_info_4_obj_reader(BucketInfo4ObjReader** bucket_info_4_obj_reader, char* obj_reader_saveroot);
int           add_bucket_info_4_obj_reader(BucketInfo4ObjReader** bucket_info_4_obj_reader,
                                           char* bucket_name, int obj_reader_saved_counter, int savepath_dir_number, int savepath_sub_dir_number);
int           get_bucket_info_4_obj_reader(BucketInfo4ObjReader** bucket_info_4_obj_reader, char* bucket_name,
                                           int* savepath_dir_number, int*  savepath_sub_dir_number);
int           mk_deep_dir(const char *dirpath);
int           cp_dir(const char *dirpath_from, const char *dirpath_to);
int           extract_dir_path(const char* restrict filepath, char* dirpath);
int           get_element_from_metadata(const char* const meta_data, uint64_t* object_size, char* object_key, char* object_version,
                                        char* last_modified, char* version_id, char* content_md5);
unsigned char *read_file(const char* const name, off_t* const file_size);
int           check_uuid_format(const char* const uuid, const char* const which, const char* const location);
int           check_optional_uuid_format(const char* const uuid, const char* const owner, const char* const location);
int           cpof_write_objects_to_files(const char* const unpackedobjpath, const unsigned char* const data_buf,
                                          const uint64_t* const current_position, const int* const num_of_obj,
                                          const LTOSObject* const objects);
int           cpof_write_an_object_to_a_file(const char* const obj_ext, const char* const unpackedobjpath, const int objnum,
                                                    const unsigned char* const object_buf, const unsigned int object_size,
                                                    const LTOSObject* const objects);
int           check_utc_format(const char* utc);

int           check_partial_reference(const char* const path);
int           clf_header(const char* const identifier, const char* const buffer, const MamHta* const mamhta,
                         int info_flag, uint64_t* const current_position, uint64_t* const num_of_marker,
                         uint64_t* const data_length);
int           is_null_filled(const char* const buffer, const uint64_t length);
int           clf_directory(const char* const identifier, const char* const buffer,
                            uint64_t* const current_position, const uint64_t num_of_marker,
                            uint64_t* const length, uint64_t* const block_offset);

int           clf_vol1_label();
int           clf_ltos_label(const MamVci* const mamvci, const MamHta* const mamhta, uint32_t* block_size);

int           clf_reference_commit_marker(int which_rcm, const MamVci* const mamvci, const MamHta* const mamhta, const uint64_t pr_number);
int           clf_partial_tapes(const char* unpackedobjpath, int which_partition, int pr_num);
int           clf_object_series(const char* unpackedobjpath, int os_top_block_num, int ocmd_length, int ocmd_block_offset);
int           clf_packed_objects(const char* unpackedobjpath, int po_top_block_num, uint64_t poid_length, uint64_t poid_block_offset);
int           clf_packed_objects_info(const char* unpackedobjpath, unsigned char* const data_buf,
                                      uint64_t* const current_position, const uint64_t po_info_length);
int           clf_check_mam_coherency(SCSI_DEVICE_PARAM* const scparam, MamVci* const mamvci, MamHta* const mamhta);
char*         clf_get_partition_name(const int part);
void*         clf_allocate_memory(const size_t size, const char* const object);
int           clf_check_read_data(const size_t actual, const size_t expected,
                                  const char* const object, const char* const filename);
int           clf_close_file(FILE* const stream);
FILE*         clf_open_file(const char* filename, const char* mode);
FILE*         clf_open_alt_file(const char* filename, const char* mode);
size_t        clf_read_file(void* const ptr, const size_t size, const size_t nobj, FILE* const stream);
int           write_object_and_meta_to_file(const char* data, const uint64_t object_size, const uint64_t str_offset, const char* filepath);

int           check_bucket_name(const char* const bucket_name);
int           check_reference_partition_lable(MamVci* const mamvci, MamHta* const mamhta, uint64_t* const total_fm_num_of_rp);
int           write_markers_to_file(const char* restrict filepath, int write_flg);
int           set_marker_file_flg(const int mf_flg);
int           get_marker_file_flg();
int           delete_files_in_directory(const char* const directory_path, const char* const ext);
int           get_tape_generation(void* scparam, char tape_gen[2]);
double        compare_time_string(const char *first_time_string, const char *second_time_string);
int           get_bucket_name(const char *bucket_list, const char *bucket_id, char **bucket_name);
void          free_safely(char ** str);
int           extract_json_element(const char *json_data, const char *json_key, char **json_element);
#endif  /* LTOS_FORMAT_CHECKER_H */

