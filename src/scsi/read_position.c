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
 * @file read_position.c
 * @brief Functions to issue the SCSI command READ POSITION
 */

#include <inttypes.h>
#include "spti_lib.h"

/**
 * SCSI command: READ POSITION - 34h
 *
 * @param  scparam   [i] Control parameter in spti_func (e.g. file descriptor)
 * @param  pos       [o] Position
 * @param  sbp       [o] Sense Buffer Pointer
 * @param  syserr    [o] System Error:          When a SCSI command failed, System error information will be stored in this structure in the future.
 * @return TRUE: success, FALSE: failed
 */
BOOL spti_read_position(void* scparam, ST_SPTI_CMD_POSITIONDATA* pos,
                        ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                        ST_SYSTEM_ERRORINFO* syserr) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:spti_read_position\n");
  static const unsigned char cmd_len = 10;
  unsigned char cmd[cmd_len];
  memset(cmd, 0, cmd_len);
  cmd[0] = 0x34;
  enum { SHORT_FORM = 0x00, LONG_FORM = 0x06, EXTENDED_FORM = 0x08 };
  cmd[1] = LONG_FORM;

  enum { SHORT_SIZE = 20, LONG_SIZE = 32, EXTENDED_SIZE = 32 };
  static const uint32_t dxfer_len = LONG_SIZE;
  unsigned char dxferp[dxfer_len];
  memset(dxferp, 0, sizeof(dxferp));
  uint32_t resid = 0;
  sg_io_hdr_t* const hdr = init_sg_io_hdr(cmd_len, cmd, SG_DXFER_FROM_DEV,
                                          dxfer_len, dxferp, sbp);

  const BOOL rc = run_scsi_command(scparam, hdr, &resid);
  if (rc) {
    pos->partitionNumber = btoui(dxferp +  4, 4);
    pos->blockNumber     = btoui(dxferp +  8, 8);
    pos->fileNumber      = btoui(dxferp + 16, 8);
    pos->bop = (dxferp[0] & 0x80) ? 1 : 0;
    pos->eop = (dxferp[0] & 0x40) ? 1 : 0;
    pos->mpu = (dxferp[0] & 0x08) ? 1 : 0;
    pos->bpu = (dxferp[0] & 0x04) ? 1 : 0;
  }
  destroy_sg_io_hdr(hdr);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO,
                     "end  :spti_read_position: partition=%d, block=%d(0x%X),"
                     "file=%d(0x%X), bop=%d, eop=%d, mpu=%d, bpu=%d\n",
                     pos->partitionNumber, pos->blockNumber, pos->blockNumber,
                     pos->fileNumber, pos->fileNumber, pos->bop, pos->eop, pos->mpu, pos->bpu);
  return rc;
}


/* just for UT */
BOOL test_read_position(const char* const path) {
  SCSI_DEVICE_PARAM* const sdp = init_scsi_device_param(path);
  ST_SPTI_CMD_POSITIONDATA pos = {0};
  ST_SPTI_REQUEST_SENSE_RESPONSE sb = {0};
  ST_SYSTEM_ERRORINFO syserr = {0};
  BOOL rc = spti_read_position(sdp, &pos, &sb, &syserr);
  if (rc) {
    printf("Partition Number: 0x%X\n", pos.partitionNumber);
    printf("Block Number: 0x%" PRIx64 "\n", pos.blockNumber);
    printf("File Number: 0x%" PRIx64 "\n", pos.fileNumber);
    printf("BOP: %d\n", pos.bop);
    printf("EOP: %d\n", pos.eop);
    printf("MPU: %d\n", pos.mpu);
    printf("LONU: %d\n", pos.bpu);
  }

  destroy_scsi_device_param(sdp);
  return rc;
}

