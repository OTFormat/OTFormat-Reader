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
 * @file check_medium_auxiliary_memory.c
 * 
 * Functions to check if a data stored in Medium auxiliary memory (MAM) complies with OTFormat.
 */

#include "ltos_format_checker.h"

static int clf_check_mam_hta(SCSI_DEVICE_PARAM* const scparam, const MamVci* const mamvci, MamHta* const mamhta);
static int clf_check_mam_part(SCSI_DEVICE_PARAM* const scparam, const uint32_t part, MamVci* const coh);
static int clf_check_mam_vcr(SCSI_DEVICE_PARAM* const scparam, const MamVci* const mamvci);

/**
 * Check MAM coherency (entry point function).
 * @param [in]  (scparam) SCSI device parameter for ScsiLib.
 * @param [out] (mamvci)  Pointer of a volume coherency information.
 * @param [out] (mamhta)  Pointer of a host-type attributes.
 * @return      (OK/NG)   OK: the format is correct.
 *                        NG: format error or any other error occurs.
 */
int clf_check_mam_coherency(SCSI_DEVICE_PARAM* const scparam, MamVci* const mamvci, MamHta* const mamhta) {
  set_top_verbose(DISPLAY_ALL_INFO);
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:clf_check_mam_coherency\n");;

  if (!scparam || !mamvci || !mamhta) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at clf_check_mam_coherency: scparam = 0x%X, mamvci = 0x%X, mamhta = 0x%X\n",
                              scparam, mamvci, mamhta);
  }

  if (clf_check_mam_part(scparam, REFERENCE_PARTITION, mamvci + REFERENCE_PARTITION)) {
    ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "MAM VCI for reference partition is invalid.\n");
  }

  if (clf_check_mam_part(scparam, DATA_PARTITION, mamvci + DATA_PARTITION)) { /* attribute was invalid */
    /* MAM parameter for data partition invalid */
    ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "MAM VCI for data partition is invalid.\n");
  }

  if (mamvci[REFERENCE_PARTITION].is_valid && mamvci[DATA_PARTITION].is_valid) {
    if (mamvci[REFERENCE_PARTITION].Data.pr_count != mamvci[DATA_PARTITION].Data.pr_count) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "MAM VCI Partial Reference number mismatch RP[%d] DP[%d]\n",
                                mamvci[REFERENCE_PARTITION].Data.pr_count, mamvci[DATA_PARTITION].Data.pr_count);
    }
    if (strcasecmp(mamvci[REFERENCE_PARTITION].Data.uuid, mamvci[DATA_PARTITION].Data.uuid)) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "MAM VCI Volume UUID mismatch RP[%s] DP[%s]\n",
                                mamvci[REFERENCE_PARTITION].Data.uuid, mamvci[DATA_PARTITION].Data.uuid);
    }
  }

  ret |= clf_check_mam_vcr(scparam, mamvci);
  ret |= clf_check_mam_hta(scparam, mamvci, mamhta);

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :clf_check_mam_coherency\n");;
  set_top_verbose(DEFAULT);
  return ret;
}

/**
 * Check Volume Change Reference
 * @param [in]  (scparam)   SCSI device parameter for ScsiLib.
 * @param [in]  (mamvci)    Pointer of a volume coherency information. 
 * @return      (OK/NG)     OK: the format is correct.
 *                          NG: format error or any other error occurs.
 */
