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
 * @file check_marker_l3_pt.c
 * 
 * Functions to check if a Partial Tape (PT) complies with OTFormat.
 */

#include "ltos_format_checker.h"

/**
 * Check ocm info format.
 * @param [in]  (buffer)              Buffer pointer of markers on this tape.
 * @param [in]  (current_position)    Current position of the buffer pointer.
 * @param [in]  (length)              OCM Info length.
 * @param [in]  (ocm_info_num)        OCM Info number. Just for trace log.
 * @return      (OK/NG)               If the format is correct or not.
 */
static int check_ocm_info(const char* const buffer, uint64_t* const current_position, const uint64_t length,
                          const int ocm_info_num) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO,
                               "start:check_ocm_info: OCM Info %d\n", ocm_info_num);
  uint64_t ocm_number_of_po           = 0;             //[Number of Packed objects] in OCM header
  uint64_t* po_block_offset           = NULL;         //[Block offset] in Packed Object Info Directory
  uint64_t* po_info_length            = NULL;         //[Packed Object Info Length] in Packed Object Info Directory
  uint64_t no_data                    = 0;
  const uint64_t pre_current_position = *current_position;

  if (clf_header(OCM_IDENTIFIER, buffer, NULL, ON, current_position, &ocm_number_of_po, &no_data) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "OCM header format is not correct.\n");
  }
  po_block_offset = clf_allocate_memory(sizeof(uint64_t) * ocm_number_of_po, "po_block_offset");
  po_info_length = clf_allocate_memory(sizeof(uint64_t) * ocm_number_of_po, "po_info_length");
  // Packed Object Info Directory
  if (clf_directory(OCM_IDENTIFIER, buffer, current_position, ocm_number_of_po, po_info_length, po_block_offset) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "OCM directory format is not correct.\n");
  }
  free(po_block_offset);
  po_block_offset = NULL;
  uint64_t ocm_info_sum_length = 0;
  for (uint64_t i = 0; i < ocm_number_of_po; i++) {
    ocm_info_sum_length +=  po_info_length[i];
  }
  if (length != *current_position - pre_current_position + ocm_info_sum_length) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO,
                              "OCM Info length is not correct.\n"
                              "%s(Data offset:%d) + (Sum of packed object info length:%lu) = (OCM Info length:%lu)\n"
                              ,INDENT, *current_position - pre_current_position, ocm_info_sum_length, length);
  }
  for (uint64_t i = 0; i < ocm_number_of_po; i++) {
    ret |= clf_packed_objects_info("", (unsigned char*) buffer, current_position, po_info_length[i]);
  }
  free(po_info_length);
  po_info_length = NULL;
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO,
                            "end  :check_ocm_info: OCM Info %d\n", ocm_info_num);
  return ret;
}

/**
 * Check Partial Reference format.
 * @param [in]  (filename) File name which has Partial Reference to check.
 * @return      (OK/NG)    If the format is correct, return OK. Otherwise, return NG.
 */
int check_partial_reference(const char* const filename) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO,
                               "start:check_partial_reference: %s\n", filename);
  set_top_verbose(DISPLAY_HEADER_AND_L43_INFO);

  FILE* fp = clf_open_file(filename, "rb");
  uint64_t pr_number_of_ocm = 0;

  { // Check PR Header
    uint8_t header[IDENTIFIER_SIZE + PR_HEADER_SIZE] = { '\0' };
    const size_t read_byte = clf_read_file(header, 1, IDENTIFIER_SIZE + PR_HEADER_SIZE, fp);
    clf_check_read_data(read_byte, sizeof(header), "Partial Reference Header", filename);

    uint64_t current_position = 0;
    uint64_t pr_data_length   = 0;
    if (clf_header(PR_IDENTIFIER, (char*)header, NULL, OFF, &current_position,
                   &pr_number_of_ocm, &pr_data_length) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                                "Partial reference header format is not correct.\n");
    }
  }

  uint64_t* ocm_info_length = (uint64_t*)clf_allocate_memory(pr_number_of_ocm * sizeof(uint64_t),
                                                                   "OCM Info Length");
  { // Check OCM Directory
    uint8_t* directory = (uint8_t*)clf_allocate_memory(pr_number_of_ocm * PR_DIR_SIZE, "OCM directory");
    const size_t read_byte   = clf_read_file(directory, 1, pr_number_of_ocm * PR_DIR_SIZE, fp);
    clf_check_read_data(read_byte, pr_number_of_ocm * PR_DIR_SIZE, "OCM Directory", filename);

    uint64_t current_position       = 0;
    uint64_t* block_offset    = (uint64_t*)clf_allocate_memory(pr_number_of_ocm * sizeof(uint64_t),
                                                                     "Block Offset");
    if (clf_directory(PR_IDENTIFIER, (char*)directory, &current_position,
                      pr_number_of_ocm, ocm_info_length, block_offset) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO,
                                "Object commit marker directory format is not correct.\n");
    }
    free(directory);
    directory    = NULL;
    free(block_offset);
    block_offset = NULL;
  }

  // Check OCM Info
  for (uint64_t ocm_info_num = 0; ocm_info_num < pr_number_of_ocm; ocm_info_num++) {
    uint8_t* ocm_info = (uint8_t*)clf_allocate_memory(ocm_info_length[ocm_info_num], "OCM Info");
    const size_t read_byte  = clf_read_file(ocm_info, 1, ocm_info_length[ocm_info_num], fp);
    clf_check_read_data(read_byte, ocm_info_length[ocm_info_num], "OCM Info", filename);
    uint64_t current_position = 0;

    ret |= check_ocm_info((char*)ocm_info, &current_position, ocm_info_length[ocm_info_num], ocm_info_num);
    free(ocm_info);
    ocm_info = NULL;
  }

  free(ocm_info_length);
  ocm_info_length = NULL;
  clf_close_file(fp);
  set_top_verbose(DEFAULT);
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_HEADER_AND_L43_INFO,
                            "end  :check_partial_reference: %s\n", filename);
  return ret;
}

