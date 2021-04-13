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
 * @file check_integrity.c
 *
 * Functions to check integrity of the data partition and the reference partition.
 */

#include <dirent.h>
#include "ltos_format_checker.h"

#define BEGINNING        (-1)
#define END              (1)
#define UNKNOWN          (-1)

uint64_t pr_num                     = 0;
static uint64_t ocm_num             = 0;
static uint64_t po_num              = 0;
static uint64_t meta_num            = 0;
uint64_t dp_rcm_block_number        = 0;
static uint32_t block_size                 = LTOS_BLOCK_SIZE;
static uint64_t last_data_offset           = 0;
static uint64_t last_meta_data_offset      = 0; //meta data offset of the last meta data.
static uint64_t num_of_meta                = 0; //Number of metas(Number of objects).
static uint64_t num_of_meta_cnt            = 0;
static uint64_t po_block_address           = 0;
static int read_marker_file_flag           = 1;
static int sequential_read_flag            = 1;
static int marker_file_flg                 = 0;
static int skip_0_padding_check_flag       = 0;

#ifdef OBJ_READER
static FILE* fp_list;
static int  savepath_dir_number                       = 1;
static int  pre_savepath_dir_number                   = 0;
static int  savepath_sub_dir_number                   = 1;
//static int  obj_reader_saved_counter                  = 0;
static char obj_r_mode[OBJ_READER_MODE_LENGTH]        = {0};
static char obj_reader_saveroot[MAX_PATH + 1]         = { 0 };
static char bucket_id_for_obj_r[UUID_SIZE + 1]        = { 0 };
static char* bucket_name_for_obj_r                    = NULL;
static char* pre_bucket_name_for_obj_r                = NULL;
static char* bucket_list_for_obj_r                    = NULL;
static BucketInfo4ObjReader* bucket_info_4_obj_reader = NULL;
static char barcode_id[BARCODE_SIZE + 1]              = DEFAULT_BARCODE;
static SCSI_DEVICE_PARAM scparam                      = { 0 };
static char* object_meta_for_json                     = NULL;
static object_list *objects                           = NULL;
#endif


/**
 * Set marker_file_flg
 * @param [in] (mf_flg)  1:marker file exists. 0:marker file does not exists.
 */
int set_marker_file_flg(const int mf_flg) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:set_marker_file_flg\n");
  marker_file_flg = mf_flg;
  return ret;
}

/**
 * Get marker_file_flg
 */
int get_marker_file_flg() {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_marker_file_flg\n");
  return marker_file_flg;
}

/**
 * Delete marker files in "reference_partition/".
 */
static int initialize_marker_files() {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:initialize_marker_files\n");

  if (get_marker_file_flg() == OFF) {
    if (delete_files_in_directory(FILE_PATH SEPARATOR, NULL) == NG){
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to delete marker files in \"reference_partition\".\n");
    }
  }
//  if ((dp = opendir(FILE_PATH SEPARATOR)) == NULL) {
//    if (errno == ENOENT) {
//      return ret;
//    } else {
//      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to open directory(%s).\n", FILE_PATH SEPARATOR);
//    }
//  }
//  while ((ent = readdir(dp)) != NULL) {
//    sprintf(filepath, "%s%s%s",FILE_PATH, SEPARATOR, ent->d_name);
//    if (remove(filepath) != 0) {
//      if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0)) {
//        continue;
//      }
//      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to remove file(%s).\n", filepath);
//    }
//  }
//  closedir(dp);

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :initialize_marker_files\n");
  return ret;
}

/**
 * Delete any files in the specified directory.
 * @param [in]  (directory_path) A directory in which you want to delete files.
 * @param [in]  (ext) An extension of the file you want to delete.
 * @return      (OK) return OK if all of the files are deleted w/o errors.
 */
int delete_files_in_directory(const char* const directory_path, const char* const ext) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:delete_files_in_directory\n");
  DIR* dp = NULL;
  char filepath[OUTPUT_PATH_SIZE + 1] = "";
  struct dirent* ent;

  if ((dp = opendir(directory_path)) == NULL) {
    if (errno == ENOENT) {
      return ret;
    } else {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to open directory(%s).\n", directory_path);
    }
  }
  while ((ent = readdir(dp)) != NULL) {
    int delete_flag = 0;
    if (ext == NULL) {
    	delete_flag = 1;
    } else if (strcmp(ent->d_name + strlen(ent->d_name) - strlen(ext), ext) == 0) {
    	delete_flag = 1;
    }
    sprintf(filepath, "%s%s", directory_path, ent->d_name);
    if (delete_flag == 1) {
      if (remove(filepath) != 0) {
        if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0)) {
          continue;
        }
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to remove file(%s).\n", filepath);
      }
    }
  }
  closedir(dp);
  dp = NULL;

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :delete_files_in_directory\n");
  return ret;
}

/**
 * Get partition name
 * @param [in]  (whitch_partition) REFERENCE_PARTITION/DATA_PARTITION
 * @return      ("reference partition"/"data partition"/"unknown")
 */
static char* get_partition_name(const int whitch_partition) {
  return whitch_partition == REFERENCE_PARTITION ? "reference partition" : (whitch_partition == DATA_PARTITION ? "data partition" : "unknown");
}

/**
 * Get marker name
 * @param [in]  (m_type) OCM/PO/META
 * @return      ("ocm"/"po"/"meta"/"unknown")
 */
static char* get_marker_name(const MARKER_TYPE m_type) {
  return m_type == OCM ? "ocm" : (m_type == PO ? "po" : (m_type == META ? "meta" : "unknown"));
}

/**
 * Move to the last reference commit marker.
 * @param [in]  (mamvci)          Pointer of a volume coherency information.
 * @param [in]  (which_partition) Data Partition:0, Reference Partition:1
 * @param [out] (fileNumber)      Pointer of a total file number.
 */
static int move_to_last_rcm(MamVci* const mamvci, const int which_partition, uint64_t* fileNumber) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:move_to_last_rcm\n");

  ST_SPTI_CMD_POSITIONDATA pos_rcm_located_at_just_before_EOD = { 0 };
  ST_SPTI_CMD_POSITIONDATA pos_EOD                            = { 0 };

  if (mamvci == NULL || (which_partition != DATA_PARTITION && which_partition != REFERENCE_PARTITION)) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at move_to_last_rcm: mamvci = %p, which_partition = %d\n",mamvci, which_partition);
  }
  if (set_tape_head(which_partition) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Can't locate to beginning of partition %d.\n", which_partition);
    return NG;
  }
  // Follow FM and locate to RCM.
  if (move_on_tape(SPACE_EOD_MODE, 0) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to space to EOD.\n");
  }
  if (read_position_on_tape(&pos_EOD)) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read position of EOD.\n");
  }
  if (fileNumber) {
    *fileNumber = pos_EOD.fileNumber;
  }
  if (move_on_tape(SPACE_FILE_MARK_MODE, -2) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to space to just before FM of the last reference commit marker.\n");
  }
  if (move_on_tape(SPACE_FILE_MARK_MODE, 1) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to space to just after FM of the last reference commit marker.\n");
  }
  if (read_position_on_tape(&pos_rcm_located_at_just_before_EOD) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read position of just after FM of the last reference commit marker.\n");
  }
  if (mamvci[which_partition].is_valid && mamvci[which_partition].Data.rcm_block != pos_rcm_located_at_just_before_EOD.blockNumber) {
    ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_HEADER_AND_L4_INFO,
        "The position of the last reference commit marker in the %s is inconsistent with rcm_block in mam.\n"
        "%sThe position of the last reference commit marker : %lu\n"
        "%srcm_block in mam                                 : %lu\n"
        ,get_partition_name(which_partition)
        ,INDENT, pos_rcm_located_at_just_before_EOD.blockNumber
        ,INDENT, mamvci[which_partition].Data.rcm_block);
  }
  if (which_partition == DATA_PARTITION) {
      dp_rcm_block_number = pos_rcm_located_at_just_before_EOD.blockNumber;
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :move_to_last_rcm\n");
  return ret;
}

/**
 * Check if there is a file mark at the end of the marker.
 * @param [in] (pos)      Position of the file mark.(BEGINNING/END)
 * @param [in] (own_flag) Whether the tape head is on(ON) the target object or just before(OFF) the target object.(ON/OFF)
 */
static int check_fm_next_to_marker(const int pos, const int own_flag) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:check_fm_next_to_marker\n");
  ST_SPTI_CMD_POSITIONDATA pos_now = { 0 };
  ST_SPTI_CMD_POSITIONDATA pos_next_fm = { 0 };

  if (own_flag == ON) {
    if (read_position_on_tape(&pos_now) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read position.\n");
    }
    if (locate_to_tape(pos_now.blockNumber -1) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to space.\n");
    }
  }
  if (read_position_on_tape(&pos_now) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read position.\n");
  }
  if (move_on_tape(SPACE_FILE_MARK_MODE, pos) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to space.\n");
  }
  if (read_position_on_tape(&pos_next_fm) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read position.\n");
  }
  if (pos_now.blockNumber + pos != pos_next_fm.blockNumber) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "There is no file mark next to the marker.\n");
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :check_fm_next_to_marker\n");
  return ret;
}

/**
 * Write markers to file.
 * @param [in] (filepath)     File path.
 * @param [in] (write_flg)       Whether skip writing marker file or not.
 */
int write_markers_to_file(const char* restrict filepath, int write_flg) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:write_markers_to_file(%s)\n", filepath);

  if (marker_file_flg == ON) {
    write_flg = OFF;
    //return ret;
  }

  struct stat stat_buf    = { 0 };
  uint32_t residual_cnt   = 0;
  uint32_t buff_size      = 0;

  if (filepath == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at write_markers_to_file: filename = %p, data_pointer = %p\n",filepath);
  }

  char* dirpath = (char*)clf_allocate_memory(strlen(filepath), "dirpath");
  extract_dir_path(filepath, dirpath);
  if(stat(dirpath, &stat_buf) != OK) {
    if(mkdir(dirpath, stat_buf.st_mode) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to make directory.\n");
    }
  }
  free(dirpath);
  dirpath = NULL;
  FILE* fp =fopen(filepath, "ab");
  if (fp == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to open file.\n");
  }
  while(1) {
    unsigned char* read_buf = (unsigned char*)clf_allocate_memory(block_size, "read_buf");
    if (read_data(block_size, read_buf, &residual_cnt) == NG) {
      if (check_fm_next_to_marker(END, ON) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read the reference partition.\n");
      }
      free(read_buf);
      read_buf = NULL;
      break;
    }
    buff_size = residual_cnt;
    if (write_flg == ON) {
      if (fwrite(read_buf, 1, buff_size, fp) != buff_size) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to write marker file(%s).\n", filepath);
      }
    }
    free(read_buf);
    read_buf = NULL;
  }
  if (fclose(fp) == EOF) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to close file.\n");
  }
  fp = NULL;
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :write_markers_to_file(%s)\n", filepath);
  return ret;
}

