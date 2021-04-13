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
 * @file read.c
 * @brief Functions to issue the SCSI command READ
 */

#include "spti_lib.h"

/**
 * SCSI command: READ - 08h
 *
 * @param  scparam   [i] Control parameter in spti_func (e.g. file descriptor)
 * @param  dxfer_len [i] Data Transfer Length
 * @param  dxferp    [o] Data Transfer Pointer
 * @param  resid     [o] Residual Count
 * @param  sbp       [o] Sense Buffer Pointer
 * @param  syserr    [o] System Error:          When a SCSI command failed, System error information will be stored in this structure in the future.
 * @return TRUE: success, FALSE: failed
 */
BOOL spti_read_data(void* scparam, uint32_t dxfer_len, void* dxferp,
                    uint32_t* resid,
                    ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                    ST_SYSTEM_ERRORINFO* syserr) {
  static const unsigned char cmd_len = 6;
  unsigned char cmd[cmd_len];
  memset(cmd, 0, cmd_len);
  cmd[0] = 0x08;
  enum { SILI = 0x02, FIXED = 0x01 };
  cmd[1] = SILI;
  cmd[2] = dxfer_len >> 16;
  cmd[3] = dxfer_len >> 8;
  cmd[4] = dxfer_len;

  sg_io_hdr_t* const hdr = init_sg_io_hdr(cmd_len, cmd, SG_DXFER_FROM_DEV,
                                          dxfer_len, dxferp, sbp);

  const BOOL rc = run_scsi_command(scparam, hdr, resid);
  destroy_sg_io_hdr(hdr);
  return rc;
}


/* just for UT */
BOOL test_read_data(const char* const path) {
  BOOL rc = FALSE;
  SCSI_DEVICE_PARAM* const sdp = init_scsi_device_param(path);
  for (int i = 0; i < 1; i++) {
    uint8_t dxferp[0x80000] = {0};
    uint32_t resid = 0;
    ST_SPTI_REQUEST_SENSE_RESPONSE sb = {0};
    ST_SYSTEM_ERRORINFO syserr = {0};
    rc = spti_read_data(sdp, sizeof(dxferp), &dxferp, &resid, &sb,
                        &syserr);
    printf("read: %d, %d, %d\n", i, rc, resid);
    if (!rc) {
      break;
    }
  }

  destroy_scsi_device_param(sdp);
  return rc;
}

