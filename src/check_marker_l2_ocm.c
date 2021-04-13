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
 * @file check_marker_l2_ocm.c
 * 
 * Functions to check if an Object Commit Marker (OCM) complies with OTFormat.
 */

#include "ltos_format_checker.h"



/* Move to an appropriate header if needed            */
/* Naming rule: ERR_MSG_ + OBJECT_NAME + _ERR_CONTENT */
#define ERR_MSG_OFFSET_ORDER                 "Any Offset should increase in the order."

#ifdef OBSOLETE
/**
 * Check object series format.
 * @param [in]  (unpackedobjpath)   Save path of unpacked object.
 * @param [in]  (os_top_block_num)  Block address of a first ocm.
 * @param [in]  (ocmd_length)       Byte size of each markers.
 * @param [in]  (ocmd_block_offset) Relative block number of ocm.
 * @return      (OK/NG)             If the format is correct or not.
 */
int clf_object_series(const char *unpackedobjpath,int os_top_block_num, int ocmd_length, int ocmd_block_offset) {
  char ocm_buffer[LTOS_BLOCK_SIZE]        = { 0 };
  int current_position                    = 0;
  unsigned int dat_size                   = 0;
  uint64_t ocm_number_of_po               = 0;             //[Number of Packed objects] in OCM header
  uint64_t no_data                        = 0;
  uint64_t po_info_length[MAX_NUM_OF_PR]  = { 0 };         //[Packed Object Info Length] in Packed Object Info Directory
  uint64_t po_block_offset[MAX_NUM_OF_PR] = { 0 };         //[Block offset] in Packed Object Info Directory
  ST_SPTI_CMD_POSITIONDATA pos            = { 0 };
  int ret                                 = OK;
  set_top_verbose(DISPLAY_HEADER_AND_L432_INFO);

  if (unpackedobjpath == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at clf_object_series");
  }
#ifndef NO_TAPE
  //[TODO] Remove from here ...
  //   Move to OCM from RCM
  if (move_on_tape(SPACE_FILE_MARK_MODE, +2) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "Failed to space at 'Object Series'.\n");
  }
  if (read_position_on_tape(&pos) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "Failed to read at 'Object Series'.\n");
  }
  // ... to here because of format change. (Tentatively, locate to L2:OCM using by File mark)

  if (read_data(LTOS_BLOCK_SIZE, ocm_buffer, &dat_size) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "Failed to read ocm_buffer.\n");
  }
#else
  output_accdg_to_vl(OUTPUT_INFO, DEFAULT, "Can't check object series format without tape.\n");
  return OK;
#endif

  // An Object commit marker.(LEVEL 2)
  output_accdg_to_vl(OUTPUT_INFO, DISPLAY_HEADER_AND_L432_INFO, "Level 2\n");
  // OCM Header
  if (clf_header(OCM_IDENTIFIER, ocm_buffer, NULL, OFF, &current_position, &ocm_number_of_po, &no_data) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "OCM header format is not correct.\n");
  }
  // Packed Object Info Directory
  if (clf_directory(OCM_IDENTIFIER, ocm_buffer, &current_position, ocm_number_of_po, po_info_length, po_block_offset) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "OCM directory format is not correct.\n");
  }
  /* Check if Packed Objects Info format is correct or not.*/
  // Packed Object Info
  for (int i = 0; i < ocm_number_of_po; i++) {
    ret |= clf_packed_objects_info(unpackedobjpath, (unsigned char*) ocm_buffer, &current_position, po_info_length[i]);
  }

  // Packed objects.(LEVEL 1)
  // Move to the top of Objects Series from OCM.
  if (move_on_tape(SPACE_FILE_MARK_MODE, OBJECT_SERIES_FILE_MARK_NUM) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "Failed to space at 'Object Series'.n");
  }
  if (read_position_on_tape(&pos) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L432_INFO, "Failed to read at 'Object Series'.n");
  }
  // Packed object
  for (int i = 0; i < ocm_number_of_po; i++) {
    ret |= clf_packed_objects(unpackedobjpath, (int)(pos.blockNumber) + 1, po_info_length[i], po_block_offset[i]);
  }

  set_top_verbose(DEFAULT);
  return ret;
}
#endif // OBSOLETE
