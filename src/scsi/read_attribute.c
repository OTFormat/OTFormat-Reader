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
 * @file read_attribute.c
 * @brief Functions to issue the SCSI command READ ATTRIBUTE
 */

#include <inttypes.h>
#include "spti_lib.h"

#define MAM_HEADER_SIZE (0x4)
#define MAM_PAGE_HEADER_SIZE (0x5)
#define MAM_PAGE_COHERENCY (0x080C)
#define MAM_PAGE_COHERENCY_SIZE_FOR_TEST (0x44)

/* just for debug
static void show_attribute(unsigned char* buf) {
  uint64_t offset = 4;
  const uint32_t data = btoui(buf, offset);
  printf("Number of bytes of attribute information: 0x%04X\n", data);

  while (offset < data) {
    const uint16_t id = btoui(buf + offset, 2);
    offset += 2;

    // format: 0: binary, 1: ascii, 10: text, 11: reserved
    const uint8_t format = *(buf + offset++) & 0x3;

    const uint16_t length = btoui(buf + offset, 2);
    offset += 2;
    if (format == 0) {
      const uint64_t val = btoui(buf + offset, length);
      printf("id: 0x%04X, format: 0x%02X, length: 0x%04X, val: 0x%016" PRIx64 "\n",
             id, format, length, val);
    } else if (format == 1 || format == 10) {
      unsigned char val[length + 1];
      memset(val, 0, sizeof(val));
      memcpy(val, buf + offset, length);
      printf("id: 0x%04X, format: 0x%02X, length: 0x%04X, val: _%s\n",
             id, format, length, val);
    }
    offset += length;
  }
}
*/

/**
 * SCSI command: READ ATTRIBUTE - 8Ch
 *
 * @param  scparam   [i] Control parameter in spti_func (e.g. file descriptor)
 * @param  partition [i] Partition number
 * @param  action    [i] Service action (SCSI CDB parameter)
 * @param  id        [i] ID (SCSI CDB parameter)
 * @param  dxfer_len [i] Data Transfer Length
 * @param  dxferp    [o] Data Transfer Pointer
 * @param  resid     [o] Residual Count
 * @param  sbp       [o] Sense Buffer Pointer
 * @param  syserr    [o] System Error:          When a SCSI command failed, System error information will be stored in this structure in the future.
 * @return TRUE: success, FALSE: failed
 */
BOOL spti_read_attribute(void* scparam, uint8_t partition, uint8_t action,
                         uint16_t id, uint32_t dxfer_len, void* dxferp,
                         uint32_t* resid,
                         ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                         ST_SYSTEM_ERRORINFO* syserr) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO,
                     "start:spti_read_attribute: action=0x%02X, partition=%d, id=0x%04X\n",
                     action, partition, id);
  static const unsigned char cmd_len = 16;
  unsigned char cmd[cmd_len];
  memset(cmd, 0, cmd_len);
  cmd[0]  = 0x8c;
  cmd[1]  = action & 0x1f;
  cmd[7]  = partition;
  cmd[8]  = id >> 8;
  cmd[9]  = id;
  cmd[10] = dxfer_len >> 24;
  cmd[11] = dxfer_len >> 16;
  cmd[12] = dxfer_len >> 8;
  cmd[13] = dxfer_len;

  sg_io_hdr_t* const hdr = init_sg_io_hdr(cmd_len, cmd, SG_DXFER_FROM_DEV,
                                          dxfer_len, dxferp, sbp);

  const BOOL rc = run_scsi_command(scparam, hdr, resid);
  destroy_sg_io_hdr(hdr);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :spti_read_attribute\n");
  return rc;
}

