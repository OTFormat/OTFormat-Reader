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
 * @file ltos_format_checker.c
 * 
 * OTFormat Checker, which is able to check if data written in a tape complies with OTFormat.
 * Both an expected and actual result will output on your console, if there is invalid data in the tape.
 * NOTE: When you build the OTFormat Checker, you need to remove the compile option "OBJ_READER".
 */
 
/* Debug Settings ------------------------- */
//#define SC_PACKED_OBJECT_CHECK_FLAG
#define TAPE_FORMAT_CHECK_FLAG
//#define NO_TAPE
/* ---------------------------------------- */

#include <unistd.h>
#include "ltos_format_checker.h"
#include "scsi_util.h"

#ifndef OBJ_READER
static int check_ltos_format(char *verbose_level, char *continue_mode, char *target_partition,
                             char *device_name, const char *unpackedobjpath);
//static int clf_volume(char* verbose_level, char* continue_mode, const char* unpackedobjpath,
//                      const MamVci* const mamvci, const MamHta* const mamhta, int which_partition);
#endif //OBJ_READER

#ifndef OBJ_READER
/**
 * Display this format check tool's usage message.
 * @return     (0)         If successful, this function doesn't return.
 *                         If a error occurs, a message is printed, and zero is returned.
 */
static void print_usage(char *appname)
{
  fprintf(stderr, "usage: %s <options>\n", appname);
  fprintf(stderr, "Available options are:\n");
  //secret option
  //fprintf(stderr, "  -p, --packedobjpath   = <path>  Specify packed object path.\n");
  //fprintf(stderr, "                                  If command -d is specified, it is ignored.\n");
  fprintf(stderr, "  -c, --continue        = <flag>  Specify continue mode.\n");
  fprintf(stderr, "                                  cont:Continue checking even if a error is found.\n");
  fprintf(stderr, "                                  exit:Stop checking if a error is found.\n");
  fprintf(stderr, "                                  default is exit.\n");
  fprintf(stderr, "  -d, --device          = <name>  Specify device name. default is /dev/sg0.\n");
  fprintf(stderr, "  -o, --outputpath      = <path>  Specify output path of packed object.\n");
  fprintf(stderr, "  -t, --target          = <name>  all:Check both Reference Partition(RP) and Data Partition(DP).\n");
  fprintf(stderr, "                                  rp:Check only RP.\n");
  fprintf(stderr, "                                  dp:Check only DP.\n");
  fprintf(stderr, "                                  default is all.\n");
  fprintf(stderr, "  -v, --verbose         = <level> Specify output_level.\n");
  fprintf(stderr, "                                  If this option is not set, nothing will be displayed.\n");
  fprintf(stderr, "                                  v:information about header.\n");
  fprintf(stderr, "                                  vv:information about L4 in addition to above.\n");
  fprintf(stderr, "                                  vvv:information about L3 in addition to above.\n");
  fprintf(stderr, "                                  vvvv:information about L2 in addition to above.\n");
  fprintf(stderr, "                                  vvvvv:information about all.\n");
  fprintf(stderr, "  -V, --version\n");
  fprintf(stderr, "  -h, --help\n");
}
#endif //OBJ_READER

#if !defined(OBJ_READER) && !defined(MONGODB_RESTORE_TOOL)
/* Command line options */
static const char *short_options    = "p:o:d:t:c:v:Vh";
static struct option long_options[] = {
  { "packedobjpath",   required_argument, 0, 'p' },
  { "outputpath",      required_argument, 0, 'o' },
  { "device",          required_argument, 0, 'd' },
  { "target",          required_argument, 0, 't' },
  { "continue",        required_argument, 0, 'c' },
  { "verbose",         required_argument, 0, 'v' },
  { "version",         no_argument,       0, 'V' },
  { "help",            no_argument,       0, 'h' },
  { 0,                    0,                    0,   0  }
};

