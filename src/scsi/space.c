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
 * @file space.c
 * @brief Functions to issue the SCSI command SPACE
 */

#include "spti_lib.h"

/**
 * SCSI command: SPACE - 11h
 *
 * @param  scparam       [i] Control parameter in spti_func (e.g. file descriptor)
 * @param  code          [i] Option for SPACE command, 0: Blocks, 1: Filemarks, 3: End of Data
 * @param  block_address [i] Destination block address
 * @param  sbp           [o] Sense Buffer Pointer
 * @param  syserr    [o] System Error:          When a SCSI command failed, System error information will be stored in this structure in the future.
 * @return TRUE: success, FALSE: failed
 */
BOOL spti_space(void* scparam, uint8_t code, uint32_t block_address,
                ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                ST_SYSTEM_ERRORINFO* syserr) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO,
                     "start:spti_space: code=0x%X, block=%d(0x%X)\n",
                     code, block_address, block_address);
  static const unsigned char cmd_len = 6;
  unsigned char cmd[cmd_len];
  memset(cmd, 0, cmd_len);
  cmd[0] = 0x11;
  cmd[1] = code & 0x07; // 1: Filemarks, 3: End of Data
  cmd[2] = block_address >> 16;
  cmd[3] = block_address >> 8;
  cmd[4] = block_address;

  sg_io_hdr_t* const hdr = init_sg_io_hdr(cmd_len, cmd, SG_DXFER_NONE, 0,
                                          NULL, sbp);

  uint32_t resid = 0;
  const BOOL rc = run_scsi_command(scparam, hdr, &resid);
  destroy_sg_io_hdr(hdr);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :spti_space\n");
  return rc;
}


/* just for UT */
BOOL test_space(const char* const path, const uint8_t code, const uint32_t block_address) {
  SCSI_DEVICE_PARAM* const sdp = init_scsi_device_param(path);
  ST_SPTI_REQUEST_SENSE_RESPONSE sb = {0};
  ST_SYSTEM_ERRORINFO syserr = {0};
  const BOOL rc = spti_space(sdp, code, block_address, &sb, &syserr);
  if (rc) {
    printf("Space: OK\n");
  }

  destroy_scsi_device_param(sdp);
  return rc;
}

