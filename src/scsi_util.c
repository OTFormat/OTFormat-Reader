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
 * @file scsi_util.c
 *
 * SCSI control utility.
 */

#include "ltos_format_checker.h"


static SCSI_DEVICE_PARAM* scsi_param                = NULL;
static ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data   = NULL;
static ST_SYSTEM_ERRORINFO* err_info                = NULL;


/**
 * Set all pointers which are essential to control a tape drive.
 * @param [in] (scsiparam) Pointer to a structure of SCSI_DEVICE_PARAM
 * @param [in] (sensedata) Pointer to a structure of ST_SPTI_REQUEST_SENSE_RESPONSE
 * @param [in] (errinfo)   Pointer to a structure of ST_SYSTEM_ERRORINFO
 */
void  set_device_pram(SCSI_DEVICE_PARAM* scsiparam, ST_SPTI_REQUEST_SENSE_RESPONSE* sensedata, ST_SYSTEM_ERRORINFO* errinfo) {
  if (scsiparam == NULL || sensedata == NULL || errinfo == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at set_device_pram");
  }
  scsi_param = scsiparam;
  sense_data = sensedata;
  err_info   = errinfo;
}


/**
 * Just a wrapper of "spti_read_data" excluded 3 arguments relating to SCSI control.
 * @param [in] (data_trans_len) Requested data size
 * @param [in] (data_pointer)   Pointer to read data
 * @param [in] (residual_count) Actual data size
 */
int read_data(uint32_t const data_trans_len, void* const data_pointer, uint32_t* const residual_count) {
//  static int c = 0; // just for debug
  int ret = OK;
  if (data_pointer == NULL || residual_count == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at read_data");
  }
//  { // just for Debug
//    ST_SPTI_CMD_POSITIONDATA pos = {0};
//    read_position_on_tape(&pos);
//    output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "read_data: %d at p=%d, b=%lu, f=%lu\n",
//                       c++, pos.partitionNumber, pos.blockNumber, pos.fileNumber);
//  }
  if (spti_read_data(scsi_param, data_trans_len, data_pointer, residual_count, sense_data, err_info) != TRUE) {
    if (sense_data->sense_key == 0 && sense_data->asc == 0 && sense_data->ascq == 1) {
      output_accdg_to_vl(OUTPUT_INFO, DISPLAY_ALL_INFO, "Filemark detected during reading data.\n");
      ret = NG; // Though this is just a warning, return NG to kick check_fm_next_to_marker at caller if needed.
    } else {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "Failed to Read data: %X/%02X/%02X.\n",
                                sense_data->sense_key, sense_data->asc, sense_data->ascq);
    }
  }
  return ret;
}

/**
 * Just a wrapper of "spti_space" excluded 3 arguments relating to SCSI control.
 * @param [in] (code)          Option for SPACE command, 0: Blocks, 1: Filemarks, 3: End of Data
 * @param [in] (block_address) Destination block address
 */
int move_on_tape(const uint8_t code, const uint32_t block_address) {
  int ret = OK;

  if (spti_space(scsi_param, code, block_address, sense_data, err_info) != TRUE) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to move on tape.\n");
  }
  return ret;
}

/**
 * Just a wrapper of "spti_read_position" excluded 3 arguments relating to SCSI control.
 * @param [out] (pos) Position
 */
int read_position_on_tape(ST_SPTI_CMD_POSITIONDATA* pos) {
  int ret = OK;
  if (pos == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at read_position_on_tape");
  }
  if (spti_read_position(scsi_param, pos, sense_data, err_info) != TRUE) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to read position on tape.\n");
  }
  return ret;
}

/**
 * Set tape head.
 * @param [in]  (which_partition)  Data Partition:0, Reference Partition:1
 * @return      (OK/NG)            If the format is correct or not.
 */
int set_tape_head(const int which_partition) {
  int ret = OK;
  if (spti_locate_partition(scsi_param, which_partition, 0, sense_data, err_info) != TRUE) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to locate partition %d.\n", which_partition);
  }
  return ret;
}

/**
 * Just a wrapper of "spti_locate" excluded 3 arguments relating to SCSI control.
 * @param [in]  (block_addres)     Absolute Block address you want to locate to.
 * @return      (OK/NG)            If the format is correct or not.
 */
int locate_to_tape(const uint32_t block_addres) {
  int ret = OK;
  if (spti_locate(scsi_param, block_addres, sense_data, err_info) != TRUE) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to locate to Block address: %d.\n", block_addres);
  }
  return ret;
}


#ifdef OBSOLETE
/**
 * Padding Hexspeaker such as "DEADBEEF" to the buffer.
 * @param [out] (data_buf)         Pointer to a buffer.
 * @param [in]  (data_length)      Padding size
 * @return      (OK/NG)            If process is complete correctly or not.
 */
int set_hexspeak(unsigned char* const data_buf, const int data_length) {
  int ret                              = OK;
  uint32_t buf_pos                     = 0;
  static const unsigned char speaker[] = { 0xDE, 0xAD, 0xBE, 0xEF}; //4byte
  static const uint32_t spk_size       = sizeof(speaker);

  if (data_buf == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at set_hexspeak");
  }
  if ( data_length < spk_size ){
    memmove(data_buf, speaker, data_length);
  } else {
    //Prepare initial data to data_buf
    memmove(data_buf, speaker, spk_size);
    buf_pos += spk_size;
    //Copy itself like Game twice.
    while (buf_pos + buf_pos < data_length) {
      memmove(data_buf + buf_pos, data_buf, buf_pos);
      buf_pos += buf_pos;
    }
    //Terminate
    memmove(data_buf + buf_pos, data_buf, data_length - buf_pos);
  }
  return ret;
}

#endif //OBSOLETE
