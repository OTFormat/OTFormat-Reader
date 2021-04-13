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
 * @file log_sense.c
 * @brief Functions to issue the SCSI command LOG SENSE
 */

#include <inttypes.h>
#include "spti_lib.h"

/**
 * SCSI command: LOG SENSE - 4Dh
 *
 * @param  scparam   [i] Control parameter in spti_func (e.g. file descriptor)
 * @param  page_code [i] Page code (SCSI CDB parameter)
 * @param  parameter [i] SCSI CDB parameter
 * @param  dxfer_len [i] Data Transfer Length
 * @param  dxferp    [o] Data Transfer Pointer
 * @param  resid     [o] Residual Count
 * @param  sbp       [o] Sense Buffer Pointer
 * @param  syserr    [o] System Error:          When a SCSI command failed, System error information will be stored in this structure in the future.
 * @return TRUE: success, FALSE: failed
 */
BOOL spti_log_sense(void* scparam, uint32_t page_code, uint32_t parameter,
                    uint32_t dxfer_len, void* dxferp, uint32_t* resid,
                    ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                    ST_SYSTEM_ERRORINFO *syserr) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO,
                     "start:spti_log_sense: page=0x%02X, parameter=0x%04X\n",
                     page_code, parameter);
  static const unsigned char cmd_len = 10;
  unsigned char cmd[cmd_len];
  memset(cmd, 0, cmd_len);
  cmd[0] = 0x4D;
  cmd[2] = 0x40 | (0x3F & page_code);
//  enum { LIFETIME_VALUES = 0x80, ADVANCED_COUNTERS = 0x20, BASIC_COUNTERS = 0x10 }; // for LP 37h
//  const unsigned char subpage_code = ADVANCED_COUNTERS;
//  cmd[3] = subpage_code;
  cmd[5] = parameter >> 8;
  cmd[6] = parameter;
  cmd[7] = dxfer_len >> 8;
  cmd[8] = dxfer_len;

  sg_io_hdr_t* const hdr = init_sg_io_hdr(cmd_len, cmd, SG_DXFER_FROM_DEV,
                                          dxfer_len, dxferp, sbp);

  const BOOL rc = run_scsi_command(scparam, hdr, resid);
  destroy_sg_io_hdr(hdr);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :spti_log_sense\n");
  return rc;
}


/* just for UT
static void show_lp00(const uint8_t* const buf, const uint32_t size) {
  printf("page code: 0x%02X\n", buf[0]);
  const uint64_t length = btoui(buf + 2, 2);
  printf("length: 0x%016" PRIx64 "\n", length);
  for (uint64_t i = 0; i < length; i++) {
    printf("code: 0x%02X\n", buf[4 + i]);
  }
}



static void show_lp37(const uint8_t* const buf, const uint32_t size) {
  printf("page code: 0x%02X\n", buf[0]);
  printf("subpage: 0x%02X\n", buf[1]);
  const uint64_t length = btoui(buf + 2, 2);
  printf("length: 0x%016" PRIx64 "\n", length);
  for (int i = 4; i < length + 4;) {
    printf("Parameter Code: 0x%016" PRIx64 "\n", btoui(buf + i, 2));
    i += 2;
    i++;
    const unsigned int l = buf[i++];
    printf("Parameter Length: 0x%02X\n", l);
    printf("Parameter Value: 0x%" PRIx64 "X\n", btoui(buf + i, l));
    i += l;
  }
}


BOOL test_log_sense(const char* const path) {
  SCSI_DEVICE_PARAM* const sdp = init_scsi_device_param(path);
  uint8_t dxferp[0x8000] = {0};
  uint32_t resid = 0;
  ST_SPTI_REQUEST_SENSE_RESPONSE sb = {0};
  ST_SYSTEM_ERRORINFO syserr = {0};
  const BOOL rc = spti_log_sense(sdp, 0x00, 0, sizeof(dxferp), &dxferp,
                                 &resid, &sb, &syserr);
  show_lp00(dxferp, resid);

  destroy_scsi_device_param(sdp);
  return rc;
}

*/
