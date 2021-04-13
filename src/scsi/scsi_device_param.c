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
 * @file scsi_device_param.c
 * @brief Functions to manage a SCSI device
 */

#include <string.h>
#include "spti_lib.h"

/**
 * Create SCSI_DEVICE_PARAM object
 *
 * @param  path [i] path of file descriptor
 * @return SCSI_DEVICE_PARAM object pointer
 */
SCSI_DEVICE_PARAM* init_scsi_device_param(const char* const path) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:init_scsi_device_param\n");
  static const int size = sizeof(SCSI_DEVICE_PARAM);
  SCSI_DEVICE_PARAM* const sdp = (SCSI_DEVICE_PARAM*)calloc(1, size);

  if (sdp) {
    sdp->fd_scsidevice = open(path, O_RDWR);
  }

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :init_scsi_device_param\n");
  return sdp;
}

/**
 * Release SCSI_DEVICE_PARAM object resources
 *
 * @param sdp Pointer of SCSI_DEVICE_PARAM object
 */
void destroy_scsi_device_param(SCSI_DEVICE_PARAM* sdp) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:destroy_scsi_device_param\n");
  close(sdp->fd_scsidevice);
  free(sdp);
  sdp = NULL;
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :destroy_scsi_device_param\n");
}
