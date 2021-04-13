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
 * @file sg_io_hdr.c
 * @brief Functions to issue a SCSI command
 */

#include <stdlib.h>
#include <string.h>
#include "spti_lib.h"
#include "output_level.h"

/*
static void set_cmd(sg_io_hdr_t* const hdr, const unsigned char cmd_len,
                    unsigned char* const cmdp) {
  if (hdr) {
    hdr->cmd_len = cmd_len;
    hdr->cmdp    = cmdp;
  }
}

static void set_dxfer(sg_io_hdr_t* const hdr, const int dxfer_direction,
                      void* const dxferp, const unsigned int dxfer_len) {
  if (hdr) {
    // SG_DXFER_NONE: for Test Unit Ready, ...
    // SG_DXFER_TO_DEV: for Write, ...
    // SG_DXFER_FROM_DEV: for Read, ...
    // SG_DXFER_TO_FROM_DEV
    hdr->dxfer_direction = dxfer_direction;

    hdr->dxfer_len = dxfer_len;
    hdr->dxferp    = dxferp;
  }
}

static void set_sb(sg_io_hdr_t* const hdr,
                   ST_SPTI_REQUEST_SENSE_RESPONSE* sbp) {
  if (hdr) {
    hdr->sbp       = (unsigned char*) sbp;
    hdr->mx_sb_len = sizeof(*sbp);
  }
}
*/

/**
 * Create sg_io_hdr_t object
 *
 * @param  cmd_len         [i] Command Length
 * @param  cmdp            [i] Command Pointer
 * @param  dxfer_direction [i] Data Transfer Direction
 * @param  dxfer_len       [i] Data Transfer Length
 * @param  dxferp          [i] Data Transfer Pointer
 * @param  sbp             [i] Sense Buffer Pointer
 * @return Pointer of sg_io_hdr_t object
 */
sg_io_hdr_t* init_sg_io_hdr(const unsigned char cmd_len,
                            unsigned char* const cmdp,
                            const int dxfer_direction,
                            const unsigned int dxfer_len,
                            void* const dxferp,
                            ST_SPTI_REQUEST_SENSE_RESPONSE* sbp) {
  sg_io_hdr_t* const hdr = (sg_io_hdr_t*) calloc(1, sizeof(sg_io_hdr_t));
  if (hdr) {
    hdr->interface_id = 'S';
    hdr->flags        = SG_FLAG_LUN_INHIBIT;
    hdr->cmd_len      = cmd_len;
    hdr->cmdp         = cmdp;

    // SG_DXFER_NONE: for Test Unit Ready, ...
    // SG_DXFER_TO_DEV: for Write, ...
    // SG_DXFER_FROM_DEV: for Read, ...
    // SG_DXFER_TO_FROM_DEV
    hdr->dxfer_direction = dxfer_direction;

    hdr->dxfer_len       = dxfer_len;
    hdr->dxferp          = dxferp;
    hdr->sbp             = (unsigned char*) sbp;
    hdr->mx_sb_len       = sizeof(*sbp);
    // set_cmd(hdr, cmd_len, cmdp);
    // set_dxfer(hdr, dxfer_direction, dxferp, dxfer_len);
    // set_sb(hdr, sbp);
  }
  return hdr;
}

/**
 * Release sg_io_hdr_t object
 *
 * @param hdr [i] Pointer of sg_io_hdr_t object
 */
void destroy_sg_io_hdr(sg_io_hdr_t* hdr) {
  free(hdr);
  hdr = NULL;
}

/**
 * Show sense data
 *
 * @param sbp [i] Sense Buffer Pointer
 * @param operation_code [i] Operation code of SCSI Command Descriptor Block (CDB)
 */
static void show_sense_data(const ST_SPTI_REQUEST_SENSE_RESPONSE* const sbp, const uint8_t operation_code) {
  const char* const description[] = { "No Sense", "Recovered Error",
                                      "Not Ready", "Medium Error",
                                      "Hardware Error", "Illegal Request",
                                      "Unit Attention", "Data Protect",
                                      "Blank Check", "",
                                      "", "Aborted Command",
                                      "", "Volume Overflow", "", "" };
  char* log_level = OUTPUT_WARNING;
  if (sbp->sense_key == 0) {
    log_level = OUTPUT_INFO;
  }
  output_accdg_to_vl(log_level, DISPLAY_ALL_INFO,
                     "Sense Key %X (%s)\n",
                     sbp->sense_key, description[sbp->sense_key]);
  output_accdg_to_vl(log_level, DISPLAY_ALL_INFO,
                     "ASC ASCQ: %02X %02X (cdb: 0x%02X)\n",
                     sbp->asc, sbp->ascq, operation_code);
}

