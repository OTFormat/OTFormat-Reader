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
 * @file locate.c
 * @brief Functions to issue the SCSI command LOCATE
 */

#include "spti_lib.h"

static const uint32_t INVALID_PARTITION_NUMBER = 0xFFFFFFFF;

/**
 * SCSI command: LOCATE(10) - 2Bh
 *
 * @param  scparam       [i] Control parameter in spti_func (e.g. file descriptor)
 * @param  partition     [i] Destination partition
 * @param  block_address [i] Destination block address
 * @param  sbp           [o] Sense Buffer Pointer:
 * @param  syserr        [o] System Error:          When a SCSI command failed, System error information will be stored in this structure in the future.
 * @return TRUE: success, FALSE: failed
 */
BOOL spti_locate_partition(void* scparam, uint32_t partition,
                           uint32_t block_address,
                           ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                           ST_SYSTEM_ERRORINFO* syserr) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO,
                     "start:spti_locate_partition: partition=%X, block=%d(0x%X)\n",
                     partition, block_address, block_address);
  static const unsigned char cmd_len = 10;
  unsigned char cmd[cmd_len];
  memset(cmd, 0, cmd_len);
  cmd[0] = 0x2B;
  cmd[3] = block_address >> 24;
  cmd[4] = block_address >> 16;
  cmd[5] = block_address >> 8;
  cmd[6] = block_address;

  if (partition != INVALID_PARTITION_NUMBER) {
    enum { CHANGE_PARTITION = 0x02, IMMEDIATE = 0x01 };
    cmd[1] |= CHANGE_PARTITION;
    cmd[8] = partition;
  }

  sg_io_hdr_t* const hdr = init_sg_io_hdr(cmd_len, cmd, SG_DXFER_NONE, 0,
                                          NULL, sbp);

  uint32_t resid = 0;
  const BOOL rc = run_scsi_command(scparam, hdr, &resid);
  destroy_sg_io_hdr(hdr);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :spti_locate_partition\n");
  return rc;
}

/**
 * SCSI command: LOCATE
 *
 * @param  scparam       [i] Control parameter in spti_func (e.g. file descriptor)
 * @param  block_address [i] Destination block address
 * @param  sbp           [o] Sense Buffer Pointer
 * @param  syserr        [o] System Error:          When a SCSI command failed, System error information will be stored in this structure in the future.
 * @return TRUE: success, FALSE: failed
 */
BOOL spti_locate(void* scparam, uint32_t block_address,
                 ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                 ST_SYSTEM_ERRORINFO* syserr) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:spti_locate\n");
  const BOOL rc =  spti_locate_partition(scparam, INVALID_PARTITION_NUMBER,
                                         block_address, sbp, syserr);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :spti_locate\n");
  return rc;
}


/* just for UT */
BOOL test_locate(const char* const path, const uint32_t block_address,
                 const char change_partition, const uint32_t partition) {
  SCSI_DEVICE_PARAM* const sdp = init_scsi_device_param(path);
  ST_SPTI_REQUEST_SENSE_RESPONSE sb = {0};
  ST_SYSTEM_ERRORINFO syserr = {0};
  BOOL rc = FALSE;
  if (change_partition) {
    rc = spti_locate_partition(sdp, partition, block_address, &sb, &syserr);
  } else {
    rc = spti_locate(sdp, block_address, &sb, &syserr);
  }
  destroy_scsi_device_param(sdp);

  return rc;
}