static void set_drive_attribute(unsigned char* buf,
                                ST_SPTI_DEVICE_TYPE_ATTRIBUTE* attr_data) {
  uint64_t offset = 4;
  const uint32_t data = btoui(buf, offset);

  while (offset < data) {
    const uint16_t id = btoui(buf + offset, 2);
    offset += 2;

    // const uint8_t format = btoui(buf + offset++, 1);
    offset++;

    const uint16_t length = btoui(buf + offset, 2);
    offset += 2;

    switch (id) {
    case 0x0000:
      attr_data->remaining_capacity = btoui(buf + offset, length);
      break;
    case 0x0001:
      attr_data->muximum_capacity = btoui(buf + offset, length);
      break;
    case 0x0002:
      attr_data->tape_alert_flags = btoui(buf + offset, length);
      break;
    case 0x0003:
      attr_data->load_count = btoui(buf + offset, length);
      break;
    case 0x0004:
      attr_data->mam_space_remaining = btoui(buf + offset, length);
      break;
    case 0x0005:
      memmove(attr_data->assigning_orgnizization, buf + offset, length);
      break;
    case 0x0006:
      attr_data->formatted_density_code = btoui(buf + offset, length);
      break;
    case 0x0007:
      attr_data->initialization_count = btoui(buf + offset, length);
      break;
    case 0x0008:
      memmove(attr_data->volume_id, buf + offset, length);
      break;
    case 0x0009:
      attr_data->volume_change_reference = btoui(buf + offset, length);
      break;
    case 0x020A:
      memmove(attr_data->serialno_last_load, buf + offset, length);
      break;
    case 0x020B:
      memmove(attr_data->serialno_load_1, buf + offset, length);
      break;
    case 0x020C:
      memmove(attr_data->serialno_load_2, buf + offset, length);
       break;
    case 0x020D:
      memmove(attr_data->serialno_load_3, buf + offset, length);
      break;
    case 0x0220:
      attr_data->totalbytes_written_medium_life = btoui(buf + offset, length);
      break;
    case 0x0221:
      attr_data->totalbytes_read_medium_life = btoui(buf + offset, length);
      break;
    case 0x0222:
      attr_data->totalbytes_written_last_load = btoui(buf + offset, length);
      break;
    case 0x0223:
      attr_data->totalbytes_read_last_load = btoui(buf + offset, length);
      break;
    }
    offset += length;
  }
}

BOOL spti_read_drive_attribute(void* scparam, uint8_t action, uint16_t id,
                               ST_SPTI_DEVICE_TYPE_ATTRIBUTE* attr_data,
                               ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                               ST_SYSTEM_ERRORINFO* syserr) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:spti_read_drive_attribute\n");
  const uint32_t dxfer_len = 0x1000;
  unsigned char dxferp[dxfer_len];
  memset(dxferp, 0, dxfer_len);
  uint32_t resid = 0;
  const BOOL rc = spti_read_attribute(scparam, 0, action, id, dxfer_len,
                                      dxferp, &resid, sbp, syserr);
  set_drive_attribute(dxferp, attr_data);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :spti_read_drive_attribute\n");
  return rc;
}

static void set_drive_host_type_attribute(unsigned char* buf,
                                          ST_SPTI_HOST_TYPE_ATTRIBUTE* attr_data) {
  uint64_t offset = 4;
  const uint32_t data = btoui(buf, offset);

  while (offset < data) {
    const uint16_t id = btoui(buf + offset, 2);
    offset += 2;

    // const uint8_t format = btoui(buf + offset++, 1);
    offset++;

    const uint16_t length = btoui(buf + offset, 2);
    offset += 2;

    switch (id) {
    case 0x0800:
      memmove(attr_data->application_vendor, buf + offset, length);
      break;
    case 0x0801:
      memmove(attr_data->application_name, buf + offset, length);
      break;
    case 0x0802:
      memmove(attr_data->application_version, buf + offset, length);
      break;
    case 0x0803:
      memmove(attr_data->user_medium_text_label, buf + offset, length);
      break;
    case 0x0804:
      memmove(attr_data->date_time_last_written, buf + offset, length);
      break;
    case 0x0805:
      attr_data->text_localization_identifier = btoui(buf + offset, length);
      break;
    case 0x0806:
      memmove(attr_data->barcode, buf + offset, length);
      break;
    case 0x0807:
      memmove(attr_data->owning_hoste_textual_name, buf + offset, length);
      break;
    case 0x0808:
      memmove(attr_data->media_pool, buf + offset, length);
      break;
    case 0x0809:
      memmove(attr_data->partition_user_text_label, buf + offset, length);
      break;
    case 0x080A:
      attr_data->load_unload_at_partition = btoui(buf + offset, length);
      break;
    case 0x080C:
      memmove(attr_data->volume_coherency_information, buf + offset, length);
      break;
    case 0x0820:
      memmove(attr_data->medium_globally_unique_identifier, buf + offset, length);
      break;
    case 0x0821:
      memmove(attr_data->media_pool_globally_unique_identifier, buf + offset, length);
      break;
    case 0x1607:
      memmove(attr_data->system_globally_unique_identifier, buf + offset, length);
      break;
    }
    offset += length;
  }
}