static int clf_check_mam_vcr(SCSI_DEVICE_PARAM* const scparam, const MamVci* const mamvci) {
  int ret = OK;

  if (!scparam || !mamvci) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at clf_check_mam_vcr: scparam = 0x%X, mamvci = 0x%X\n",
                              scparam, mamvci);
  }

  ST_SPTI_REQUEST_SENSE_RESPONSE sense_data      = { 0 };
  ST_SYSTEM_ERRORINFO syserr                     = { 0 };
  ST_SPTI_DEVICE_TYPE_ATTRIBUTE device_attr_data = { 0 };

  if (spti_read_drive_attribute(scparam, 0, 0, &device_attr_data, &sense_data, &syserr) != FALSE) {
    const uint64_t volume_change_ref = device_attr_data.volume_change_reference;
    if (((mamvci + REFERENCE_PARTITION)->is_valid && (mamvci + REFERENCE_PARTITION)->Data.volume_change_ref != volume_change_ref)
     || ((mamvci + DATA_PARTITION)->is_valid && (mamvci + DATA_PARTITION)->Data.volume_change_ref != volume_change_ref)) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "MAM VCR mismatch.\n"
                                "%sVCI[RP] [%lu]\n"
                                "%sVCI[DP] [%lu]\n"
                                "%sVCR     [%lu]\n",
                                INDENT, (mamvci + REFERENCE_PARTITION)->Data.volume_change_ref,
                                INDENT, (mamvci + DATA_PARTITION)->Data.volume_change_ref,
                                INDENT, volume_change_ref);
    }
  } else {
    // MAM access error should be acceptable.
    ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "Failed to access Medium Auxiliary Memory Volume Change Reference in Cartridge Memory.\n");
  }

  return ret;
}

/**
 * Check Application Name of Host-type attributes.
 * @param [out] (mamhta) Pointer of a host-type attributes.
 * @return      (OK/NG) OK: the application name is valid.
 *                      NG: the application name is invalid.
 */
static int check_application_name(const MamHta* const mamhta) {
  int ret = OK;

  if (!mamhta) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at check_application_name: mamhta = 0x%X\n", mamhta);
  }

  if (mamhta->is_valid) {
    static const char* const expected_prefix = IMPLEMENTATION_IDENTIFIER " ";
    if (strncmp(mamhta->Data.application_name, expected_prefix, strlen(expected_prefix))) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO,
                                "Application name \"%s\" in Medium Auxiliary Memory Host-type Attributes should start with \"%s\"\n",
                                mamhta->Data.application_name, expected_prefix);
    }

    for (int i = 0; i < MAM_HTA_NAME_SIZE; i++) {
      if (!isprint(mamhta->Data.application_name[i])) {
        ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO,
                                  "Application name in Medium Auxiliary Memory Host-type Attributes has invalid value 0x%02d at %d.\n",
                                  mamhta->Data.application_name[i], i);
      }
    }
  }
  return ret;
}

/**
 * Check Host-type attribute.
 * @param [in]  (scparam)   SCSI device parameter for ScsiLib.
 * @param [in]  (mamvci)    Pointer of a volume coherency information.
 * @param [out] (mamhta)    Pointer of a host-type attributes.
 * @return      (OK/NG)     OK: the format is correct.
 *                          NG: format error or any other error occurs.
 */