/**
 * Read marker file.
 * @param [in]  (str_size)   Binary data size to read.
 * @param [in]  (str_offset) From where to start reading the binary data.
 * @param [in]  (filepath)   File path of Marker file.
 * @param [out] (str)        Binary data of marker file.
 */
int read_marker_file(const uint64_t str_size, const uint64_t str_offset, const char* filepath, char* str) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:read_marker_file(%s) size=%lu offset=%lu\n",
                               filepath, str_size, str_offset);

  FILE* fp =clf_open_alt_file(filepath, "rb");

  if (fseek(fp, str_offset, SEEK_SET) != 0) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed at fseek() while reading file(%s).\n", filepath);
  }
  clf_read_file(str, str_size, 1, fp);
  clf_close_file(fp);
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :read_marker_file(%s)\n", filepath);
  return ret;
}

/**
 * Check if there is no difference between the marker file and the data on the data partition.
 * @param [in] (m_type)         Marker type.(OCM/PO/META)
 * @param [in] (filepath)       File path of the marker file.
 * @param [in] (read_size)      Data size to check difference.
 * @param [in] (offset)         Offset from current block to the current logical position.
 * @param [in] (pr_file_offset) Offset of the marker file from the beginning to the target marker position.
 */
static int check_diff_btwn_file_and_tape(const MARKER_TYPE m_type, const char* filepath, const uint64_t read_size,
                                         uint64_t offset, const uint64_t pr_file_offset) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:check_diff_btwn_file_and_tape(%s)\n",
                               filepath);

  uint32_t residual_cnt = 0;
  struct stat stat_buf  = { 0 };
  uint64_t readed_size  = 0;
  int identifier_flg    = ON;
  int top_flg           = ON;
  int read_fin_flg      = OFF;
  char* meta_data       = (char*)clf_allocate_memory(META_MAX_SIZE, "meta_data");
  uint64_t meta_data_offset = 0;
  int meta_first_block_flag = 1;
  int data_first_block_flag = 1;

  if (filepath == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at write_markers_to_file: filepath = %p\n",filepath);
  }
  if ((stat(filepath, &stat_buf) != OK) && read_marker_file_flag == 1) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to get the file(%s) size.\n", filepath);
  }

  while(read_fin_flg == OFF) {
    char* tape_data = (char*)clf_allocate_memory(block_size, "tape_data");
    char* file_data = (char*)clf_allocate_memory(block_size, "file_data");
    if (read_data(block_size, tape_data, &residual_cnt) == NG) {
      if (check_fm_next_to_marker(END, ON) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to read data from tape.\n");
      }
      ST_SPTI_CMD_POSITIONDATA pos_now = { 0 };
      if (read_position_on_tape(&pos_now) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read position.\n");
      }
      if (locate_to_tape(pos_now.blockNumber -1) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to space.\n");
      }
      if ((uint64_t)stat_buf.st_size != readed_size) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "The size is different between the marker on the data partition and the marker file(%s).\n"
                                                         "%sThe size of the marker on the data partition:%d\n"
                                                         "%sThe size of the marker file                 :%d\n",
                                                         filepath, INDENT, residual_cnt, INDENT, stat_buf.st_size);
      }
      free(tape_data);
      tape_data = NULL;
      free(file_data);
      file_data = NULL;
      break;
    }
    if (read_marker_file_flag == 1) {
      if (read_marker_file(MIN(block_size, stat_buf.st_size - pr_file_offset - readed_size),
                           pr_file_offset + readed_size, filepath, file_data) == NG) {
          ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to read file(%s).\n", filepath);
      }
    }
    readed_size += residual_cnt;

    if (m_type == VOL1_LABEL || m_type == OTF_LABEL || m_type == RCM || m_type == PR) {
      if (memcmp(tape_data, file_data, residual_cnt) != 0) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT,
                                  "The data partition is inconsistent with marker file(%d:%s).\n",
                                  m_type, filepath);
      }
#ifdef OBJ_READER
      if (m_type == RCM && !memcmp(filepath, LAST_RCM_PATH, sizeof(LAST_RCM_PATH)) && identifier_flg == ON) {
        uint64_t system_info_size__for_obj_r = 0;
        uint64_t data_offset_for_obj_r       = 0;
        uint8_t* rcm_header = (uint8_t*)clf_allocate_memory(RCM_HEADER_SIZE, "PO Header");
        memmove(rcm_header, tape_data + IDENTIFIER_SIZE, RCM_HEADER_SIZE);
        r64(BIG, rcm_header + DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE, &system_info_size__for_obj_r, 1);
        r64(BIG, rcm_header + DIRECTORY_OFFSET_SIZE, &data_offset_for_obj_r, 1);
        free(bucket_list_for_obj_r);
        bucket_list_for_obj_r = NULL;
        bucket_list_for_obj_r = (char*)clf_allocate_memory(system_info_size__for_obj_r, "bucket list");
        memmove(bucket_list_for_obj_r, tape_data + IDENTIFIER_SIZE + data_offset_for_obj_r, system_info_size__for_obj_r);
        free(rcm_header);
        rcm_header = NULL;
        }
#endif
    } else if (m_type == OCM || m_type == PO || m_type == META){
      if (m_type == OCM && identifier_flg == ON) {
        if (memcmp(tape_data, OCM_IDENTIFIER, strlen(OCM_IDENTIFIER)) != 0) {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "The object commit marker identifier on the data partition is not correct.\n");
        }

        readed_size -= strlen(OCM_IDENTIFIER);
        residual_cnt -= strlen(OCM_IDENTIFIER);
        identifier_flg = OFF;
      } else if (m_type == PO && identifier_flg == ON) {
        if (memcmp(tape_data, PO_IDENTIFIER_ASCII_CODE, strlen(PO_IDENTIFIER_ASCII_CODE)) != 0) {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "The packed object identifier on the data partition is not correct.\n");
        }
#ifdef OBJ_READER
        uint8_t* po_header = (uint8_t*)clf_allocate_memory(PO_HEADER_SIZE, "PO Header");
        memmove(po_header, tape_data + PO_IDENTIFIER_SIZE, PO_HEADER_SIZE);
        uuid_unparse(po_header + DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE + NUMBER_OF_OBJECTS_SIZE + PACK_ID_SIZE, bucket_id_for_obj_r);
        free(po_header);
        po_header = NULL;
        free(bucket_name_for_obj_r);
        bucket_name_for_obj_r = NULL;
        bucket_name_for_obj_r = (char*)clf_allocate_memory(BUCKET_LIST_BUCKETNAME_MAX_SIZE + 1, "bucket_name_for_obj_r");
        char *bucket_list = (char*)clf_allocate_memory(strlen(bucket_list_for_obj_r), "bucket_list");
        extract_json_element(bucket_list_for_obj_r, "BucketList", &bucket_list);
        get_bucket_name(bucket_list, bucket_id_for_obj_r, &bucket_name_for_obj_r);
        add_bucket_info_4_obj_reader(&bucket_info_4_obj_reader, bucket_name_for_obj_r, 0, 1, 1);
        free(bucket_list);
        bucket_list = NULL;
#endif
        readed_size -= strlen(PO_IDENTIFIER_ASCII_CODE);
        residual_cnt -= strlen(PO_IDENTIFIER_ASCII_CODE);
        identifier_flg = OFF;
      }
      if (memcmp(tape_data + offset, file_data, MIN(block_size - offset, read_size - (readed_size - residual_cnt))) != 0
          && read_marker_file_flag == 1) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT,
                                  "The data partition is inconsistent with marker file(%d:%s).\n",
                                  m_type, filepath);
      }
//      if (read_size != UNKNOWN) {
        if (read_size <= readed_size) {
          if (m_type == OCM && readed_size != read_size) {
            ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT,
                                      "The data partition is inconsistent with marker file(%d:%s).\n",
                                      m_type, filepath);
          }
#ifdef OBJ_READER
        if (m_type == META) {
          memmove(meta_data + meta_data_offset, tape_data + offset, MIN(block_size - offset, read_size - (readed_size - residual_cnt)));
          meta_data_offset += MIN(block_size - offset, read_size - (readed_size - residual_cnt));
          if (offset + read_size > block_size) {
            memset(tape_data, 0, block_size);
              if (read_data(block_size, tape_data, &residual_cnt) == NG) {
              ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to read data from tape.\n");
              }
              memmove(meta_data + meta_data_offset, tape_data, read_size + offset - block_size);
              offset = offset - block_size;
          }
          uint64_t object_size                = 0;
          char object_key[MAX_PATH + 1]       = { 0 };
          char last_modified[MAX_PATH + 1]    = { 0 };
          char version_id[MAX_PATH + 1]       = { 0 };
          char content_md5[MAX_PATH + 1]      = { 0 };
          char object_id[UUID_SIZE + 1]       = { 0 };
          char object_data_path[MAX_PATH + 1] = { 0 };
          char object_meta_path[MAX_PATH + 1] = { 0 };
          get_element_from_metadata(meta_data, &object_size, &object_key[0], &object_id[0], &last_modified[0], &version_id[0], &content_md5[0]);
          if (strncmp(obj_r_mode, "output_list", sizeof("output_list")) == 0) {
            make_key_str_value_pairs(&object_meta_for_json, "object_key", object_key);
            make_key_ulong_int_value_pairs(&object_meta_for_json, "size", object_size);
            make_key_str_value_pairs(&object_meta_for_json, "last_modified", last_modified);
            make_key_str_value_pairs(&object_meta_for_json, "version_id", version_id);
            make_key_str_value_pairs(&object_meta_for_json, "content_md5", content_md5);
            make_key_str_value_pairs(&object_meta_for_json, "object_id", object_id);
          }

          if (LARGE_OBJ_SIZE*pow(1024, 3) <= object_size) {
            if (check_disk_space(obj_reader_saveroot, object_size) == NG) {
              ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "Failed to check the disk space.\n");
            }
          }

          if (strcmp(obj_r_mode , "output_objects_in_object_list") != 0) {
            get_bucket_info_4_obj_reader(&bucket_info_4_obj_reader, bucket_name_for_obj_r, &savepath_dir_number, &savepath_sub_dir_number);
          }

          Bool dir_max_limit_flag = false;
          if (savepath_dir_number > OBJ_READER_MAX_SAVE_NUM) {
        	  dir_max_limit_flag = true;
          }

          if ((strncmp(obj_r_mode, "full_dump", sizeof("full_dump")) == 0)
        		  || (strncmp(obj_r_mode, "resume_dump", sizeof("resume_dump")) == 0)
              || (strncmp(obj_r_mode, "output_objects_in_object_list", sizeof("output_objects_in_object_list")) == 0)){
            sprintf(object_meta_path, "%s/%s/%04d/%04d/%s/%s.meta", obj_reader_saveroot, bucket_name_for_obj_r, savepath_dir_number, savepath_sub_dir_number, object_key, object_id);
            if (meta_first_block_flag == 1) {
              struct stat stat_buf;
              if (stat(object_meta_path, &stat_buf) == OK) {
                free(tape_data);
                tape_data = NULL;
                free(file_data);
                file_data = NULL;
                break;
              }
              meta_first_block_flag = 0;
            }
            if (dir_max_limit_flag != true) {
              mk_deep_dir(object_meta_path);
            }
            if (dir_max_limit_flag != true) {
              write_object_and_meta_to_file(meta_data, strlen(meta_data), 0, object_meta_path);
            }
          }
          free(meta_data);
          meta_data = NULL;

          sprintf(object_data_path, "%s/%s/%04d/%04d/%s/%s.data", obj_reader_saveroot, bucket_name_for_obj_r, savepath_dir_number, savepath_sub_dir_number, object_key, object_id);
          uint64_t remained_tape_data_size = block_size - offset - MIN(block_size - offset, read_size - (readed_size - residual_cnt));
          if ((strncmp(obj_r_mode, "full_dump", sizeof("full_dump")) == 0)
        	  || (strncmp(obj_r_mode, "resume_dump", sizeof("resume_dump")) == 0)
			  || (strncmp(obj_r_mode, "output_objects_in_object_list", sizeof("output_objects_in_object_list")) == 0)){
            if (data_first_block_flag == 1) {
              struct stat stat_buf;
              if (stat(object_data_path, &stat_buf) == OK) {
                free(tape_data);
                tape_data = NULL;
                free(file_data);
                file_data = NULL;
                break;
              }
              data_first_block_flag = 0;
            }
            if (dir_max_limit_flag != true) {
              write_object_and_meta_to_file(tape_data, (uint64_t)(MIN(object_size, remained_tape_data_size)), block_size - remained_tape_data_size, object_data_path);
            }
          }

          object_size -= MIN(object_size, remained_tape_data_size);
          while (0 < object_size) { //In case the object data is on multiple blocks.
              memset(tape_data, 0, block_size);
              if (read_data(block_size, tape_data, &residual_cnt) == NG) {
              if (check_fm_next_to_marker(END, ON) == NG) {
                ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to read data from tape.\n");
              }
            }
           if ((strncmp(obj_r_mode, "full_dump", sizeof("full_dump")) == 0)
       		 || (strncmp(obj_r_mode, "resume_dump", sizeof("resume_dump")) == 0)
               || (strncmp(obj_r_mode, "output_objects_in_object_list", sizeof("output_objects_in_object_list")) == 0)){
        	   if (dir_max_limit_flag != true) {
               write_object_and_meta_to_file(tape_data, (uint64_t)(MIN(object_size, block_size)), 0, object_data_path);
        	   }
           }
           object_size -= MIN(block_size, object_size);
         }

        }