/**
 * Run SCSI command
 *
 * @param  scparam   [i]    Control parameter in spti_func (e.g. file descriptor)
 * @param  hdr       [i->o] SCSI Generic Input/Output Header
 * @return TRUE: success, FALSE: failed
 */
BOOL run_scsi_command(void* const scparam, sg_io_hdr_t* const hdr, uint32_t* const resid) {
  if (!scparam || !hdr || !resid) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "Invalid argument @ run_scsi_command\n");
    return FALSE;
  }

  const SCSI_DEVICE_PARAM* const psdp       = (SCSI_DEVICE_PARAM*)scparam;
  unsigned char sense_data[96]              = { 0 };
  ST_SPTI_REQUEST_SENSE_RESPONSE* const sbp = (ST_SPTI_REQUEST_SENSE_RESPONSE*)hdr->sbp;
  hdr->sbp                                  = sense_data;

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start SCSI_COMMAND:(%x)\n", hdr->cmdp[0]);
  const int ret = ioctl(psdp->fd_scsidevice, SG_IO, hdr);

  sbp->scsi_status             = hdr->status;
  sbp->filemark                = sense_data[2] & 0x80 ? 1 : 0;
  sbp->eom                     = sense_data[2] & 0x40 ? 1 : 0;
  sbp->ili                     = sense_data[2] & 0x20 ? 1 : 0;
  sbp->sense_key               = sense_data[2] & 0x0F;
  sbp->infomation              = sense_data[3] << 24 | sense_data[4] << 16 | sense_data[5] << 8 | sense_data[6];
  sbp->additional_sense_length = sense_data[7];
  sbp->cmd_spec_info           = sense_data[8] << 24 | sense_data[9] << 16 | sense_data[10] << 8 | sense_data[11];
  sbp->asc                     = sense_data[12];
  sbp->ascq                    = sense_data[13];
  sbp->field_rep_unit_code     = sense_data[14];
  sbp->sksv                    = sense_data[15] & 0x80 ? 1 : 0;
  sbp->c_d                     = sense_data[15] & 0x40 ? 1 : 0;
  sbp->bpv                     = sense_data[15] & 0x08 ? 1 : 0;
  sbp->bit_pointer             = sense_data[15] & 0x07;
  sbp->field_pointer           = sense_data[16] << 8 | sense_data[17];
  sbp->cln                     = sense_data[21] & 0x08 ? 1 : 0;

  hdr->sbp = (unsigned char*)sbp;
  *resid   = hdr->dxfer_len - hdr->resid;

  if (ret < 0) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "Sending SCSI Command failed.\n");
    return FALSE;
  }
  char* log_level = OUTPUT_WARNING;
  if (sbp->sense_key == 0) {
    log_level = OUTPUT_INFO;
  }
  switch (hdr->status) {
  case 0x00: // GOOD
    break;
  case 0x02:
    output_accdg_to_vl(log_level, DISPLAY_ALL_INFO,
                       "CHECK CONDITION: A problem occurred during command processing.\n");
    show_sense_data(sbp, hdr->cmdp[0]);
    break;
  case 0x08: 
    output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO,
                       "BUSY: The drive is unable to accept the command at this time.\n");
    break;
  case 0x18: 
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "RESERVATION CONFLICT: The drive is reserved.\n");
    break;
  case 0x28: 
    output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "QUEUE FULL\n");
    break;
  default:   
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "Undefined status: 0x%x\n", hdr->status);
    break;
  }

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end SCSI_COMMAND:(%x)\n", hdr->cmdp[0]);

  return hdr->status == 0;
}

/**
 * Convert from binary to uint64_t
 *
 * @param  buf  [i] Binary data
 * @param  size [i] Length of the binary data
 * @return Value in the binary data. When the size is more than 8, the value in the last 8 bytes are returned.
 */
uint64_t btoui(const unsigned char* const buf, const int size) {
  uint64_t val = 0;
  for (int i = 0; i < size; i++) {
    val <<= 8;
    val += buf[i];
  }
  return val;
}