BOOL spti_read_drive_host_type_attribute(void* scparam, uint8_t action,
                                         uint16_t id,
                                         ST_SPTI_HOST_TYPE_ATTRIBUTE* attr_data,
                                         ST_SPTI_REQUEST_SENSE_RESPONSE* sbp,
                                         ST_SYSTEM_ERRORINFO* syserr) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:spti_read_drive_host_type_attribute\n");
  const uint32_t dxfer_len = 0x1000;
  unsigned char dxferp[dxfer_len];
  memset(dxferp, 0, dxfer_len);
  uint32_t resid = 0;
  const BOOL rc =  spti_read_attribute(scparam, 0, action, id, dxfer_len,
                                       dxferp, &resid, sbp,
                                       syserr);
  set_drive_host_type_attribute(dxferp, attr_data);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :spti_read_drive_host_type_attribute\n");
  return rc;
}


/* just for UT */
static void show_mam_page(const unsigned char* const dxferp) {
  uint64_t offset = 4;
  const uint32_t data = btoui(dxferp, offset);

  const uint16_t id = btoui(dxferp + offset, 2);
  offset += 2;

  const uint8_t format = btoui(dxferp + offset++, 1) & 0x3;

  const uint16_t length = btoui(dxferp + offset, 2);
  offset += 2;

  printf("mam_page: data: 0x%08X, id: 0x%04X, format: 0x%02X, length: 0x%04X\n",
         data, id, format, length);

  for (int i = 0; i < length; i++) {
    printf("i: 0x%02X, buf: 0x%02X\n", i, dxferp[offset]);
    offset++;
  }
}

BOOL test_read_attribute(const char* const path) {
  SCSI_DEVICE_PARAM* const sdp = init_scsi_device_param(path);
  uint32_t resid = 0;
  unsigned char dxferp[MAM_PAGE_COHERENCY_SIZE_FOR_TEST + MAM_PAGE_HEADER_SIZE + MAM_HEADER_SIZE] = {0};
  ST_SPTI_REQUEST_SENSE_RESPONSE sbp = {0};
  ST_SYSTEM_ERRORINFO syserr = {0};
  const BOOL rc = spti_read_attribute(sdp, REFERENCE_PARTITION, 0x00, MAM_PAGE_COHERENCY,
                                      sizeof(dxferp), dxferp, &resid, &sbp,
                                      &syserr);
  if (rc) {
    printf("dxferp[0]: %d\n", dxferp[0]);
  }
  show_mam_page(dxferp);
  destroy_scsi_device_param(sdp);
  return rc;
}