#endif
          if ((m_type == META) && (num_of_meta == num_of_meta_cnt) && (skip_0_padding_check_flag == 0)) {
            // Read large object data to the end block.
            const uint64_t last_obj_size = last_data_offset - last_meta_data_offset;
#ifndef OBJ_READER
            for (int n = 0; n < (offset + last_obj_size) / block_size; n++) {
              memset(tape_data, 0, block_size);
              if (read_data(block_size, tape_data, &residual_cnt) == NG) {
                ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "Failed to read Packed Object\n");
                free(tape_data);
                free(file_data);
                break;
              }
            }
#endif
            // Checked the end of the last block of packed object.
            const uint64_t padding_size = block_size - (offset + last_obj_size) % block_size;
            char* const zero_padding = (char*)clf_allocate_memory(padding_size, "zero_padding");
            if (memcmp(tape_data + (offset + last_obj_size) % block_size, zero_padding, padding_size) != 0) {
              ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "The end of the packed objects must be zero-padded.\n");
            }
            free(zero_padding);
          }
          read_fin_flg = ON;
          free(tape_data);
          tape_data = NULL;
          free(file_data);
          file_data = NULL;
          break;
        }
//     }
    }
    free(tape_data);
    tape_data = NULL;
    free(file_data);
    file_data = NULL;
    if (top_flg == ON) {
      top_flg = OFF;
      offset = 0;
    }
  }
  free(meta_data);
  meta_data = NULL;
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :check_diff_btwn_file_and_tape(%s)\n", filepath);
  return ret;
}

/**
 * Check total filemarks number.
 * @param [in] (total_fm_num) Total number of filemarks.
 * @param [in] (num_of_pr)    Total number of partial references.
 * @param [in] (num_of_ocm)   Total number of object commit marker.
 */
static int check_fm_num(const uint64_t total_fm_num, const uint64_t num_of_pr, const uint64_t num_of_ocm) {
  int ret               = OK;
  const uint32_t fm_num = 4;// Total number of filemarks after vol1 label, otf label, and first and last rcm
  if (total_fm_num != num_of_pr + (num_of_ocm * 2) + fm_num) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "Total number of filemarks is not correct.\n");
  }
  return ret;
}

/**
 * Get number of partial references.
 * @param [out] (pr_num) Number of partial references.
 */
int get_pr_num(uint64_t* pr_num) {
  int ret       = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_pr_num\n");
  int read_size = IDENTIFIER_SIZE + DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE + DATA_LENGTH_SIZE + NUMBER_OF_PARTIAL_REFERENCE_SIZE;
  char* str     = (char*)clf_allocate_memory(read_size, "str");

  if (read_marker_file(read_size, 0, LAST_RCM_PATH, str) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file.\n");
  }
  r64(BIG, (unsigned char *)(str + IDENTIFIER_SIZE + DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE + DATA_LENGTH_SIZE), pr_num, 1);
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :get_pr_num\n");
  free(str);
  str = NULL;
  return ret;
}

/**
 * Check last reference commit marker of reference partition.
 * @param [in] (which_partition) REFERENCE_PARTITION/DATA_PARTITION
 * @param [in]  (mamvci)     Pointer of a volume coherency information.
 * @param [in]  (mamhta)     Pointer of a host-type attributes.
 * @param [out] (fileNumber) Pointer of a total file number.
 */
static int check_last_rcm_integrity(const int which_partition, MamVci* const mamvci, MamHta* const mamhta, uint64_t* fileNumber) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L4_INFO, "start:check_last_rcm_integrity\n");

  if (mamvci == NULL || mamhta == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L4_INFO,
                              "Invalid arguments at check_last_rcm_of_rp: mamvci = %p, mamhta = %p\n",
                              mamvci, mamhta);
  }
  if (move_to_last_rcm(mamvci, which_partition, fileNumber) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "Failed to move to the last reference commit marker.\n");
  }
  if (which_partition == REFERENCE_PARTITION) {
    if (write_markers_to_file(LAST_RCM_PATH, ON) == NG){
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L4_INFO,
                                "Failed to write the last reference commit marker to file.\n");
    }
    if (get_pr_num(&pr_num) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO,
                                "Failed to get the number of partial references.\n");
    }
    if (clf_reference_commit_marker(LAST, mamvci, mamhta, pr_num) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO,
                                "Format violation is detected at the last reference commit marker on Reference Partition.\n");
    }
  } else if (which_partition == DATA_PARTITION) {
    if (check_diff_btwn_file_and_tape(RCM, LAST_RCM_PATH, UNKNOWN, 0, 0) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO,
                                "The data partition is inconsistent with marker file(%s).\n", LAST_RCM_PATH);
    }
    if (check_fm_next_to_marker(END, OFF) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "There is no file mark at the end of the last reference commit marker.\n");
    }
  }

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L4_INFO, "end  :check_last_rcm_integrity\n");
  return ret;
}

/**
 * Check vol1 label of reference partition.
 * @param [in] (which_partition) REFERENCE_PARTITION/DATA_PARTITION
 */
static int check_vol1_label_integrity(const int which_partition) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_INFO, "start:check_vol1_label_integrity\n");

  if (set_tape_head(which_partition) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_INFO,
                              "Can't locate to the beginning of the %s.\n", get_partition_name(which_partition));
    return NG;
  }
  if (which_partition == REFERENCE_PARTITION) {
    if (write_markers_to_file(VOL1_LABEL_PATH, ON) == NG){
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_INFO,
                                "Failed to write vol1 label of the %s.\n", get_partition_name(which_partition));
    }
    if (clf_vol1_label() == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO,
                                "Format violation is detected at vol1 label on Reference Partition.\n");
    }
  } else if (which_partition == DATA_PARTITION) {
    if (check_diff_btwn_file_and_tape(VOL1_LABEL, VOL1_LABEL_PATH, VOL1_LABEL_SIZE, 0, 0) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO,
                                "The data partition is inconsistent with marker file(%s).\n", VOL1_LABEL_PATH);
    }
    if (check_fm_next_to_marker(END, OFF) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "There is no file mark at the end of vol1 label of the %s.\n", get_partition_name(which_partition));
    }
  }

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_INFO, "end  :check_vol1_label_integrity\n");
  return ret;
}

/**
 * Check OTF label of reference partition.
 * @param [in] (which_partition) REFERENCE_PARTITION/DATA_PARTITION
 * @param [in] (mamvci)          Pointer of a volume coherency information.
 * @param [in] (mamhta)          Pointer of a host-type attributes.
 * @param [in] (write_flg)       Whether skip writing marker file or not.
 */
static int check_otf_label_integrity(const int which_partition, MamVci* const mamvci, MamHta* const mamhta, int write_flg) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_INFO, "start:check_otf_label_integrity\n");

  if (mamvci == NULL || mamhta == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_INFO,
                              "Invalid arguments at check_otf_label_of_rp: mamvci = %p, mamhta = %p\n",mamvci, mamhta);
  }
  if (which_partition == REFERENCE_PARTITION) {
    if (write_markers_to_file(OTF_LABEL_PATH, write_flg) == NG){
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_INFO, "Failed to write otf label to file.\n");
    }
    if (clf_ltos_label(mamvci, mamhta, &block_size) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO,
                                "Format violation is detected at OTF label on Reference Partition.\n");
    }
  } else if (which_partition == DATA_PARTITION) {
    if (check_diff_btwn_file_and_tape(OTF_LABEL, OTF_LABEL_PATH, UNKNOWN, 0, 0) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO,
                                "The data partition is inconsistent with marker file(%s).\n", OTF_LABEL_PATH);
    }
    if (check_fm_next_to_marker(END, OFF) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "There is no file mark at the end of otf label.\n");
    }
  }

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_INFO, "end  :check_otf_label_integrity\n");
  return ret;
}

/**
 * Check first reference commit marker of reference partition.
 * @param [in] (which_partition) REFERENCE_PARTITION/DATA_PARTITION
 * @param [in] (mamvci) Pointer of a volume coherency information.
 * @param [in] (mamhta) Pointer of a host-type attributes.
 */
