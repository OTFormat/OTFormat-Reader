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
 * @file spti_lib.h
 * @brief Structure definition to store a result by SCSI command
 */

#ifndef __SCSI_DEF_H
#define __SCSI_DEF_H

#include <stdint.h>

#define SYSTEM_ERROR_MESSAGE_LEN 255

/** Structure for System Error in SPTI functions */
typedef struct {
  uint32_t code;
  uint32_t msg_len;
  char message[SYSTEM_ERROR_MESSAGE_LEN];
} ST_SYSTEM_ERRORINFO;

/** Structure for Sense and data information by Check Condition command in SPTI functions */
typedef struct {                   // Field of Sense Data
  uint32_t infomation;             // byte 3-6
  uint32_t cmd_spec_info;          // byte 8-11.
  uint16_t field_pointer;          // byte 16-17: Point to the CDB byte or parameter byte in error.
  uint8_t scsi_status;
  uint8_t valid;                   // byte 0, bit 7: Indicates that the information byte 3-6 are valid or not.
  uint8_t error_code;
  uint8_t segment_number;
  uint8_t filemark;                // byte 2, bit 7: The current command has encountered a filemark or not.
  uint8_t eom;                     // byte 2, bit 6: Indicates that the device is at the end of medium or not.
  uint8_t ili;                     // byte 2, bit 5: Incorrect Length Indicator
  uint8_t sense_key;               // byte 2, bit 3-0
  uint8_t additional_sense_length; // byte 7
  uint8_t asc;                     // byte 12: Additional Sense Code
  uint8_t ascq;                    // byte 13: Additional Sense Code Qualifier
  uint8_t field_rep_unit_code;     // byte 14: Field Replaceable Unit Code (FRU)
  uint8_t bit_pointer;             // byte 15, bit 2-0: Points to bit in error of field specified by FIELD POINTER.
  uint8_t bpv;                     // byte 15, bit 3: Bit Pointer Valid
  uint8_t c_d;                     // byte 15, bit 6: Control/Data
  uint8_t sksv;                    // byte 15, bit 7: Sense Key Specific Valid
  uint8_t cln;                     // byte 21, bit 3: This device is requesting a clean or not.
  uint8_t vendor_unique1[8];
  uint8_t vendor_unique2[4];
  uint8_t vendor_unique3[4];
} ST_SPTI_REQUEST_SENSE_RESPONSE;

/** Device Type Attributes: 0x0000-0x0223 */
typedef struct {
  uint64_t remaining_capacity;
  uint64_t muximum_capacity;
  uint64_t tape_alert_flags;
  uint64_t load_count;
  uint64_t mam_space_remaining;
  uint8_t  assigning_orgnizization[8];
  uint8_t  formatted_density_code;
  uint16_t initialization_count;
  uint8_t  volume_id[32];
  uint8_t  serialno_last_load[40];
  uint8_t  serialno_load_1[40];
  uint8_t  serialno_load_2[40];
  uint8_t  serialno_load_3[40];
  uint64_t totalbytes_written_medium_life;
  uint64_t totalbytes_read_medium_life;
  uint64_t totalbytes_written_last_load;
  uint64_t totalbytes_read_last_load;
  uint32_t volume_change_reference;
} ST_SPTI_DEVICE_TYPE_ATTRIBUTE;

/** Host Type Attributes: 0x0800-0x080A 0x0820 */
typedef struct {
  uint8_t application_vendor[8];
  uint8_t application_name[32];
  uint8_t application_version[8];
  uint8_t user_medium_text_label[160];
  uint8_t date_time_last_written[12];
  uint8_t text_localization_identifier;
  uint8_t barcode[32];
  uint8_t owning_hoste_textual_name[80];
  uint8_t media_pool[160];
  uint8_t partition_user_text_label[16];
  uint8_t load_unload_at_partition;
  uint8_t volume_coherency_information[70];
  uint8_t medium_globally_unique_identifier[36 + 1];
  uint8_t media_pool_globally_unique_identifier[36 + 1];
  uint8_t system_globally_unique_identifier[36 + 1];
} ST_SPTI_HOST_TYPE_ATTRIBUTE;

/** Structure for Position data by Read Position command in SPTI functions */
typedef struct {
  uint64_t blockNumber; // number of logical objects between bop and the current logical position
  uint64_t fileNumber; // number of filemarks between bop and the current logical position
  uint8_t bop; // beginning of partition: 0: is not at bop, 1: bop
  uint8_t eop; // end of partition: 0: is not between early warning and eop, 1: between ew and eop
  uint8_t mpu; // mark position unknown: 0: fileNumber is valid, 1: is not known
  uint8_t bpu; // logical object number unknown: 0: exact, 1: estimate
  uint32_t partitionNumber; // partition number for the current logical position
} ST_SPTI_CMD_POSITIONDATA;

#endif  /* __SCSI_DEF_H */

