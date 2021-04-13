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
 * @brief Function declaration to issue a SCSI command.
 */

#ifndef SPTI_LIB_H_
#define SPTI_LIB_H_

#include <scsi_resparam.h>
#include <stdio.h>

#include <errno.h>
#include <stdint.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <limits.h>
#include <linux/limits.h>

#include "output_level.h"
#include "scsi_resparam.h"
#include "scsi/sg.h"

#define DATA_PARTITION                            (1)
#define REFERENCE_PARTITION                       (0)

typedef int BOOL;

#ifndef TRUE
#define TRUE 1
#endif // TRUE

#ifndef FALSE
#define FALSE 0
#endif // TRUE


/** Structure for SCSI device */
typedef struct scsi_device_param
{
    int fd_scsidevice;
} SCSI_DEVICE_PARAM;

SCSI_DEVICE_PARAM* init_scsi_device_param(const char* const path);
void destroy_scsi_device_param(SCSI_DEVICE_PARAM* p);

sg_io_hdr_t* init_sg_io_hdr(const unsigned char cmd_len,
                            unsigned char* const cmdp,
                            const int dxfer_direction,
                            const unsigned int dxfer_len,
                            void* const dxferp,
                            ST_SPTI_REQUEST_SENSE_RESPONSE* sbp);
void destroy_sg_io_hdr(sg_io_hdr_t* hdr);
BOOL run_scsi_command(void* const scparam, sg_io_hdr_t* const hdr, uint32_t* const resid);
uint64_t btoui(const unsigned char* const buf, const int size);

BOOL spti_locate(void* scparam, uint32_t blockAddress, ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data,
                 ST_SYSTEM_ERRORINFO* syserr);
BOOL spti_locate_partition(void* scparam, uint32_t partition, uint32_t blockAddress,
                           ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data, ST_SYSTEM_ERRORINFO* syserr);
BOOL spti_read_attribute(void* scparam, uint8_t partNo, uint8_t action, uint16_t id,
                         uint32_t reqSize, void* datBuffer, uint32_t* datSize,
                         ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data, ST_SYSTEM_ERRORINFO* syserr);
BOOL spti_read_data(void* scparam, uint32_t reqSize, void* datBuffer, uint32_t* datSize,
                    ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data, ST_SYSTEM_ERRORINFO* syserr);
BOOL spti_read_drive_attribute(void* scparam, uint8_t action, uint16_t id,
                               ST_SPTI_DEVICE_TYPE_ATTRIBUTE* attr_data,
                               ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data, ST_SYSTEM_ERRORINFO* syserr);
BOOL spti_read_drive_host_type_attribute(void* scparam, uint8_t action, uint16_t id,
                                         ST_SPTI_HOST_TYPE_ATTRIBUTE* attr_data,
                                         ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data,
                                         ST_SYSTEM_ERRORINFO *syserr);
BOOL spti_read_position(void* scparam, ST_SPTI_CMD_POSITIONDATA* pos,
                        ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data, ST_SYSTEM_ERRORINFO* syserr);
BOOL spti_rewind(void* scparam, ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data, ST_SYSTEM_ERRORINFO* syserr);
BOOL spti_space(void* scparam, uint8_t code, uint32_t blockAddress,
                ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data, ST_SYSTEM_ERRORINFO* syserr);
BOOL spti_test_unit_ready(void* scparam, ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data,
                          ST_SYSTEM_ERRORINFO* syserr);
BOOL spti_log_sense(void* scparam, uint32_t page_code, uint32_t parameter,
                    uint32_t dxfer_len, void* dxferp, uint32_t* resid,
                    ST_SPTI_REQUEST_SENSE_RESPONSE* sbp, ST_SYSTEM_ERRORINFO *syserr);

/* just for UT */
BOOL test_inquiry(const char* const path);
BOOL test_locate(const char* const path, const uint32_t block_address,
                 const char change_partition, const uint32_t partition);
BOOL test_log_sense(const char* const path);
BOOL test_read_attribute(const char* const path);
BOOL test_read_drive_attribute(const char* const path);
BOOL test_read_drive_host_type_attribute(const char* const path);
BOOL test_read_data(const char* const path);
BOOL test_read_position(const char* const path);
BOOL test_rewind(const char* const path);
BOOL test_space(const char* const path, const uint8_t code,
                const uint32_t block_address);
BOOL test_test_unit_ready(const char* const path);


#endif /* SPTI_LIB_H_ */