static int check_first_rcm_integrity(const int which_partition, MamVci* const mamvci, MamHta* const mamhta) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L4_INFO, "start:check_first_rcm_integrity\n");

  if (mamvci == NULL || mamhta == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L4_INFO,
                              "Invalid arguments at check_first_rcm_of_rp: mamvci = %p, mamhta = %p\n",mamvci, mamhta);
  }
  if (which_partition == REFERENCE_PARTITION) {
    if (write_markers_to_file(FIRST_RCM_PATH, ON) == NG){
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L4_INFO,
                                "Failed to write the first reference commit marker to file.\n");
    }
    if (clf_reference_commit_marker(FIRST, mamvci, mamhta, pr_num) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO,
                                "Format violation is detected at the first reference commit marker on Reference Partition.\n");
    }
  } else if (which_partition == DATA_PARTITION) {
    if (check_diff_btwn_file_and_tape(RCM, FIRST_RCM_PATH, UNKNOWN, 0, 0) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L4_INFO, // FIXME: system error??
                                "The data partition is inconsistent with marker file(%s).\n", FIRST_RCM_PATH);
    }
    if (check_fm_next_to_marker(END, OFF) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "There is no file mark at the end of the first reference commit marker.\n");
    }
  }

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L4_INFO, "end  :check_first_rcm_integrity\n");
  return ret;
}

/**
 * Check partial references.
 * @param [in] (mamvci)        Pointer of a volume coherency information.
 * @param [in] (target_pr_num) What number of pr to check.
 */
static int check_pr_integrity(const int which_partition, MamVci* const mamvci, int target_pr_num, const int last_flag) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO, "start:check_pr_integrity: %d\n",
                               which_partition);

  ST_SPTI_CMD_POSITIONDATA pos_just_after_last_pr = { 0 };
  ST_SPTI_CMD_POSITIONDATA pos_top_of_last_rcm    = { 0 };
  char filepath[100]                              = FILE_PATH SEPARATOR "PR_"; // FIXME Why this variable contains not full path but only file name???
  char char_target_pr_num[100]                    = { 0 };

  if (mamvci == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                              "Invalid arguments at move_to_last_rcm: mamvci = %p\n",mamvci);
  }
  sprintf(char_target_pr_num, "%d", target_pr_num);
  strcat(filepath, char_target_pr_num);
  if (which_partition == REFERENCE_PARTITION) {
    if (write_markers_to_file(filepath, ON) == NG){
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                                "Failed to write partial reference to file.\n");
    }
    {
      if (check_partial_reference(filepath) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                                  "Format violation is detected at Partial Reference #%d on Reference Partition.\n",
                                  target_pr_num);
      }
    }
  } else if (which_partition == DATA_PARTITION) {
    struct stat stat_buf;
    if (stat(filepath, &stat_buf) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                                "Failed to get the file(%s) size.\n", filepath);
    }
    if (check_diff_btwn_file_and_tape(PR, filepath, stat_buf.st_size, 0, 0) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                                "The data partition is inconsistent with marker file(%s).\n", FIRST_RCM_PATH);
    }
    if (check_fm_next_to_marker(END, OFF) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "There is no file mark at the end of partial reference.\n");
    }
  }

  if (last_flag == ON) {
    read_position_on_tape(&pos_just_after_last_pr);
    move_to_last_rcm(mamvci, which_partition, NULL);
    read_position_on_tape(&pos_top_of_last_rcm);
    if (pos_just_after_last_pr.blockNumber != pos_top_of_last_rcm.blockNumber) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                                "The position of just after the last pr is not the same as that of the top of the last reference commit marker.\n"
                                "%sThe block number of just after the last pr                      : %lu\n"
                                "%sThe block number of the top of the last reference commit marker : %lu\n", INDENT, pos_just_after_last_pr.blockNumber,INDENT, pos_top_of_last_rcm.blockNumber);
    }
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO, "end  :check_pr_integrity: %d\n",
                            which_partition);
  return ret;
}

/**
 * Get last data offset from object directory in po info.
 * @param [in]  (filepath)              File path of PR Marker file.
 * @param [in]  (pr_file_offset)        Offset of the marker file from the beginning to the target marker position.
 * @param [in]  (marker_len)            Length of the target marker.
 */
static int get_last_data_offset(const char* filepath, const uint64_t pr_file_offset, const uint64_t marker_len) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_last_data_offset\n");

  char* bin_for_last_data_offset = (char*)clf_allocate_memory(sizeof(uint64_t), "bin_for_last_data_offset");
  if (read_marker_file(sizeof(uint64_t),
                       pr_file_offset + marker_len - sizeof(uint64_t),
                       filepath, bin_for_last_data_offset) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
  }
  r64(BIG, (unsigned char *)(bin_for_last_data_offset), &last_data_offset, 1);
  free(bin_for_last_data_offset);
  bin_for_last_data_offset = NULL;
  char* bin_for_last_meta_offset = (char*)clf_allocate_memory(sizeof(uint64_t), "bin_for_last_meta_offset");
  if (read_marker_file(sizeof(uint64_t),
                       pr_file_offset + marker_len - PO_DIR_SIZE - sizeof(uint64_t) * 2,
                       filepath, bin_for_last_meta_offset) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
  }
  r64(BIG, (unsigned char *)(bin_for_last_meta_offset), &last_meta_data_offset, 1);
  free(bin_for_last_meta_offset);
  bin_for_last_meta_offset = NULL;
  char* bin_for_num_of_meta = (char*)clf_allocate_memory(sizeof(uint64_t), "bin_for_num_of_meta");
  if (read_marker_file(sizeof(uint64_t),
                       pr_file_offset + DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE,
                       filepath, bin_for_num_of_meta) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
  }
  r64(BIG, (unsigned char *)(bin_for_num_of_meta), &num_of_meta, 1);
  free(bin_for_num_of_meta);
  bin_for_num_of_meta = NULL;
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :get_last_data_offset\n");
  return ret;
}

/**
 * Check if ocm info of the data partition and the reference partition is consistent.
 * @param [in] (m_type)         Marker type.(OCM/PO/META)
 * @param [in] (block_number)   Number of logical objects between bop and the current logical position.
 * @param [in] (offset)         Offset from current block to the current logical position.
 * @param [in] (pr_file_num)    Number of marker file(PR_X).
 * @param [in] (pr_file_offset) Offset of the marker file from the beginning to the target marker position.
 * @param [in] (marker_len)     Length of the target marker.
 */
static int check_part_of_pr_integrity(const MARKER_TYPE m_type, const uint64_t block_number, const uint64_t offset,
                                      const uint64_t pr_file_num, const uint64_t pr_file_offset, const uint64_t marker_len) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO, "start:check_part_of_pr_integrity\n");

  ST_SPTI_CMD_POSITIONDATA pos = { 0 };
  char filepath[100]           = { 0 };

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "%s:%lu,%lu,%lu,%lu,%lu\n",
                            get_marker_name(m_type),block_number, offset, pr_file_num, pr_file_offset, marker_len);
  if (m_type == META) {
    ST_SPTI_CMD_POSITIONDATA pos_before_locate = { 0 };
    read_position_on_tape(&pos_before_locate);
    locate_to_tape(block_number);
    read_position_on_tape(&pos);
    if ((pos.blockNumber < pos_before_locate.blockNumber - 1) && sequential_read_flag == 1) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                                "The position of the meta on the data partition is not correct.\n");
    }
  } else if (m_type == OCM) {
    if (check_fm_next_to_marker(END, OFF) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                                "There is no file mark just before the object commit marker.\n");
    }
    num_of_meta_cnt = 0;
  }
  read_position_on_tape(&pos);
  if (pos.blockNumber != block_number) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                              "The actual position %lu of the %s on the data partition is different from expected value %lu.\n",
                              pos.blockNumber, get_marker_name(m_type), block_number);
  }
  sprintf(filepath, "%s%s%s%lu", FILE_PATH, SEPARATOR, "PR_", pr_file_num);
  if (m_type == PO) {
    if (get_last_data_offset(filepath, pr_file_offset, marker_len) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "Can't get the last data offset from file(%s).\n", filepath);
    }
    num_of_meta_cnt = 0;
  }
  if (m_type == META) {
    num_of_meta_cnt += 1;
#ifdef OBJ_READER
    if (strncmp(obj_r_mode, "output_list", sizeof("output_list")) == 0) {
      object_meta_for_json  = (char*)clf_allocate_memory(MAX_PATH, "object_meta_for_json");
    }
#endif
  }
  if (check_diff_btwn_file_and_tape(m_type, filepath, marker_len, offset, pr_file_offset) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO,
                              "The data partition is inconsistent with marker file(%s).\n", FIRST_RCM_PATH);
  }
#ifdef OBJ_READER
  if (strncmp(obj_r_mode, "output_list", sizeof("output_list")) == 0) {
    if (m_type == META) {
      make_key_ulong_int_value_pairs(&object_meta_for_json, "block_address", po_block_address);
      make_key_ulong_int_value_pairs(&object_meta_for_json, "offset", (block_number - po_block_address) * block_size + offset);
      make_key_ulong_int_value_pairs(&object_meta_for_json, "meta_size", marker_len);
      char* list_file_path  = (char*)clf_allocate_memory(MAX_PATH, "list_file_path");
      sprintf(list_file_path, "%s/%s/%s_%04d.lst", obj_reader_saveroot, barcode_id, bucket_name_for_obj_r, savepath_dir_number);
      int mk_fp_flag = 0;
      int new_list_flag = 0;
      if (pre_bucket_name_for_obj_r == NULL) {
        mk_fp_flag = 1;
      } else if (!((strcmp(pre_bucket_name_for_obj_r, bucket_name_for_obj_r) == 0) && (pre_savepath_dir_number == savepath_dir_number))) {
    	 mk_fp_flag = 1;
      }
      if (mk_fp_flag == 1) {
        struct stat stat_buf = { 0 };
        char* dirpath  = (char*)clf_allocate_memory(strlen(list_file_path), "dirpath");
        extract_dir_path(list_file_path, dirpath);
        if(stat(dirpath, &stat_buf) != OK) {
          if(mkdir(dirpath, stat_buf.st_mode) != OK) {
            ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to make directory.\n");
          }
        }
        free(dirpath);
        dirpath = NULL;
        if (fp_list != NULL) {
          fclose(fp_list);
          fp_list = NULL;
        }
        if ((fp_list = fopen(list_file_path,"r")) == NULL) {
        	new_list_flag = 1;
        } else {
          fclose(fp_list);
          fp_list = NULL;
        }
        if (savepath_dir_number <= OBJ_READER_MAX_SAVE_NUM) {
          fp_list = fopen(list_file_path,"ab");
        }
      }
      if (savepath_dir_number <= OBJ_READER_MAX_SAVE_NUM) {
        add_key_value_pairs_to_array_in_json_file(new_list_flag, fp_list, list_file_path, object_meta_for_json);
      }
      free(pre_bucket_name_for_obj_r);
      pre_bucket_name_for_obj_r = NULL;
      pre_bucket_name_for_obj_r = (char*)clf_allocate_memory(BUCKET_LIST_BUCKETNAME_MAX_SIZE + 1, "pre_bucket_name_for_obj_r");
      strcpy(pre_bucket_name_for_obj_r, bucket_name_for_obj_r);
      pre_savepath_dir_number = savepath_dir_number;
      free(object_meta_for_json);
      object_meta_for_json = NULL;
      free(list_file_path);
      list_file_path = NULL;
    }
  }
#endif
  if (m_type == OCM) {
    if (check_fm_next_to_marker(END, OFF) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "There is no file mark at the end of the object commit marker.\n");
    }
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO, "end  :check_part_of_pr_integrity\n");
  return ret;
}

