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
 * @file test_unit_ready.c
 * @brief Functions to issue the SCSI command TEST UNIT READY
 */

#include "spti_lib.h"

/**
 * SCSI command: TEST UNIT READY - 00h
 *
 * @param  scparam   [i] Control parameter in spti_func (e.g. file descriptor)
 * @param  sbp       [o] Sense Buffer Pointer
 * @param  syserr    [o] System Error:          When a SCSI command failed, System error information will be stored in this structure in the future.
 * @return TRUE: success, FALSE: failed
 */
BOOL spti_test_unit_ready(void* scparam,
                          ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                          ST_SYSTEM_ERRORINFO* syserr) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:spti_test_unit_ready\n");
  static const unsigned char cmd_len = 6;
  unsigned char cmd[cmd_len];
  memset(cmd, 0, cmd_len);

  sg_io_hdr_t* const hdr = init_sg_io_hdr(cmd_len, cmd, SG_DXFER_NONE, 0,
                                          NULL, sbp);

  uint32_t resid = 0;
  const BOOL rc = run_scsi_command(scparam, hdr, &resid);
  destroy_sg_io_hdr(hdr);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :spti_test_unit_ready\n");
  return rc;
}


/* just for UT */
BOOL test_test_unit_ready(const char* const path) {
  SCSI_DEVICE_PARAM* const sdp = init_scsi_device_param(path);
  ST_SPTI_REQUEST_SENSE_RESPONSE sb = {0};
  ST_SYSTEM_ERRORINFO syserr = {0};
  const BOOL rc = spti_test_unit_ready(sdp, &sb, &syserr);
  if (rc) {
    printf("Test Unit Ready: OK\n");
  }

  destroy_scsi_device_param(sdp);
  return rc;
}