static int clf_check_mam_hta(SCSI_DEVICE_PARAM* const scparam, const MamVci* const mamvci, MamHta* const mamhta) {
  int ret = OK;

  if (!scparam || !mamvci || !mamhta) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at clf_check_mam_hta: scparam = 0x%X, mamvci = 0x%X, mamhta = 0x%X\n",
                              scparam, mamvci, mamhta);
  }

  ST_SPTI_HOST_TYPE_ATTRIBUTE host_attr          = {{0},{0},{0},{0},{0},0,{0},{0},{0},{0},0,{0},{0},{0},{0}};
  ST_SPTI_REQUEST_SENSE_RESPONSE sense_data      = { 0 };
  ST_SYSTEM_ERRORINFO syserr                     = { 0 };

  if (spti_read_drive_host_type_attribute(scparam, 0x00, 0x800, &host_attr, &sense_data, &syserr)) {
    memmove(mamhta->Data.application_vendor,  host_attr.application_vendor,  MAM_HTA_VENDOR_SIZE);
    memmove(mamhta->Data.application_name,    host_attr.application_name,    MAM_HTA_NAME_SIZE);
    memmove(mamhta->Data.application_version, host_attr.application_version, MAM_HTA_VERSION_SIZE);
    memmove(mamhta->Data.barcode,             host_attr.barcode,             MAM_HTA_BARCODE_SIZE);
#ifdef FORMAT_031
    memcpy(mamhta->Data.volume_id,           host_attr.medium_globally_unique_identifier,     UUID_SIZE);
    memcpy(mamhta->Data.pool_id,             host_attr.media_pool_globally_unique_identifier, UUID_SIZE);
#else
    uuid_unparse(host_attr.medium_globally_unique_identifier,                      mamhta->Data.system_id);
    uuid_unparse(host_attr.medium_globally_unique_identifier + sizeof(uuid_t),     mamhta->Data.volume_id);
    uuid_unparse(host_attr.media_pool_globally_unique_identifier,                  mamhta->Data.pool_id);
    uuid_unparse(host_attr.media_pool_globally_unique_identifier + sizeof(uuid_t), mamhta->Data.pool_group_id);
#endif
    mamhta->is_valid = TRUE;

    ret |= check_application_name(mamhta);
#ifndef FORMAT_031
    ret |= check_uuid_format(mamhta->Data.system_id, "System", LOCATION_MAM_HTA);
#endif
    ret |= check_uuid_format(mamhta->Data.volume_id, "Volume", LOCATION_MAM_HTA);
    for (int part = 0; part < NUMBER_OF_PARTITIONS; part++) {
      if ((mamvci + part)->is_valid
       && strcasecmp((mamvci + part)->Data.uuid, mamhta->Data.volume_id)) {
        ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "Volume UUID is inconsistent in Medium Auxiliary Memory\n"
                                  "%sVolume Coherency Information of %s Partition: %s%s\n"
                                  "%sHost-type Attributes:                                %s\n",
                                  INDENT, clf_get_partition_name(part),
                                  (part == REFERENCE_PARTITION ? "" : (part == DATA_PARTITION ? "     " : "  ")), // padding
                                  (mamvci + part)->Data.uuid, INDENT, mamhta->Data.volume_id);
      }
    }
    ret |= check_uuid_format(mamhta->Data.pool_id,       "Pool",       LOCATION_MAM_HTA);
    //memset(mamhta->Data.pool_group_id, 0, sizeof(mamhta->Data.pool_group_id)); //for Debug
    //strcpy(mamhta->Data.pool_group_id , ZERO_FILLED_UUID); //for Debug
    ret |= check_optional_uuid_format(mamhta->Data.pool_group_id, "Pool Group", LOCATION_MAM_HTA);
  } else {
    // MAM access error should be acceptable.
    ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "Failed to access Medium Auxiliary Memory Host-type Attribute in Cartridge Memory.\n");
  }

  return ret;
}

/**
 * Check Application Client Specific Information Length of Volume Coherency Information.
 *
 * @param [in]  (acsil) Pointer of Application Client Specific Information Length of Volume Coherency Information.
 * @return      (OK/NG) OK: the length is correct.
 *                      NG: the length is incorrect.
 */
static int clf_check_vci_acsi_length(const uint8_t* const acsil) {
  int ret = OK;

  if (!acsil) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at clf_check_vci_acsi_length: acsil = 0x%X\n", acsil);
  }

  const uint16_t expected_len = strlen(IMPLEMENTATION_IDENTIFIER) + MAM_VCI_ACSI_VERSION_SIZE + sizeof(uuid_t);
  uint16_t ap_clent_specific_len     = 0;

  r16(BIG, acsil, &ap_clent_specific_len, 1); // APPLICATION CLIENT SPECIFIC INFORMATION LENGTH

  if (ap_clent_specific_len != expected_len) {
    ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO,
                              "MAM VCI application client specific information length %d should be %d\n",
                              ap_clent_specific_len, expected_len);
  }

  return ret;
}

/**
 * Check Application Client Specific Information of Volume Coherency Information.
 *
 * @param [in]  (acsi)  Pointer of Application Client Specific Information of Volume Coherency Information.
 * @param [out] (coh)   VCI information.
 * @return      (OK/NG) OK: the length is correct.
 *                      NG: the length is incorrect.
 */