/**
 * Get meta num in a ocm info.
 * @param [in]  (str)               Binary data of PR file.
 * @param [in]  (ocm_h_data_offset) Data offset of ocm header.
 * @param [in]  (part_of_po_num)    Number of po in target ocm.
 * @param [out] (meta_num)          Number of meta in a ocm info.
 */
static int get_meta_num_in_ocm_info(const char* str,
                                const uint64_t ocm_h_data_offset,
                                const uint64_t part_of_po_num,
                                uint64_t* meta_num) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO, "start:get_meta_num_in_ocm_info\n");

  uint64_t part_of_meta_num        = 0;
  uint64_t po_info_length          = 0;
  uint64_t po_info_offset          = 0;
  uint64_t offset_to_taget_po_info = 0;

  for (uint64_t i = 0; i < part_of_po_num; i++) {
    offset_to_taget_po_info = ocm_h_data_offset + po_info_offset;
    r64(BIG, (unsigned char *)(str + offset_to_taget_po_info + DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE), &part_of_meta_num, 1);
    *meta_num += part_of_meta_num;
    r64(BIG, (unsigned char *)(str + OCM_HEADER_SIZE + (OCM_DIR_SIZE * i)), &po_info_length, 1);
    po_info_offset += po_info_length;
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO, "end  :get_meta_num_in_ocm_info\n");
  return ret;
}

/**
 * Get number of object commit markers.
 * @param [in]  (pr_num)   Number of partial references.
 * @param [out] (ocm_num)  Number of object commit markers.
 * @param [out] (po_num)   Number of packed objects.
 * @param [out] (meta_num) Number of meta datas.
 */
int get_ocm_po_meta_num(const int pr_num, uint64_t* ocm_num, uint64_t* po_num, uint64_t* meta_num) {
  int ret                    = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_ocm_po_meta_num\n");

  for (int i = 0; i < pr_num; i++) {
    uint64_t ocm_info_offset = 0; // Sum of ocm_info_length.

    static const int pr_top_info_read_size  = IDENTIFIER_SIZE + PR_HEADER_SIZE; // Size of PR Header w/ identifier.
    char* pr_top_info_str = (char*)clf_allocate_memory(pr_top_info_read_size, "pr_top_info_str");

    char filepath[100] = {0};
    sprintf(filepath, "%s%d", PR_PATH_PREFIX, i);
    if (read_marker_file(pr_top_info_read_size, 0, filepath, pr_top_info_str) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file.\n");
    }

    uint64_t part_of_ocm_num = 0; // Value of Number of OCM in PR Header.
    r64(BIG, (unsigned char *)(pr_top_info_str + IDENTIFIER_SIZE + DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE),
        &part_of_ocm_num, 1);
    *ocm_num += part_of_ocm_num;
    ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "pr%d: ocm_num=%lu, added=%lu\n",
                              i, *ocm_num, part_of_ocm_num);

    uint64_t pr_h_data_offset = 0; // Value of Data offset in PR Header.
    r64(BIG, (unsigned char *)(pr_top_info_str + IDENTIFIER_SIZE + DIRECTORY_OFFSET_SIZE), &pr_h_data_offset, 1);
    free(pr_top_info_str);
    pr_top_info_str = NULL;

    for (uint64_t j = 0; j < part_of_ocm_num; j++) {
      //get ocm_info_length.
      char* ocm_dir_str = (char*)clf_allocate_memory(sizeof(uint64_t), "ocm_dir_str");
      if (read_marker_file(sizeof(uint64_t), IDENTIFIER_SIZE + PR_HEADER_SIZE + PR_DIR_SIZE * j, filepath,
                           ocm_dir_str) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file.\n");
      }

      uint64_t ocm_info_length = 0; // Value of OCM Length in OCM Directory.
      r64(BIG, (unsigned char *)(ocm_dir_str), &ocm_info_length, 1);
      free(ocm_dir_str);
      ocm_dir_str = NULL;

      char* ocm_info_str = (char*)clf_allocate_memory(ocm_info_length, "ocm_info_str");
      if (read_marker_file(ocm_info_length, IDENTIFIER_SIZE + pr_h_data_offset + ocm_info_offset, filepath,
                           ocm_info_str) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file.\n");
      }

      uint64_t part_of_po_num = 0; // Value of Number of Packed Objects in OCM Header.
      r64(BIG, (unsigned char *)(ocm_info_str + DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE), &part_of_po_num, 1);

      uint64_t ocm_h_data_offset = 0; // Value of Data offset in OCM Header.
      r64(BIG, (unsigned char *)(ocm_info_str + DIRECTORY_OFFSET_SIZE), &ocm_h_data_offset, 1);
      *po_num += part_of_po_num;
      ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "pr%d: ocm%d: po_num=%lu, added=%lu\n",
                                i, j, *po_num, part_of_po_num);
      get_meta_num_in_ocm_info(ocm_info_str, ocm_h_data_offset, part_of_po_num, meta_num);
      ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "pr%d: ocm%d: meta_num=%lu\n",
                                i, j, *meta_num);
      free(ocm_info_str);
      ocm_info_str = NULL;

      ocm_info_offset += ocm_info_length;
    }
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :get_ocm_po_meta_num\n");
  return ret;
}

/**
 * Get address of partial reference.
 * @param [in]  (pr_num)      Number of partial references.
 * @param [out] (blockNumber) Number of logical objects between bop and the current logical position.
 * @param [out] (offset)      Offset from current block to the current logical position.
 */
static int get_address_of_pr(const int pr_num, uint64_t* block_number, uint64_t* offset) {
  int ret           = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO, "start:get_address_of_pr\n");
  uint64_t pr_block = 0;

  if (dp_rcm_block_number == 0) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to get block number of the last reference commit marker of data partition.\n");
  }
  char* str = (char*)clf_allocate_memory(sizeof(uint64_t), "str");
  if (read_marker_file(sizeof(uint64_t), IDENTIFIER_SIZE + RCM_HEADER_SIZE + RCM_DIR_SIZE * (pr_num - 1), LAST_RCM_PATH, str) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file.\n");
  }
  r64(BIG, (unsigned char *)(str), &pr_block, 1);
  *offset = 0;
  #ifdef FORMAT_031
  *block_number = pr_block;
  #else
  *block_number = dp_rcm_block_number - pr_block;
  #endif
  free(str);
  str = NULL;

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO, "end  :get_address_of_pr\n");
  return ret;
}

/**
 * Get address of marker.
 * @param [in]  (filepath)                 File path of PR Marker file.
 * @param [in]  (m_type)                   Marker type.(OCM/PO/META)
 * @param [in]  (offset_to_taget_po_info)  Offset from the beginning of the pr identifier to the target po info.
 * @param [in]  (offset_to_taget_ocm_info) Offset from the beginning of the pr identifier to the target ocm info.
 * @param [in]  (marker_num)               Target marker number.
 * @param [in]  (po_ctr)                   Number of po from the beginning of the OCM to the po containing the target marker.
 * @param [in]  (ocm_ctr)                  Number of ocm from the beginning of the PR to the ocm containing the target marker.
 * @param [in]  (pr_ctr)                   Number of pr from the beginning to the pr containing the target marker.
 * @param [in]  (pkg_meta_num)             Number of meta from the beginning to the po just before the po containing the target meta.
 * @param [in]  (pkg_po_num)               Number of po from the beginning to the ocm just before the ocm containing the target marker.
 * @param [in]  (pkg_ocm_num)              Number of ocm from the beginning to the pr just before the pr containing the target marker.
 * @param [out] (block_number)             Number of logical objects between bop and the current logical position.
 * @param [out] (offset)                   Offset from current block to the current logical position.
 * @param [out] (marker_len)               Length of the target marker.
 */