int main(int argc, char **argv) {
  char device_name[DEVICE_NAME_SIZE + 1];
  device_name[0]       = EOS;
  char packedobjpath[PACKED_OBJ_PATH_SIZE + 1];
  packedobjpath[0]     = EOS;
  char outputpath[OUTPUT_PATH_SIZE + 1];
  outputpath[0]        = EOS;
  char target_partition[MAX_PATH + 1];
  snprintf(target_partition, PATH_MAX + 1, "%s", ALL);
  //target_partition     = ALL;
  char continue_mode[MAX_PATH + 1];
  snprintf(continue_mode, PATH_MAX + 1, "%s", EXIT);
  //continue_mode        = EXIT;
  char verbose_level[MAX_PATH + 1] = DISPLAY_COMMON_INFO;
  int ret              = OK;

  /* Parse command line options */
  while (1) {
    int option_index = 0;
    int c            = getopt_long(argc, argv,
                                   short_options, long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'p':
      snprintf(packedobjpath, PACKED_OBJ_PATH_SIZE + 1, "%s", optarg);
      break;
    case 'o':
      snprintf(outputpath, OUTPUT_PATH_SIZE + 1, "%s", optarg);
      break;
    case 'd':
      snprintf(device_name, DEVICE_NAME_SIZE + 1, "%s", optarg);
      break;
    case 't':
      snprintf(target_partition, PATH_MAX + 1, "%s", optarg);
      break;
    case 'c':
      snprintf(continue_mode, PATH_MAX + 1, "%s", optarg);
      break;
    case 'v':
      snprintf(verbose_level, PATH_MAX + 1, "%s", optarg);
      break;
    case 'V':
      fprintf(stderr, "%s\n", FORMAT_CHECKER_VERSION);
      exit(EXIT_SUCCESS);
    case 'h':
      print_usage(argv[0]);
      exit(EXIT_SUCCESS);
    default: break;
    }
  }
  set_vl(verbose_level);
  set_c_mode(continue_mode);

#ifdef SC_PACKED_OBJECT_CHECK_FLAG
  // Read the file, store it in memory.
  unsigned char *data_buf = NULL;
  off_t data_buf_size     = 0;
  data_buf                = read_file(packedobjpath, &data_buf_size);

  if (data_buf == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "Can't read file .\n", packedobjpath);
  } else if (check_packed_object_format(unpackedobjpath,data_buf) != 0) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_AND_L4321_INFO, "Packed object format is not correct.\n");
  }
  free(data_buf);
#endif

#ifdef TAPE_FORMAT_CHECK_FLAG
  // Check the format of data on tape.
  if (check_ltos_format(verbose_level, continue_mode, target_partition,device_name,outputpath) != 0) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_COMMON_INFO, "LTOS format is not correct.\n");
  }
#endif

  output_accdg_to_vl(OUTPUT_INFO, DISPLAY_COMMON_INFO, "OTFormat check is completed.\n");

  return ret;
}
#endif //OBJ_READER

#ifndef OBJ_READER
/**
 * Check LTOS format.
 * @param [in]  (verbose_level)    How much information to display.
 * @param [in]  (continue_mode)    Whether continue checking or not when a error is found.
 * @param [in]  (target_partition) Which partition(DP/RP) to be checked.
 * @param [in]  (device_name)      Scsi device name.
 * @param [in]  (unpackedobjpath)  Save path of unpacked object.
 * @return      (OK/NG)            If tape format is correct or not.
 */
