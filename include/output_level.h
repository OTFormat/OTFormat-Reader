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
 * @file output_level.h
 * @brief Function declaration to output information according to verbose level.
 */

#ifndef OUTPUT_LEVEL_H_
#define OUTPUT_LEVEL_H_

#include <stdlib.h>

#define DISPLAY_COMMON_INFO                       ""       //FRAMEWORK for complete message.
#define DISPLAY_HEADER_INFO                       "v"      //LABEL for VOL1 label and OTFormat label
#define DISPLAY_HEADER_AND_L4_INFO                "vv"     //RCM for Reference Commit Marker
#define DISPLAY_HEADER_AND_L43_INFO               "vvv"    //PR for Partial Reference
#define DISPLAY_HEADER_AND_L432_INFO              "vvvv"   //OCM for Object Commit Marker
#define DISPLAY_HEADER_AND_L4321_INFO             "vvvvv"  //POINFO for Packed Object
#define DISPLAY_ALL_INFO                          "vvvvvv" //MISC for Cartridge Memory and others
#define DEFAULT                                   "default"

#define OUTPUT_SYSTEM_ERROR                       "[SYS_ERR] "
#define OUTPUT_ERROR                              "[ERROR  ] "
#define OUTPUT_WARNING                            "[WARNING] "
#define OUTPUT_INFO                               "[INFO   ] "
#define OUTPUT_DEBUG                              "[DEBUG  ] "
#define OUTPUT_TRACE                              "[TRACE  ] "

#define LOCATION_MAM                              "Medium Auxiliary Memory"
#define LOCATION_MAM_HTA                          "Medium Auxiliary Memory Host-type Attribute"
#define LOCATION_MAM_VCI                          "Medium Auxiliary Memory Volume Coherency Information"

#define INDENT                                    "                   " // message header length is 19 like "[ERROR  ] [LABEL ] "

void  set_vl(const char* const vl);
void  set_top_verbose(const char* const t_verbose);
const char* get_top_verbose();
void  set_c_mode(const char* const c_mode);
int   output_accdg_to_vl(const char* const log_level, const char* verbose, const char * restrict format, ...);

#endif /* OUTPUT_LEVEL_H_ */