static int get_block_num_and_offset_of_marker(const char* filepath, const MARKER_TYPE m_type,
                                              uint64_t offset_to_taget_po_info, uint64_t offset_to_taget_ocm_info,
                                              const int marker_num,
                                              const int po_ctr, const int ocm_ctr, const int pr_ctr,
                                              const uint64_t pkg_meta_num, const uint64_t pkg_po_num, const uint64_t pkg_ocm_num,
                                              uint64_t* block_number, uint64_t* offset, uint64_t* marker_len) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_block_num_and_offset_of_marker\n");

  char* str_for_marker_len   = NULL;
  char* str_for_ocm          = NULL;
  char* str_for_po           = NULL;
  char* str_for_meta         = NULL;
  char* str_for_po_h_doffset = NULL;
  uint64_t meta_block_offset = 0;
  uint64_t po_block_offset   = 0;
  uint64_t ocm_block_offset  = 0;
  uint64_t pt_block_number   = 0;
  uint64_t pt_offset         = 0;
  uint64_t obj_block_offset  = 0;
  uint64_t po_h_data_offset  = 0;
  const uint64_t offset_to_ocm_info_dir = IDENTIFIER_SIZE + PR_HEADER_SIZE;
  const uint64_t offset_to_po_info_dir  = offset_to_taget_ocm_info + OCM_HEADER_SIZE ;
  const uint64_t offset_to_obj_dir      = offset_to_taget_po_info + PO_HEADER_SIZE;

  if (filepath == NULL || block_number == NULL || offset == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at get_block_num_and_offset_of_marker: filepath = %p, block_number = %p, offset = %p\n",
                              filepath, block_number, offset);
  }
  if (m_type == OCM) {
    str_for_marker_len = (char*)clf_allocate_memory(sizeof(uint64_t), "str_for_marker_len of OCM");
    if (read_marker_file(sizeof(uint64_t),
                         offset_to_ocm_info_dir + ((marker_num - pkg_ocm_num - 1) * PR_DIR_SIZE),
                         filepath, str_for_marker_len) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
    }
    str_for_ocm = (char*)clf_allocate_memory(sizeof(uint64_t), "str_for_ocm of OCM");
    if (read_marker_file(sizeof(uint64_t),
                         offset_to_ocm_info_dir + ((marker_num - pkg_ocm_num - 1) * PR_DIR_SIZE) + OCM_INFO_LENGTH_SIZE,
                         filepath, str_for_ocm) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
    }
    r64(BIG, (unsigned char *)(str_for_marker_len), marker_len, 1);
    r64(BIG, (unsigned char *)(str_for_ocm), &ocm_block_offset, 1);
    free(str_for_marker_len);
    str_for_marker_len = NULL;
    free(str_for_ocm);
    str_for_ocm        = NULL;
  } else if (m_type == PO) {
    str_for_marker_len = (char*)clf_allocate_memory(sizeof(uint64_t), "str_for_marker_len of PO");
    if (read_marker_file(sizeof(uint64_t),
                         offset_to_taget_po_info + LENGTH_DIRECTORY,
                         filepath, str_for_marker_len) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
    }
    str_for_ocm = (char*)clf_allocate_memory(sizeof(uint64_t), "str_for_ocm of PO");
    if (read_marker_file(sizeof(uint64_t),
                         offset_to_ocm_info_dir + ocm_ctr * PR_DIR_SIZE + OCM_INFO_LENGTH_SIZE,
                         filepath, str_for_ocm) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
    }
    str_for_po = (char*)clf_allocate_memory(sizeof(uint64_t), "str_for_po of PO");
    if (read_marker_file(sizeof(uint64_t),
                         offset_to_po_info_dir + ((marker_num - pkg_po_num - 1) * OCM_DIR_SIZE) + LENGTH_DIRECTORY,
                         filepath, str_for_po) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
    }
    r64(BIG, (unsigned char *)(str_for_marker_len), marker_len, 1);
    r64(BIG, (unsigned char *)(str_for_po), &po_block_offset, 1);
    r64(BIG, (unsigned char *)(str_for_ocm), &ocm_block_offset, 1);
    free(str_for_marker_len);
    str_for_marker_len = NULL;
    free(str_for_ocm);
    str_for_ocm        = NULL;
    free(str_for_po);
    str_for_po         = NULL;
  } else if (m_type == META) {
    str_for_po_h_doffset = (char*)clf_allocate_memory(sizeof(uint64_t), "str_for_po_h_doffset of META");
    if (read_marker_file(sizeof(uint64_t),
                         offset_to_taget_po_info + DIRECTORY_OFFSET_SIZE,
                         filepath, str_for_po_h_doffset) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
    }
    str_for_ocm = (char*)clf_allocate_memory(sizeof(uint64_t), "str_for_ocm of META");
    if (read_marker_file(sizeof(uint64_t),
                         offset_to_ocm_info_dir + ocm_ctr * PR_DIR_SIZE + OCM_INFO_LENGTH_SIZE,
                         filepath, str_for_ocm) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
    }
    str_for_po = (char*)clf_allocate_memory(sizeof(uint64_t), "str_for_po of META");
    if (read_marker_file(sizeof(uint64_t),
                         offset_to_po_info_dir + po_ctr * OCM_DIR_SIZE + LENGTH_DIRECTORY,
                         filepath, str_for_po) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
    }
    str_for_meta = (char*)clf_allocate_memory(sizeof(uint64_t) * 2, "str_for_meta of META");
    if (read_marker_file(sizeof(uint64_t) * 2,
                         offset_to_obj_dir + ((marker_num - pkg_meta_num - 1) * PO_DIR_SIZE) + OBJECT_ID_SIZE,
                         filepath, str_for_meta) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
    }


    r64(BIG, (unsigned char *)(str_for_meta), &meta_block_offset, 1);
    r64(BIG, (unsigned char *)(str_for_meta + META_DATA_OFFSET_SIZE), &obj_block_offset, 1);
    *marker_len = obj_block_offset - meta_block_offset;
    r64(BIG, (unsigned char *)(str_for_po), &po_block_offset, 1);
    r64(BIG, (unsigned char *)(str_for_ocm), &ocm_block_offset, 1);
    r64(BIG, (unsigned char *)(str_for_po_h_doffset), &po_h_data_offset, 1);
    free(str_for_po_h_doffset);
    str_for_po_h_doffset = NULL;
    free(str_for_ocm);
    str_for_ocm          = NULL;
    free(str_for_po);
    str_for_po           = NULL;
    free(str_for_meta);
    str_for_meta         = NULL;
  }
  get_address_of_pr(pr_ctr + 1, &pt_block_number, &pt_offset);
  *offset = 0;
  #ifdef FORMAT_031
  if (m_type == OCM) {
    *block_number = 6 + ocm_block_offset; // Impossible to know the ocm block address without reading data partition.
    *offset = strlen(OCM_IDENTIFIER);
  } else if (m_type == PO) {
    *block_number = 6 + po_block_offset; // Impossible to know the ocm block address without reading data partition.
    *offset = strlen(PO_IDENTIFIER_ASCII_CODE);
  } else if (m_type == META) {
    *block_number = 6 + po_block_offset +
        ((strlen(PO_IDENTIFIER_ASCII_CODE) + po_h_data_offset + meta_block_offset) / block_size); // Impossible to know the ocm block address without reading data partition.
    *offset = (strlen(PO_IDENTIFIER_ASCII_CODE) + po_h_data_offset + meta_block_offset) % block_size;
  } else {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at get_address_of_marker: m_type = %d\n", m_type);
  }
  #else
  if (m_type == OCM) {
    *block_number = pt_block_number - ocm_block_offset;
    *offset = strlen(OCM_IDENTIFIER);
  } else if (m_type == PO) {
    *block_number = pt_block_number - ocm_block_offset - po_block_offset;
    *offset = strlen(PO_IDENTIFIER_ASCII_CODE);
  } else if (m_type == META) {
    *block_number = pt_block_number - ocm_block_offset - po_block_offset
                  + ((strlen(PO_IDENTIFIER_ASCII_CODE) + meta_block_offset) / block_size);
    *offset = (strlen(PO_IDENTIFIER_ASCII_CODE) + meta_block_offset) % block_size;
  } else {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at get_address_of_marker: m_type = %d\n", m_type);
  }
  #endif

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :get_block_num_and_offset_of_marker\n");
  return ret;
}

/**
 * Get offset from the beginning of the upper marker containing the target marker to the target marker.
 * @param [in]  (m_type)               Marker type.(OCM/PO/META)
 * @param [in]  (filepath)             File path of PR Marker file.
 * @param [in]  (offset)               Offset from the beginning of the marker file to the directory of the upper marker containing the target marker.
 * @param [in]  (dir_size)             Size of the directory of the upper marker containing the target marker.
 * @param [in]  (marker_num)           Marker number from the upper marker containing the target marker to the target marker.
 * @param [in]  (data_offset)          Offset from the beginning of the binary data of PR file to the target marker data.
 * @param [out] (target_marker_offset) offset from the beginning of the upper marker containing the target marker to the target marker.
 */
static int get_target_marker_offset(const MARKER_TYPE m_type, const char* filepath, const uint64_t offset, const uint64_t dir_size,
                                    const uint64_t marker_num, const uint64_t data_offset,
                                    uint64_t* target_marker_offset) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_target_marker_offset\n");

  uint64_t length           = 0;
  uint64_t meta_data_offset = 0;
  uint64_t obj_data_offset  = 0;
  char* str                 = NULL;

  *target_marker_offset = 0;
  if (m_type == META) {
    for (uint64_t i = 0; i < marker_num - 1; i++) {
      str = (char*)clf_allocate_memory(sizeof(uint64_t) * 2 , "str");
      ret |= output_accdg_to_vl(OUTPUT_DEBUG, DISPLAY_ALL_INFO,
                                "read_marker_file@get_target_marker_offset: 1: %d %lu\n", i, marker_num);
      if (read_marker_file(sizeof(uint64_t) * 2, offset + (dir_size * i) + OBJECT_ID_SIZE, filepath, str) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
      }
      r64(BIG, (unsigned char *)(str), &meta_data_offset, 1);
      r64(BIG, (unsigned char *)(str + META_DATA_OFFSET_SIZE), &obj_data_offset, 1);
      *target_marker_offset += obj_data_offset - meta_data_offset;
      free(str);
      str = NULL;
    }
  } else {
    for (uint64_t i = 0; i < marker_num - 1; i++) {
      str = (char*)clf_allocate_memory(sizeof(uint64_t), "str");
      ret |= output_accdg_to_vl(OUTPUT_DEBUG, DISPLAY_ALL_INFO,
                                "read_marker_file@get_target_marker_offset: 2: %d %lu\n", i, marker_num);
      if (read_marker_file(sizeof(uint64_t), offset + (dir_size * i), filepath, str) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
      }
      r64(BIG, (unsigned char *)(str), &length, 1);
      *target_marker_offset += length;
      free(str);
      str = NULL;
    }
  }
  *target_marker_offset += data_offset;

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :get_target_marker_offset\n");
  return ret;
}

/**
 * Get address of OCM/PO/meta.
 * @param [in]  (m_type)         Marker type.(OCM/PO/META)
 * @param [in]  (marker_num)     Index of marker of the type in a partition. Index of the first marker should be 1.
 * @param [out] (blockNumber)    Number of logical objects between bop and the current logical position.
 * @param [out] (offset)         Offset from current block to the current logical position.
 * @param [out] (pr_file_num)    Number of marker file(PR_X).
 * @param [out] (pr_file_offset) Offset of the marker file from the beginning to the target marker position.
 * @param [out] (marker_len)     Length of the target marker.
 */