static int check_ltos_format(char *verbose_level, char *continue_mode, char *target_partition,
                             char *device_name, const char *unpackedobjpath) {
  ST_SPTI_REQUEST_SENSE_RESPONSE sense_data = { 0 };
  ST_SYSTEM_ERRORINFO syserr                = { 0 };
  SCSI_DEVICE_PARAM scparam                 = { 0 };
  MamHta mamhta                             = { 0 };
  MamVci mamvci[NUMBER_OF_PARTITIONS]       = { { 0 } };
  int ret                                   = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:check_ltos_format\n");
  char command_buff[COMMAND_SIZE + 1]       = { '\0' };
  int fd                                    = ERROR;

  //Check if tape device exists.
  sprintf(command_buff, DEVICE_CHECK_COMMAND, device_name);
  if (WEXITSTATUS(system(command_buff)) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Can't find tape device(%s).\n"
                              "%sCheck option '-d'.\n", device_name, INDENT);
    return ret;
  }

  // Open scsi device.
  fd = open(device_name, O_RDWR);
  if (fd == ERROR) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Can't open file: %s\n"
                              "%sfd = %d, errno = %d: %s\n", device_name, INDENT, fd, errno, strerror(errno));
    return ret;
  }
  scparam.fd_scsidevice = fd;

  //[TODO] No operation confirmation. Check return code.
  static const int max_tur_count = 4;
  int i = 0;
  for (i = 0; i < max_tur_count; i++) { // Allow TUR error three times.
    if (spti_test_unit_ready(&scparam, &sense_data, &syserr) == FALSE) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "Failed to test unit ready.\n");
      sleep(1); // sleep 1 second.
    } else {
      break;
    }
  }
  if (i == max_tur_count) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to test unit ready.\n");
    return ret;
  }
  if (clf_check_mam_coherency(&scparam, mamvci, &mamhta) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "MAM Check Error\n");
  }

  // Set all pointers relating to SCSI control to utility module.
  set_device_pram(&scparam, &sense_data, &syserr);

  check_integrity(mamvci, &mamhta);

  /*
  // These clf_volume will be replaced with check_interity.
  if (strcmp(target_partition, RP) == 0 || strcmp(target_partition, ALL)==0) {
    clf_volume(verbose_level, continue_mode, unpackedobjpath, &mamvci[REFERENCE_PARTITION], &mamhta, REFERENCE_PARTITION);
  }
  if (strcmp(target_partition, DP) == 0 || strcmp(target_partition, ALL)==0) {
    clf_volume(verbose_level, continue_mode, unpackedobjpath, &mamvci[DATA_PARTITION], &mamhta, DATA_PARTITION);
  }
  */

  close(fd);
  fd = ERROR;
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :check_ltos_format\n");
  return ret;
}

/**
 * Check LTOS Volume format.
 * @param [in]  (verbose_level)    How much information to display.
 * @param [in]  (continue_mode)    Whether continue checking or not when a error is found.
 * @param [in]  (unpackedobjpath)  Save path of unpacked object.
 * @param [in]  (sense_data)       SCSI device parameter for ScsiLib.
 * @param [in]  (syserr)           SCSI device parameter for ScsiLib.
 * @param [in]  (scparam)          SCSI device parameter for ScsiLib.
 * @param [in]  (dat_size)         SCSI device parameter for ScsiLib.
 * @param [in]  (mamvci)           Pointer of a volume coherency information.
 * @param [in]  (mamhta)           Pointer of a host-type attributes.
 * @param [in]  (which_partition)  Which partition(DP/RP) to be checked.
 * @return      (OK/NG)            If the format is correct or not.
 */
/*
static int clf_volume(char *verbose_level, char *continue_mode,
                      const char *unpackedobjpath,
                      const MamVci* const mamvci,
                      const MamHta* const mamhta,
                      int which_partition) {
  int ret                               = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:clf_volume: Check %s partition.\n",
                                                             clf_get_partition_name(which_partition));
  uint64_t num_of_pr                    = 0;
  uint64_t rcm_pr_blocks[MAX_NUM_OF_PR] = { 0 };

  //[TODO] No operation confirmation. Check return code.
  if (set_tape_head(which_partition) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Can't locate to beginning of partition %d.\n", which_partition);
    return NG;
  }
  // Check VOL1 Label.
  if (clf_vol1_label(which_partition, verbose_level, continue_mode) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Vol1 Label format is not correct.\n");
  }
  // Check LTOS Label.
  if (clf_ltos_label(mamvci, mamhta) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "LTOS Label format is not correct.\n");
  }

  // Check full tape format.
  // First reference commit marker.(LEVEL 4)
  if (clf_reference_commit_marker(&num_of_pr, FIRST, rcm_pr_blocks, mamvci, mamhta) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "First reference commit marker format is not correct.\n");
  }

  // Last reference commit marker.(LEVEL 4)
  if (clf_reference_commit_marker(&num_of_pr, LAST, rcm_pr_blocks, mamvci, mamhta) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Last reference commit marker format is not correct.\n");
  }

  // Partial tapes.
  // [TODO] Pending: RP-L4
  // We don't check L1~3 in alpha version.
  if (which_partition == DATA_PARTITION) {
    for (int pr_num = 0; pr_num < num_of_pr; pr_num++) {
      ret |= clf_partial_tapes(unpackedobjpath, which_partition, rcm_pr_blocks[pr_num]);
    }
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :clf_volume: Check %s partition.\n",
                            clf_get_partition_name(which_partition));
  return ret;
}
*/
#endif //OBJ_READER