#ifdef OBSOLETE
/**
 * Check partial tapes format.
 * @param [in]  (unpackedobjpath)  Save path of unpacked object.
 * @param [in]  (which_partition)  Data Partition:0, Reference Partition:1
 * @param [in]  (pr_block)         Block address of a pr.
 * @return      (OK/NG)            If the format is correct or not.
 */
int clf_partial_tapes(const char *unpackedobjpath, int which_partition, int pr_block) {
  int ret                                        = OK;
  char partial_reference_buffer[LTOS_BLOCK_SIZE] = { 0 };
  int current_position                           = 0;
  unsigned int dat_size                          = 0;
  uint64_t pt_number_of_ocm                      = 0;
  uint64_t pt_data_length                        = 0;
  uint64_t ocmd_length[MAX_NUM_OF_PR]            = { 0 };
  uint64_t ocmd_block_offset[MAX_NUM_OF_PR]      = { 0 };
  ST_SPTI_CMD_POSITIONDATA pos                   = { 0 };
  set_top_verbose(DISPLAY_HEADER_AND_L43_INFO);

  if (unpackedobjpath == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at clf_partial_tapes");
  }
#ifndef NO_TAPE
  if (locate_to_tape(pr_block) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "Failed to locate to Partial Reference.\n");
    set_top_verbose(DEFAULT);
    return ret; // When locate failed, the partial reference does not exist in the tape.
  }
  if (read_data(LTOS_BLOCK_SIZE, partial_reference_buffer, &dat_size) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "Failed to read Partial Reference.\n");
    set_top_verbose(DEFAULT);
    return ret; // When read failed, no additional check is required.
  }
#else
  output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L43_INFO,
                     "Can't check partial reference format without tape.\n");
  set_top_verbose(DEFAULT);
  return ret;
#endif
  // A partial reference.(LEVEL 3)
  output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L43_INFO, "Level 3\n");
  if (clf_header(PR_IDENTIFIER, partial_reference_buffer, NULL, OFF, &current_position,
                 &pt_number_of_ocm, &pt_data_length) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "Partial reference header format is not correct.\n");
  }
  if (clf_directory(PR_IDENTIFIER, partial_reference_buffer, &current_position,
                    pt_number_of_ocm, ocmd_length, ocmd_block_offset) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "Object commit marker directory format is not correct.\n");
  }
  {
    uint64_t ocm_info_sum_length = 0;
    for (int ocm_info_num = 0; ocm_info_num < pt_number_of_ocm; ocm_info_num++) {
      ret |= check_ocm_info(partial_reference_buffer, &current_position, ocmd_length[ocm_info_num],
                            ocm_info_num, &ocm_info_sum_length);
    }
  }

  // Object series.
  if (which_partition == DATA_PARTITION) {
    if (move_on_tape(SPACE_FILE_MARK_MODE, (int)((pt_number_of_ocm * OBJECT_SERIES_FILE_MARK_NUM) - 1)) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "Failed to space at Object Commit Marker.\n");
    }
    if (read_position_on_tape(&pos) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L43_INFO, "Failed to read position.\n");
    }

    for (int ocm_num = 0; ocm_num < pt_number_of_ocm; ocm_num++) {
      ret |= clf_object_series(unpackedobjpath, (int)(pos.blockNumber) + ONE_BEFORE,
                               ocmd_length[ocm_num], ocmd_block_offset[ocm_num]);
    }
  }
  set_top_verbose(DEFAULT);
  return ret;
}
#endif // OBSOLETE
