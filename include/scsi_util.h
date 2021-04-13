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
 * @file scsi_util.h
 * @brief Function declaration to issue a wrapped SCSI command.
 */

#ifndef SCSI_UTIL_H
#define SCSI_UTIL_H

#include "ltos_format_checker.h"


void  set_device_pram(SCSI_DEVICE_PARAM* scsi_param, ST_SPTI_REQUEST_SENSE_RESPONSE* sense_data, ST_SYSTEM_ERRORINFO* err_info);

int read_data(uint32_t data_trans_len, void* data_pointer, uint32_t* residual_count);
int move_on_tape(uint8_t code, uint32_t block_address) ;
int read_position_on_tape(ST_SPTI_CMD_POSITIONDATA* pos);
int set_tape_head(const int which_partition);
int locate_to_tape(const uint32_t block_addres);
//int set_hexspeak(unsigned char* const data_buf, const int data_length);
//int read_data_from_multi_blocks(unsigned char* const buf, uint64_t* const offset, unsigned char* const out, const uint64_t length);

#endif /* SCSI_UTIL_H */
