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
 * @file inquiry.c
 * @brief Functions to issue the SCSI command INQUIRY
 */

#include "spti_lib.h"

/**
 * SCSI command: INQUIRY - 12h
 *
 * @param  scparam   [i] Control parameter in spti_func (e.g. file descriptor)
 * @param  page_code [i] Page code (SCSI CDB parameter)
 * @param  dxfer_len [i] Data Transfer Length
 * @param  dxferp    [o] Data Transfer Pointer
 * @param  resid     [o] Residual Count
 * @param  sbp       [o] Sense Buffer Pointer
 * @param  syserr    [o] System Error:          When a SCSI command failed, System error information will be stored in this structure in the future.
 * @return TRUE: success, FALSE: failed
 */
BOOL spti_inquiry(void* scparam, uint32_t page_code, uint32_t dxfer_len,
                  void* dxferp, uint32_t* resid,
                  ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                  ST_SYSTEM_ERRORINFO* syserr) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:spti_inquiry: page_code=0x%02X\n", page_code);
  static const unsigned char cmd_len = 6;
  unsigned char cmd[cmd_len];
  memset(cmd, 0, cmd_len);
  cmd[0] = 0x12;
  cmd[2] = page_code & 0x1f;
  cmd[4] = 0xff;

  sg_io_hdr_t* const hdr = init_sg_io_hdr(cmd_len, cmd, SG_DXFER_FROM_DEV,
                                          dxfer_len, dxferp, sbp);

  const BOOL rc = run_scsi_command(scparam, hdr, resid);
  destroy_sg_io_hdr(hdr);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :spti_inquiry\n");
  return rc;
}


/* just for UT */
// Utility functions
static void extract(const unsigned char* const source,
                    unsigned char* const target,
                    const int length, const int offset) {
  for (int i = 0; i < length; i++) {
    *(target + i) = *(source + offset + i);
  }
}

static void show_str(const char* const label, const unsigned char* const data) {
  printf("%s: _%s_\n", label, data);
}

static void show_int(const char* const label, const int data) {
  printf("%s: _%d_\n", label, data);
}

// T10 VENDOR IDENTIFICATION
#define VENDOR_ID_LENGTH 8
#define VENDOR_ID_OFFSET 8
static void show_vendor_id(const unsigned char* const dxferp) {
  unsigned char vendor_id[VENDOR_ID_LENGTH + 1] = {0};
  extract(dxferp, vendor_id, VENDOR_ID_LENGTH, VENDOR_ID_OFFSET);
  show_str("Vendor ID", vendor_id);
}

// PRODUCT IDENTIFICATION
#define PRODUCT_ID_LENGTH 16
#define PRODUCT_ID_OFFSET 16
static void show_product_id(const unsigned char* const dxferp) {
  unsigned char product_id[PRODUCT_ID_LENGTH + 1] = {0};
  extract(dxferp, product_id, PRODUCT_ID_LENGTH, PRODUCT_ID_OFFSET);
  show_str("Product ID", product_id);
}

// PRODUCT REVISION LEVEL
#define REVISION_LENGTH 4
#define REVISION_OFFSET 32
static void show_revision_level(const unsigned char* const dxferp) {
  unsigned char revision[REVISION_LENGTH + 1] = {0};
  extract(dxferp, revision, REVISION_LENGTH, REVISION_OFFSET);
  show_str("Revision Level", revision);
}

BOOL test_inquiry(const char* const path) {
  SCSI_DEVICE_PARAM* const sdp = init_scsi_device_param(path);
  static const uint32_t dxfer_len = 256 * 1024;
  unsigned char dxferp[dxfer_len];
  memset(dxferp, 0, sizeof(dxferp));
  uint32_t resid = 0;
  ST_SPTI_REQUEST_SENSE_RESPONSE sb = {0};
  ST_SYSTEM_ERRORINFO syserr = {0};
  const BOOL rc = spti_inquiry(sdp, 0, dxfer_len, dxferp, &resid, &sb, &syserr);
  if (rc) {
    show_vendor_id(dxferp);
    show_product_id(dxferp);
    show_revision_level(dxferp);
    show_int("resid", resid);
  }

  destroy_scsi_device_param(sdp);

  return rc;
}