static int clf_check_vci_acsi(const uint8_t* const acsi, MamVci* const coh) {
  int ret = OK;

  if (!acsi || !coh) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at clf_check_vci_acsi: acsi = 0x%X, coh = 0x%X\n", acsi, coh);
  }

  if (strncmp((char*)acsi, IMPLEMENTATION_IDENTIFIER, strlen(IMPLEMENTATION_IDENTIFIER))) {
    char acsi_id[sizeof(IMPLEMENTATION_IDENTIFIER)] = { '\0' };
    memmove(acsi_id, acsi, strlen(IMPLEMENTATION_IDENTIFIER));
    ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO,
                              "MAM VCI application client specific information ID %s should be %s\n",
                              acsi_id, IMPLEMENTATION_IDENTIFIER);
  }

  int offset = strlen(IMPLEMENTATION_IDENTIFIER);
  static const int expected_acsi_version = 1;
  coh->Data.version = *(acsi + offset);
  if (coh->Data.version != expected_acsi_version) {
    ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO,
                              "MAM VCI application client specific information version %d should be %d\n",
                              coh->Data.version, expected_acsi_version);
  }

  offset += MAM_VCI_ACSI_VERSION_SIZE;
  uuid_unparse(acsi + offset, coh->Data.uuid);
  ret |= check_uuid_format(coh->Data.uuid, "Volume", LOCATION_MAM_VCI);

  return ret;
}

/**
 * Get VCI data for the specific partition.
 * @param [in]  (scparam) SCSI device parameter for ScsiLib.
 * @param [in]  (part)    Partition number.
 * @param [out] (coh)     VCI information.
 * @return      (OK/NG)   OK: the format is correct.
 *                        NG: format error or any other error occurs.
 */
static int clf_check_mam_part(SCSI_DEVICE_PARAM* const scparam, const uint32_t part, MamVci* const coh) {
  int ret = OK;

  if (!scparam || 1 < part || !coh) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Invalid arguments at clf_check_mam_part: scparam = 0x%X, part = %u, coh = 0x%X\n",
                              scparam, part, coh);
  }

  coh->is_valid     = FALSE;
  uint32_t dat_size = 0;

  unsigned char coh_data[MAM_PAGE_COHERENCY_SIZE + MAM_PAGE_HEADER_SIZE + MAM_HEADER_SIZE];

  ST_SPTI_REQUEST_SENSE_RESPONSE sense_data = { 0 };
  ST_SYSTEM_ERRORINFO syserr                = { 0 };

  if (spti_read_attribute(scparam, part, 0x00, MAM_PAGE_COHERENCY,
                          sizeof(coh_data), coh_data, &dat_size,
                          &sense_data, &syserr) != FALSE) {
    coh->is_valid = TRUE;

    uint16_t id  = 0;
    uint16_t len = 0;

    r16(BIG, coh_data + MAM_HEADER_SIZE, &id, 1);      // ATTRIBUTE IDENTIFIER
    r16(BIG, coh_data + MAM_HEADER_SIZE + 3, &len, 1); // ATTRIBUTE LENGTH

    const uint8_t vcr_size = coh_data[MAM_HEADER_SIZE + 5]; // VOLUME CHANGE REFERENCE VALUE LENGTH

    if (id != MAM_PAGE_COHERENCY) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO,
                                "MAM VCI ID 0x%04Xh should be 0x%04Xh in partition %d\n",
                                id, MAM_PAGE_COHERENCY, part);
    }

    if (len != MAM_PAGE_COHERENCY_SIZE) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "MAM VCI length %d should be %d in partition %d\n",
                                len, MAM_PAGE_COHERENCY_SIZE, part);
    }

    coh->Data.volume_change_ref = 0;
    coh->Data.rcm_block         = 0;

    switch (vcr_size) {
    case 8:
      r64(BIG, coh_data + MAM_HEADER_SIZE + 6, &coh->Data.volume_change_ref, 1); // VOLUME CHANGE REFERENCE VALUE
      break;
    default:
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "MAM VCI VCR size %d should be %d\n", vcr_size, 8);
    }

    r64(BIG, coh_data + MAM_HEADER_SIZE + 14, &coh->Data.pr_count,  1); // VOLUME CHOHERENCY COUNT
    r64(BIG, coh_data + MAM_HEADER_SIZE + 22, &coh->Data.rcm_block, 1); // VOLUME COHERENCY SET IDENTIFIER

    ret |= clf_check_vci_acsi_length(coh_data + MAM_HEADER_SIZE + 30);
    ret |= clf_check_vci_acsi(coh_data + MAM_HEADER_SIZE + 32, coh);
  } else {
    // MAM access error should be acceptable.
    ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "Failed to access Medium Auxiliary Memory Volume Coherency Information in Cartridge Memory.\n");
  }

  return ret;
}