static void show_drive_attribute(const ST_SPTI_DEVICE_TYPE_ATTRIBUTE* const attr) {
  printf("remaining_capacity: 0x%016" PRIx64 "\n", attr->remaining_capacity);
  printf("muximum_capacity: 0x%016" PRIx64 "\n", attr->muximum_capacity);
  printf("tape_alert_flags: 0x%016" PRIx64 "\n", attr->tape_alert_flags);
  printf("load_count: 0x%016" PRIx64 "\n", attr->load_count);
  printf("mam_space_remaining: 0x%016" PRIx64 "\n", attr->mam_space_remaining);
  printf("assigning_orgnizization: %s\n", attr->assigning_orgnizization);
  printf("formatted_density_code: 0x%02X\n", attr->formatted_density_code);
  printf("initialization_count: 0x%08X\n", attr->initialization_count);
  printf("volume_id: %s\n", attr->volume_id);
  printf("volume_change_reference: 0x%08X\n", attr->volume_change_reference);
  printf("serialno_last_load: %s\n", attr->serialno_last_load);
  printf("serialno_load_1: %s\n", attr->serialno_load_1);
  printf("serialno_load_2: %s\n", attr->serialno_load_2);
  printf("serialno_load_3: %s\n", attr->serialno_load_3);
  printf("totalbytes_written_medium_life: 0x%016" PRIx64 "\n", attr->totalbytes_written_medium_life);
  printf("totalbytes_read_medium_life: 0x%016" PRIx64 "\n", attr->totalbytes_read_medium_life);
  printf("totalbytes_written_last_load: 0x%016" PRIx64 "\n", attr->totalbytes_written_last_load);
  printf("totalbytes_read_last_load: 0x%016" PRIx64 "\n", attr->totalbytes_read_last_load);
}

BOOL test_read_drive_attribute(const char* const path) {
  SCSI_DEVICE_PARAM* const sdp = init_scsi_device_param(path);
  ST_SPTI_DEVICE_TYPE_ATTRIBUTE attr_data = {0};
  ST_SPTI_REQUEST_SENSE_RESPONSE sbp = {0};
  ST_SYSTEM_ERRORINFO syserr = {0};
  const BOOL rc = spti_read_drive_attribute(sdp, 0, 0, &attr_data, &sbp, &syserr);
  if (rc) {
    show_drive_attribute(&attr_data);
    printf("read_drive_attribute: OK\n");
  }
  destroy_scsi_device_param(sdp);
  return rc;
}

static void show_drive_host_type_attribute(const ST_SPTI_HOST_TYPE_ATTRIBUTE* const attr) {
  printf("application_vendor: %s\n", attr->application_vendor);
  printf("application_name: %s\n", attr->application_name);
  printf("application_version: %s\n", attr->application_version);
  printf("user_medium_text_label: %s\n", attr->user_medium_text_label);
  printf("date_time_last_written: %s\n", attr->date_time_last_written);
  printf("text_localization_identifier: 0x%02X\n", attr->text_localization_identifier);
  printf("barcode: %s\n", attr->barcode);
  printf("owning_hoste_textual_name: %s\n", attr->owning_hoste_textual_name);
  printf("media_pool: %s\n", attr->media_pool);
  printf("partition_user_text_label: %s\n", attr->partition_user_text_label);
  printf("load_unload_at_partition: 0x%02X\n", attr->load_unload_at_partition);
  printf("volume_coherency_information: %s\n", attr->volume_coherency_information);
  printf("medium_globally_unique_identifier: %s\n", attr->medium_globally_unique_identifier);
  printf("media_pool_globally_unique_identifier: %s\n", attr->media_pool_globally_unique_identifier);
  printf("system_globally_unique_identifier: %s\n", attr->system_globally_unique_identifier);
}

BOOL test_read_drive_host_type_attribute(const char* const path) {
  SCSI_DEVICE_PARAM* const sdp = init_scsi_device_param(path);
  ST_SPTI_HOST_TYPE_ATTRIBUTE attr_data = {{0},{0},{0},{0},{0},0,{0},{0},{0},{0},0,{0},{0},{0},{0}};
  ST_SPTI_REQUEST_SENSE_RESPONSE sbp = {0};
  ST_SYSTEM_ERRORINFO syserr = {0};
  const BOOL rc = spti_read_drive_host_type_attribute(sdp, 0, 0x800, &attr_data,
                                                      &sbp, &syserr);
  if (rc) {
    show_drive_host_type_attribute(&attr_data);
  }
  destroy_scsi_device_param(sdp);
  return rc;
}