int get_address_of_marker(MARKER_TYPE m_type, const uint64_t marker_num,
                                 uint64_t* block_number, uint64_t* offset, uint64_t* pr_file_num,
                                 uint64_t* pr_file_offset, uint64_t* marker_len) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO,
                               "start:get_address_of_marker: m_type=%d, marker_num=%d\n", m_type, marker_num);

  char* str_from_ocm_info_dir       = NULL;
  char* str_from_tgt_ocm_info       = NULL;
  char* str_from_tgt_po_info        = NULL;
  char* str_from_tgt_ocm_info_dir   = NULL;
  char* str_from_tgt_po_info_dir    = NULL;
  uint64_t pr_h_data_offset         = 0;
  uint64_t ocm_h_data_offset        = 0;
  uint64_t ocm_info_length          = 0;
  uint64_t part_of_ocm_num          = 0;
  uint64_t total_num_of_ocm         = 0;
  uint64_t current_num_of_ocm       = 0;
  uint64_t po_h_data_offset         = 0;
  uint64_t po_info_length           = 0;
  uint64_t part_of_po_num           = 0;
  uint64_t total_num_of_po          = 0;
  uint64_t current_num_of_po        = 0;
  uint64_t part_of_meta_num         = 0;
  uint64_t total_num_of_meta        = 0;
  uint64_t current_num_of_meta      = 0;
  uint64_t offset_to_taget_ocm_info = 0;
  uint64_t offset_to_taget_po_info  = 0;
  char filepath[100]                = {0};

  if (block_number == NULL || offset == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at get_address_of_marker: block_number = %p, offset = %p\n",block_number, offset);
  }
  for (uint64_t pt_counter = 0; pt_counter < pr_num; pt_counter++) {
    *pr_file_num = pt_counter;
    sprintf(filepath, "%s%lu", PR_PATH_PREFIX, pt_counter);
    str_from_ocm_info_dir = (char*)clf_allocate_memory(sizeof(uint64_t) * 2, "str_from_ocm_info_dir");
    ret |= output_accdg_to_vl(OUTPUT_DEBUG, DISPLAY_ALL_INFO, "read_marker_file@get_address_of_marker: 1\n");
    if (read_marker_file(sizeof(uint64_t) * 2,
                         IDENTIFIER_SIZE + DIRECTORY_OFFSET_SIZE,
                         filepath, str_from_ocm_info_dir) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
    }
    r64(BIG, (unsigned char *)(str_from_ocm_info_dir + DATA_OFFSET_SIZE), &part_of_ocm_num, 1);
    r64(BIG, (unsigned char *)(str_from_ocm_info_dir), &pr_h_data_offset, 1);
    free(str_from_ocm_info_dir);
    str_from_ocm_info_dir = NULL;
    current_num_of_ocm = total_num_of_ocm;
    total_num_of_ocm += part_of_ocm_num;
    if (m_type == OCM && marker_num <= total_num_of_ocm) {
      if (marker_num <= current_num_of_ocm) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                                  "pt%d marker_num %d should not be smaller than current_num_of_ocm %lu.\n",
                                  pt_counter, marker_num, current_num_of_ocm);
      }
      get_target_marker_offset(OCM, filepath, IDENTIFIER_SIZE + PR_HEADER_SIZE, PR_DIR_SIZE,
                               marker_num - current_num_of_ocm, IDENTIFIER_SIZE + pr_h_data_offset, pr_file_offset);
      get_block_num_and_offset_of_marker(filepath, m_type, 0, 0,
                                         marker_num, 0, 0, pt_counter, 0, 0, current_num_of_ocm,
                                         block_number, offset, marker_len);
      break;
    }
    uint64_t ocm_info_offset          = 0;
    for (uint64_t ocm_counter = 0; ocm_counter < part_of_ocm_num; ocm_counter++) {
      offset_to_taget_ocm_info = IDENTIFIER_SIZE + pr_h_data_offset + ocm_info_offset;
      str_from_tgt_ocm_info = (char*)clf_allocate_memory(sizeof(uint64_t) * 2, "str_from_tgt_ocm_info");
      ret |= output_accdg_to_vl(OUTPUT_DEBUG, DISPLAY_ALL_INFO, "read_marker_file@get_address_of_marker: 2\n");
      if (read_marker_file(sizeof(uint64_t) * 2,
                           offset_to_taget_ocm_info + DIRECTORY_OFFSET_SIZE,
                           filepath, str_from_tgt_ocm_info) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
      }
      r64(BIG, (unsigned char *)(str_from_tgt_ocm_info + DATA_OFFSET_SIZE), &part_of_po_num, 1);
      r64(BIG, (unsigned char *)(str_from_tgt_ocm_info), &ocm_h_data_offset, 1);
      free(str_from_tgt_ocm_info);
      str_from_tgt_ocm_info = NULL;
      current_num_of_po = total_num_of_po;
      total_num_of_po += part_of_po_num;
      if (m_type == PO && marker_num <= total_num_of_po) {
        if (marker_num <= current_num_of_po) {
          ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                                    "pt%d ocm%d marker_num %d should not be smaller than current_num_of_po %lu.\n",
                                    pt_counter, ocm_counter, marker_num, current_num_of_po);
        }
        get_target_marker_offset(PO, filepath, offset_to_taget_ocm_info + OCM_HEADER_SIZE, OCM_DIR_SIZE,
                                 marker_num - current_num_of_po, offset_to_taget_ocm_info + ocm_h_data_offset, pr_file_offset);
        get_block_num_and_offset_of_marker(filepath, m_type, *pr_file_offset, offset_to_taget_ocm_info,
                                           marker_num, 0, ocm_counter, pt_counter, 0, current_num_of_po, current_num_of_ocm,
                                           block_number, offset, marker_len);
        ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :get_address_of_marker in ocm loop\n");
        return ret;
      }
      uint64_t po_info_offset           = 0;
      for (uint64_t po_counter = 0; po_counter < part_of_po_num; po_counter++) {
        offset_to_taget_po_info = offset_to_taget_ocm_info + ocm_h_data_offset + po_info_offset;
        str_from_tgt_po_info = (char*)clf_allocate_memory(sizeof(uint64_t) * 2, "str_from_tgt_po_info");
        ret |= output_accdg_to_vl(OUTPUT_DEBUG, DISPLAY_ALL_INFO, "read_marker_file@get_address_of_marker: 3\n");
        if (read_marker_file(sizeof(uint64_t) * 2,
                             offset_to_taget_po_info + DIRECTORY_OFFSET_SIZE,
                             filepath, str_from_tgt_po_info) == NG) {
          ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
        }
        r64(BIG, (unsigned char *)(str_from_tgt_po_info + DATA_OFFSET_SIZE), &part_of_meta_num, 1);
        r64(BIG, (unsigned char *)(str_from_tgt_po_info), &po_h_data_offset, 1);
        free(str_from_tgt_po_info);
        str_from_tgt_po_info = NULL;
        current_num_of_meta = total_num_of_meta;
        total_num_of_meta += part_of_meta_num;
        if (m_type == META && marker_num <= total_num_of_meta) {
          if (marker_num <= current_num_of_meta) {
            ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                                      "pt%d ocm%d marker_num %d should not be smaller than current_num_of_meta %lu.\n",
                                      pt_counter, ocm_counter, marker_num, current_num_of_meta);
          }
          get_target_marker_offset(META, filepath, offset_to_taget_po_info + PO_HEADER_SIZE, PO_DIR_SIZE,
                                   marker_num - current_num_of_meta, offset_to_taget_po_info + po_h_data_offset,
                                   pr_file_offset);
          get_block_num_and_offset_of_marker(filepath, m_type, offset_to_taget_po_info, offset_to_taget_ocm_info,
                                             marker_num, po_counter, ocm_counter, pt_counter,
                                             current_num_of_meta, current_num_of_po, current_num_of_ocm,
                                             block_number, offset, marker_len);
          ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :get_address_of_marker in po loop\n");
          return ret;
        }
        str_from_tgt_po_info_dir = (char*)clf_allocate_memory(sizeof(uint64_t), "str_from_tgt_po_info_dir");
        ret |= output_accdg_to_vl(OUTPUT_DEBUG, DISPLAY_ALL_INFO, "read_marker_file@get_address_of_marker: 4\n");
        if (read_marker_file(sizeof(uint64_t),
                             offset_to_taget_ocm_info + OCM_HEADER_SIZE + (OCM_DIR_SIZE * po_counter),
                             filepath, str_from_tgt_po_info_dir) == NG) {
          ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
        }
        r64(BIG, (unsigned char *)(str_from_tgt_po_info_dir), &po_info_length, 1);
        free(str_from_tgt_po_info_dir);
        str_from_tgt_po_info_dir = NULL;
        po_info_offset += po_info_length;
      }
      str_from_tgt_ocm_info_dir = (char*)clf_allocate_memory(sizeof(uint64_t), "str_from_tgt_ocm_info_dir");
      ret |= output_accdg_to_vl(OUTPUT_DEBUG, DISPLAY_ALL_INFO, "read_marker_file@get_address_of_marker: 5\n");
      if (read_marker_file(sizeof(uint64_t),
                           IDENTIFIER_SIZE + PR_HEADER_SIZE + PR_DIR_SIZE * ocm_counter,
                           filepath, str_from_tgt_ocm_info_dir) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read file(%s).\n", filepath);
      }
      r64(BIG, (unsigned char *)(str_from_tgt_ocm_info_dir), &ocm_info_length, 1);
      free(str_from_tgt_ocm_info_dir);
      str_from_tgt_ocm_info_dir = NULL;
      ocm_info_offset += ocm_info_length;
    }
  }

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :get_address_of_marker\n");
  return ret;
}

/**
 * Get next marker.
 * @param [in]  (pr_num)      Number of next target pr.
 * @param [in]  (ocm_num)     Number of next target ocm.
 * @param [in]  (po_num)      Number of next target po.
 * @param [in]  (meta_num)    Number of next target meata.
 * @param [out] (m_type)      Marker type.(PR/OCM/PO/META)
 */
static int get_next_marker(const uint64_t pr_num, const uint64_t ocm_num, const uint64_t po_num, const uint64_t meta_num,
                           const uint64_t pr_max, const uint64_t ocm_max, const uint64_t po_max, const uint64_t meta_max,
                           MARKER_TYPE* m_type) {
  int ret                   = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_next_marker\n");
  uint64_t next_pr_b_num    = ULONG_MAX;
  uint64_t next_ocm_b_num   = ULONG_MAX;
  uint64_t next_po_b_num    = ULONG_MAX;
  uint64_t next_meta_b_num  = ULONG_MAX;
  uint64_t next_offset      = 0;
  uint64_t pr_file_num      = 0;
  uint64_t pr_file_offset   = 0;
  uint64_t marker_len       = 0;

  // These variables are used only for trace message.
  static uint64_t pr_count   = 0;
  static uint64_t ocm_count  = 0;
  static uint64_t po_count   = 0;
  static uint64_t meta_count = 0;
  uint64_t count             = 0;

  if (pr_num <= pr_max ) {
    get_address_of_pr(pr_num, &next_pr_b_num, &next_offset);
  }
  if (ocm_num <= ocm_max ) {
    get_address_of_marker(OCM, ocm_num, &next_ocm_b_num, &next_offset, &pr_file_num, &pr_file_offset, &marker_len);
  }
  if (po_num <= po_max ) {
    get_address_of_marker(PO, po_num, &next_po_b_num, &next_offset, &pr_file_num, &pr_file_offset, &marker_len);
  }
  if (meta_num <= meta_max ) {
    get_address_of_marker(META, meta_num, &next_meta_b_num, &next_offset, &pr_file_num, &pr_file_offset, &marker_len);
  }
  if (next_pr_b_num < next_ocm_b_num && next_pr_b_num < next_po_b_num && next_pr_b_num < next_meta_b_num) {
    *m_type = PR;
    count = ++pr_count;
  } else if (next_ocm_b_num < next_po_b_num && next_ocm_b_num < next_meta_b_num) {
    *m_type = OCM;
    count = ++ocm_count;
  } else if (next_po_b_num <= next_meta_b_num) {
    *m_type = PO;
    count = ++po_count;
  } else {
    *m_type = META;
    count = ++meta_count;
  }

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO,
      "end  :get_next_marker: %s:%d pr:%lu,%lu ocm:%lu,%lu po:%lu,%lu meta:%lu,%lu  npr=%lu nocm=%lu npo=%lu, nmeta=%lu\n",
      get_marker_name(*m_type), count, pr_num, pr_max, ocm_num, ocm_max, po_num, po_max, meta_num, meta_max,
      next_pr_b_num, next_ocm_b_num, next_po_b_num, next_meta_b_num);
  return ret;
}

/**
 * Set blocl size.(at clf_ltos_label())
 * @param [in] (mamvci)          Pointer of a volume coherency information.
 * @param [in] (mamhta)          Pointer of a host-type attributes.
 */
static int set_block_size(MamVci* const mamvci, MamHta* const mamhta) {
  int ret = OK;
  if (set_tape_head(REFERENCE_PARTITION) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Can't locate to beginning of the reference partition.\n");
    return NG;
  }
  if (move_on_tape(SPACE_FILE_MARK_MODE, 1) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to space to just before OTF label.\n");
  }
  if (check_otf_label_integrity(REFERENCE_PARTITION, mamvci, mamhta, ON) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "OTF Label format is not correct.\n");
  }
  return ret;
}

/**
 * Check integrity of reference partition and data partition.
 * @param [in] (mamvci) Pointer of a volume coherency information.
 * @param [in] (mamhta) Pointer of a host-type attributes.
 */
int check_integrity(MamVci* const mamvci, MamHta* const mamhta, ...) {
  int ret                     = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:check_integrity\n");
  int last_flag               = OFF;
  uint64_t total_fm_num_of_rp = 0;
  uint64_t total_fm_num_of_dp = 0;
  int first_locate_flag       = OFF;
  int skip_dp_check_flag      = 0;
  skip_0_padding_check_flag   = 0;

#ifdef MONGODB_RESTORE_TOOL
  skip_dp_check_flag = 1;
#endif

#ifdef OBJ_READER
  if (get_marker_file_flg()) {
    first_locate_flag     = ON;
  }
  va_list ap;
  va_start(ap, mamhta);
  sprintf(obj_r_mode, "%s", va_arg(ap, char*));
  scparam = va_arg(ap, SCSI_DEVICE_PARAM);
  sprintf(obj_reader_saveroot, "%s", va_arg(ap, char*));
  sprintf(barcode_id, "%s", va_arg(ap, char*));
  if (strcmp(obj_r_mode , "output_objects_in_object_list") == 0) {
    objects = va_arg(ap, object_list*);
    bucket_name_for_obj_r = va_arg(ap, char*);
    skip_0_padding_check_flag = 1;
  }
  va_end(ap);

  if (strcmp(obj_r_mode , "resume_dump") == 0) {
	  skip_0_padding_check_flag = 1;
  }
  if (strcmp(obj_r_mode , "output_objects_in_object_list") != 0) {
    initialize_bucket_info_4_obj_reader(&bucket_info_4_obj_reader, obj_reader_saveroot);
  } else {
    savepath_dir_number     = 0;
    savepath_sub_dir_number = 0;
  }
  if (strcmp(obj_r_mode , "output_objects_in_object_list") == 0) {
    read_marker_file_flag = 0;
    sequential_read_flag  = 0;
    struct object_list* current      = NULL;
    current = objects;
    if (set_tape_head(DATA_PARTITION) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Can't locate to beginning of partition %d.\n", DATA_PARTITION);
      return NG;
    }
    while(current != NULL) {
      if (check_part_of_pr_integrity(META, current->block_address + (current->meta_offset / block_size), (current->meta_offset) % block_size, 0, 0, current->metadata_size) != OK) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "The partial reference format is not correct.\n");
      }
      current = current->next;
    }
    return ret;
  }
  //if (strncmp(obj_r_mode, "full_dump", sizeof("full_dump")) == 0) {
#endif

  if (marker_file_flg == OFF) {
    if (check_reference_partition_lable(mamvci, mamhta, &total_fm_num_of_rp) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Data stored in Reference Partition is not complying with OTFormat.\n");
    }

    ret |= output_accdg_to_vl(OUTPUT_DEBUG, DISPLAY_HEADER_AND_L43_INFO, "check_integrity: pr_num=%lu\n", pr_num);
    for (uint64_t target_pr_num = 0; target_pr_num < pr_num; target_pr_num++) {
      if (target_pr_num + 1 == pr_num) {
        last_flag = ON;
      }
      if (check_pr_integrity(REFERENCE_PARTITION, mamvci, target_pr_num, last_flag) != OK) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "Partial reference format is not correct.\n");
      }
    }
    if (skip_dp_check_flag == 1) {
      return ret;
    }
    if (check_fm_num(total_fm_num_of_rp, pr_num, 0) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "Total number of filemarks is not correct.\n");
    }
  }
  get_pr_num(&pr_num);
  get_ocm_po_meta_num(pr_num, &ocm_num, &po_num, &meta_num);
  ret |= output_accdg_to_vl(OUTPUT_DEBUG, DISPLAY_HEADER_AND_L43_INFO,
                            "check_integrity: pr_num=%lu ocm_num=%lu po_num=%lu meta_num=%lu\n",
                            pr_num, ocm_num, po_num, meta_num);

  if (check_last_rcm_integrity(DATA_PARTITION, mamvci, mamhta, &total_fm_num_of_dp) != OK) {// Set "dp_rcm_block_number".
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "The last reference commit marker format is not correct.\n");
  }
  if (marker_file_flg == OFF) {
    if (check_vol1_label_integrity(DATA_PARTITION) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Vol1 Label format is not correct.\n");
    }
    if (check_otf_label_integrity(DATA_PARTITION, mamvci, mamhta, OFF) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "OTF Label format is not correct.\n");
    }
    if (check_first_rcm_integrity(DATA_PARTITION, mamvci, mamhta) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "The first reference commit marker format is not correct.\n");
    }
    if (check_fm_num(total_fm_num_of_dp, pr_num, ocm_num) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "Total number of filemarks is not correct.\n");
    }
  }

#ifdef OBJ_READER
  char tape_gen[2] = {0};
  get_tape_generation(&scparam, tape_gen);
  if (read_marker_file(VOLUME_IDENTIFIER_SIZE, LABEL_IDENTIFIER_SIZE + LABEL_NUMBER_SIZE, VOL1_LABEL_PATH, &barcode_id[0]) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to read file(%s).\n", VOL1_LABEL_PATH);
  }
  strncpy(&barcode_id[6], tape_gen, 2);
#endif
  uint64_t pr_cnt              = 1; // Index of Partial Reference in reference partition.
  uint64_t ocm_cnt             = 1; // Index of Object Commit Marker in reference partition.
  uint64_t po_cnt              = 1; // Index of Packed Object in reference partition.
  uint64_t meta_cnt            = 1; // Index of Object Metadata in reference partition.
  uint64_t block_number        = 0; // Number of logical objects between bop and the current logical position.
  uint64_t offset              = 0; // Offset from current block to the current logical position.
  uint64_t pr_file_num         = 0; // Number of marker file(PR_X).
  uint64_t pr_file_offset      = 0; // Offset of the marker file from the beginning to the target marker position.
  uint64_t marker_len          = 0; // Length of the target marker.
  MARKER_TYPE m_type           = VOL1_LABEL;
  ST_SPTI_CMD_POSITIONDATA pos = { 0 };

#ifdef OBJ_READER
  if (strcmp(obj_r_mode , "resume_dump") == 0) {
    if(get_history(barcode_id, &pr_cnt, &ocm_cnt, &po_cnt, &meta_cnt) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "There is no history file.\n%sTry full dump.\n", INDENT);
    }
  }
#endif
  if (!(pr_cnt == 1 && ocm_cnt == 1 && po_cnt == 1 && meta_cnt == 1)) {
    first_locate_flag = ON;
  }
  while (pr_cnt < pr_num + 1 || ocm_cnt < ocm_num + 1 || po_cnt < po_num + 1 || meta_cnt < meta_num + 1) {
    ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "pr:%lu,%lu ocm:%lu,%lu po:%lu,%lu meta:%lu,%lu\n",
                              pr_cnt, pr_num, ocm_cnt, ocm_num, po_cnt, po_num, meta_cnt, meta_num);

    get_next_marker(pr_cnt, ocm_cnt, po_cnt, meta_cnt, pr_num, ocm_num, po_num, meta_num, &m_type);
    if (m_type == PR) {
      get_address_of_pr(pr_cnt, &block_number, &offset);
      if (first_locate_flag == ON) {
        locate_to_tape(block_number);
        first_locate_flag = OFF;
      }
      read_position_on_tape(&pos);
      if (pos.blockNumber != block_number) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "The position of the partial reference on the data partition is not correct.\n");
      }
      if (check_pr_integrity(DATA_PARTITION, mamvci, pr_cnt - 1, pr_cnt == pr_num) != OK) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "The partial reference format is not correct.\n");
      }
      output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "pr   : %lu,%lu\n", block_number, offset);
      pr_cnt++;
    } else if (m_type == OCM) {
      get_address_of_marker(OCM, ocm_cnt, &block_number, &offset, &pr_file_num, &pr_file_offset, &marker_len);
      if (first_locate_flag == ON) {
        locate_to_tape(block_number);
        first_locate_flag = OFF;
      }
      if (check_part_of_pr_integrity(OCM, block_number, offset, pr_file_num, pr_file_offset, marker_len) != OK) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "The partial reference format is not correct.\n");
      }
      ocm_cnt++;
    } else if (m_type == PO) {
      get_address_of_marker(PO, po_cnt, &block_number, &offset, &pr_file_num, &pr_file_offset, &marker_len);
#ifdef OBJ_READER
      if (strncmp(obj_r_mode, "output_list", sizeof("output_list")) == 0) {
        po_block_address = block_number;
      }
#endif
      if (first_locate_flag == ON) {
        locate_to_tape(block_number);
        first_locate_flag = OFF;
      }
      if (check_part_of_pr_integrity(PO, block_number, offset, pr_file_num, pr_file_offset, marker_len) != OK) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "The partial reference format is not correct.\n");
      }
#ifdef OBJ_READER
      output_history(barcode_id, pr_cnt, ocm_cnt, po_cnt, meta_cnt);
#endif
      po_cnt++;
    } else if (m_type == META) {
      get_address_of_marker(META, meta_cnt, &block_number, &offset, &pr_file_num, &pr_file_offset, &marker_len);
      if (first_locate_flag == ON) {
        locate_to_tape(block_number);
        first_locate_flag = OFF;
      }
      if (check_part_of_pr_integrity(META, block_number, offset, pr_file_num, pr_file_offset, marker_len) != OK) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "The partial reference format is not correct.\n");
      }
      meta_cnt++;
    }
  }
#ifdef OBJ_READER
  if (fp_list != NULL) {
    fclose(fp_list);
    fp_list = NULL;
  }
#endif
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :check_integrity\n");

  return ret;
}



/**
 * Check format of the first and second RCM, VOL1Lable, and OTFormatLable in Reference Partition.
 * @param [in] (mamvci) Pointer of a volume coherency information.
 * @param [in] (mamhta) Pointer of a host-type attributes.
 * @param [in] (total_fm_num_of_rp) Pointer to the number of file marks in reference partition.
 */
int check_reference_partition_lable(MamVci* const mamvci, MamHta* const mamhta, uint64_t* const total_fm_num_of_rp) {
  int ret                     = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:check_reference_partition_lable\n");

  if (initialize_marker_files() != OK) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to initialize at check_reference_partition_lable.\n");
  }
  if (mamvci == NULL || mamhta == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at check_format: mamvci = %p, mamhta = %p\n",mamvci, mamhta);
  }
  if (set_block_size(mamvci, mamhta) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Can't get the block size.\n");
  }
  if (check_last_rcm_integrity(REFERENCE_PARTITION, mamvci, mamhta, total_fm_num_of_rp) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "The last reference commit marker format is not correct.\n");
  }
  if (check_vol1_label_integrity(REFERENCE_PARTITION) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Vol1 Label format is not correct.\n");
  }
  if (check_otf_label_integrity(REFERENCE_PARTITION, mamvci, mamhta, OFF) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "OTF Label format is not correct.\n");
  }
  if (check_first_rcm_integrity(REFERENCE_PARTITION, mamvci, mamhta) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4_INFO, "The first reference commit marker format is not correct.\n");
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :check_reference_partition_lable\n");

  return ret;
}


