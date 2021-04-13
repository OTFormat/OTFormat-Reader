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
 * @file object_reader.c
 *
 * OTFormat Reader, which is able to read a full or partial data written in a tape which complies with OTFormat.
 * NOTE: When you build the OTFormat Reader, you need to add the compile option "OBJ_READER".
 */

#include <unistd.h>
#include "ltos_format_checker.h"
#include "scsi_util.h"

#ifdef OBJ_READER
/**
 * Display object reader's usage message.
 * @return     (0)         If successful, this function doesn't return.
 *                         If a error occurs, a message is printed, and zero is returned.
 */
static void print_usage(char *appname)
{
  fprintf(stderr, "usage: %s <options>\n", appname);
  fprintf(stderr, "Available options are:\n");
  fprintf(stderr, "  -b, --bucket          = <name>   Specify a bucket name in which an object you specified is stored.\n");
  fprintf(stderr, "  -d, --drive           = <name>   Specify a device name of a tape drive.\n");
  fprintf(stderr, "  -F, --Force           : Avoid to check a disk space during either Full dump or Resume dump.\n");
  fprintf(stderr, "  -f, --full-dump       : Read all objects from a tape formatted with the OTFoarmt.\n");
  fprintf(stderr, "  -h, --help\n");
  fprintf(stderr, "  -i, --interval        : Output a progress to \"history.log\" during either Full dump or Resume dump.\n");
  fprintf(stderr, "  -L, --Level           = <value>  Specify output level. default is 0\n");
  fprintf(stderr, "                                   0: Object Data and Meta\n");
  fprintf(stderr, "                                   1: Packed Object\n");
  fprintf(stderr, "  -l, --list            : Output a list of all objects in each bucket stored in a tape.\n");
  fprintf(stderr, "  -o, --object-key      = <name>   Specify an object KEY.\n");
  fprintf(stderr, "  -O, --Object-id       = <ID or Option> Specify an Object version. default is \"latest\".\n");
  fprintf(stderr, "                          <ID>     Specify a versioned object ID, which will be shown in a list file.\n");
  fprintf(stderr, "                          <Option> Either \"latest\" or \"all\" is available.\n");
  fprintf(stderr, "                                   \"latest\" : Output ONLY the latest version object. \n");
  fprintf(stderr, "                                   \"all\"    : Output ALL versions with the Object-Key. \n");
  fprintf(stderr, "  -r, --resume-dump     : Resume a Full dump process when \"history.log\" file was updated.\n");
  fprintf(stderr, "  -s, --save-path       = <path>   Specify a full path where data will be stored. Default is the application path.\n");
  fprintf(stderr, "  -v, --verbose         = <level>  Specify output_level.\n");
  fprintf(stderr, "                                   If this option is not set, nothing will be displayed.\n");
  fprintf(stderr, "                                   v:information about header.\n");
  fprintf(stderr, "                                   vv:information about L4 in addition to above.\n");
  fprintf(stderr, "                                   vvv:information about L3 in addition to above.\n");
  fprintf(stderr, "                                   vvvv:information about L2 in addition to above.\n");
  fprintf(stderr, "                                   vvvvv:information about L1 in addition to above.\n");
  fprintf(stderr, "                                   vvvvvv:information about MISC for MAM and others in addition to above.\n");
}

/* Command line options */
static const char *short_options    = "b:d:Ffhi:L:lo:O:rs:v:";
static struct option long_options[] = {
  { "bucket",          required_argument, 0, 'b' },
  { "drive",           required_argument, 0, 'd' },
  { "Force",           no_argument,       0, 'F' },
  { "full-dump",       no_argument,       0, 'f' },
  { "help",            no_argument,       0, 'h' },
  { "interval",        required_argument, 0, 'i' },
  { "Level",           required_argument, 0, 'L' },
  { "list",            no_argument,       0, 'l' },
  { "object-key",      required_argument, 0, 'o' },
  { "Object-id",       required_argument, 0, 'O' }, // Oct 28, 2020 added instead of Version-id
  { "resume-dump",     no_argument,       0, 'r' },
  { "save-path",       required_argument, 0, 's' },
  { "verbose",         required_argument, 0, 'v' },
  { 0,                    0,                    0,   0  }
};


/**
 * Open a tape device.
 * @param [in]  (device_name)      Scsi device name.
 * @param [in]  (fd)               file descriptor.
 * @param [in] (scparam)           Pointer to a structure of SCSI_DEVICE_PARAM
 * @param [in] (sense_data)        Pointer to a structure of ST_SPTI_REQUEST_SENSE_RESPONSE
 * @param [in] (syserr)            Pointer to a structure of ST_SYSTEM_ERRORINFO
 * @param [in] (mamvci)            Pointer of a volume coherency information.
 * @param [in] (mamhta)            Pointer of a host-type attributes.
 * @return      (OK/NG)            If tape format is correct or not.
 */
static int open_drive(const char* const device_name, int* fd,
                       SCSI_DEVICE_PARAM* const scparam, ST_SPTI_REQUEST_SENSE_RESPONSE* const sense_data,
                       ST_SYSTEM_ERRORINFO* const syserr, MamVci* const mamvci, MamHta* const mamhta) {
  int ret                                   = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:open_drive\n");
  char command_buff[COMMAND_SIZE + 1]       = { '\0' };

  //Check if tape device exists.
  sprintf(command_buff, DEVICE_CHECK_COMMAND, device_name);
  if (WEXITSTATUS(system(command_buff)) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Can't find tape device(%s).\n"
                              "%sCheck option '-d'.\n", device_name, INDENT);
    return ret;
  }
  // Open scsi device.
  *fd = open(device_name, O_RDWR);

  if (*fd == ERROR) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Can't open file: %s\n"
                              "%sfd = %d, errno = %d: %s\n", device_name, INDENT, *fd, errno, strerror(errno));
    return ret;
  }
  scparam->fd_scsidevice = *fd;

  //[TODO] No operation confirmation. Check return code.
  static const int max_tur_count = 4;
  int i = 0;
  for (i = 0; i < max_tur_count; i++) { // Allow TUR error three times.
    if (spti_test_unit_ready(scparam, sense_data, syserr) == FALSE) {
      ret |= output_accdg_to_vl(OUTPUT_WARNING, DISPLAY_ALL_INFO, "Failed to test unit ready.\n");
      sleep(1); // sleep 1 second.
    } else {
      break;
    }
  }
  if (i == max_tur_count) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to open the device you specified.\n");
    return ret;
  }
  if (clf_check_mam_coherency(scparam, mamvci, mamhta) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "MAM Check Error\n");
  }

  // Set all pointers relating to SCSI control to utility module.
  set_device_pram(scparam, sense_data, syserr);

  output_accdg_to_vl(OUTPUT_INFO, DISPLAY_ALL_INFO, "Device: %s is opened.\n", device_name);
  return ret;
}


/**
 * Check if interval is valid.
 * @param [in]  (str_interval)   Block size in string.
 * @param [out] (interval)       Block size in integer.
 * @return      (OK/NG)          Return OK if no errors.
 */
static int check_interval(const char* const str_interval, uint32_t* const interval) {
  int ret = OK;

  for (uint32_t i = 0; i < strlen(str_interval); i++) {
    if (!isdigit(str_interval[i])) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_INFO,
          "Interval must be digit(s).\n");
    }
  }

  *interval = atoi(str_interval);
  //printf("interval=%u\n", *interval); //For debug

  if (*interval < (int)MIN_HISTORY_INTERVAL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_INFO,
        "Interval must be greater than %d seconds.\n", MIN_HISTORY_INTERVAL);
  }

  if ((int)MAX_HISTORY_INTERVAL < *interval) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_INFO,
        "Interval must be %d seconds or less.\n", MAX_HISTORY_INTERVAL);
  }

  return ret;
}

/**
 * Check if arguments are valid.
 * @param [in]  (is_drive_specified)       Boolean
 * @param [in]  (is_output_list)           Boolean
 * @param [in]  (is_resume_dump_required)  Boolean
 * @param [in]  (is_full_dump_required)    Boolean
 * @param [in]  (is_output_object)         Boolean
 * @param [in]  (bucket_name)              Bucket name in string
 * @param [in]  (object_key)               Object key in string.
 * @return      (OK/NG)                    Return OK if no errors.
 */
static int check_arguments(const Bool is_drive_specified, const Bool is_output_list, const Bool is_resume_dump_required,
                           const Bool is_full_dump_required, const Bool is_output_object,
                           const char* const bucket_name, const char* const object_key,
                           const char* const object_id,
                           const uint32_t structure_level) {
  int ret = OK;

  // Required argument check
  if (is_drive_specified == false) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "Please specify --drive option.\n");
  }
  if ( is_output_list        == false && is_resume_dump_required == false
    && is_full_dump_required == false && is_output_object        == false ) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
           "Please specify at least --full-dump, --resume-dump, --list, or both --bucket and --object option.\n");
  }
  //   Both bucket_name and object_key are required to output an object from a tape.
  if (is_output_object == true) {
    if ( strlen(bucket_name) < 1 || strlen(object_key) < 1 ) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "Please specify both --bucket and --object.\n");
    }
  }
  // Collision check
  if ((structure_level == OUTPUT_PACKED_OBJECT) && (strncmp(object_id, "all", 3) == OK)) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
               "You cannot specify \"-O all -L 1\".\n"
               "%sIf you want to get packed objects, please specify \"-O latest\" or \"-O {specific object id}\"", INDENT);
  }
  if (is_output_list == true) {
	  if (is_resume_dump_required == true && is_full_dump_required == true) {
		  ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
		             "Please specify either --full-dump, --resume-dump, or --list option.\n");
	  } else if (is_resume_dump_required == true) {
		  ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
		             "Please specify either --resume-dump or --list option.\n");
	  } else if (is_full_dump_required == true) {
		  ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
		             "Please specify either --full-dump or --list option.\n");
	  }
  }
  //   when -b, -L, -o or -O is specified, is_output_object will be "true".
  if (is_output_object == true) {
    if (is_resume_dump_required == true && is_full_dump_required == true) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
             "Please specify either --full-dump, --resume-dump, or both --bucket and --object option.\n");
    }
    if (is_resume_dump_required == true ) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
             "Please specify either --resume-dump or both --bucket and --object option.\n");
    }
    if (is_full_dump_required == true) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
             "Please specify either --full-dump or both --bucket and --object option.\n");
    }
  }
  if (is_resume_dump_required == true && is_full_dump_required == true) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
           "Please specify either --full-dump or --resume-dump.\n");
  }
  return ret;
}


int main(int argc, char **argv) {
  int ret                                                 = OK;
  char bucket_name[BUCKET_LIST_BUCKETNAME_MAX_SIZE + 1]   = { '\0' };
  char drive_name[DEVICE_NAME_SIZE + 1]                   = { '\0' };
  uint32_t structure_level                                = 0;                   // default = 0 (object)
  uint32_t history_interval                               = DEFAULT_HISTORY_INTERVAL; // default = 3600 sec
  Bool is_output_list                                     = false;
  Bool is_output_object                                   = false;
  Bool is_full_dump_required                              = false;
  Bool is_resume_dump_required                            = false;
  Bool is_force_enabled                                   = false;
  Bool is_drive_specified                                 = false;
  char object_key[MAX_KEY_SIZE + 1]                       = { '\0' };
  char object_id[UUID_SIZE + 1]                           = VERSION_OPT_LATEST;  // default = latest
  char save_path[OUTPUT_PATH_SIZE + 1]                    = { '\0' };
  char pr_file_path[OUTPUT_PATH_SIZE + 1]                 = { '\0' };
  char list_path[OUTPUT_PATH_SIZE + 1]                    = { '\0' };
  char verbose_level[OUTPUT_PATH_SIZE + 1]                = DISPLAY_COMMON_INFO;
  char barcode_id[BARCODE_SIZE + 1]                       = DEFAULT_BARCODE;
  int fd_tape                                             = ERROR;               // File descriptor for tape drive
  ST_SPTI_REQUEST_SENSE_RESPONSE sense_data               = { 0 };
  ST_SYSTEM_ERRORINFO syserr                              = { 0 };
  SCSI_DEVICE_PARAM scparam                               = { 0 };
  MamHta mamhta                                           = { 0 };
  MamVci mamvci[NUMBER_OF_PARTITIONS]                     = { { 0 } };
  object_list *objects                                    = NULL;
  uint64_t total_fm_num_in_rp                             = 0;
  uint64_t total_pr_num_in_rp                             = 0;
  time_t lap_start                                        = time(NULL);

  set_lap_start(lap_start);
  // Set default value in order to output an error during "Parse command line options"
  set_vl(verbose_level);
  set_c_mode(CONT);
  /* Parse command line options */
  while (1) {
    int option_index = 0;
    int c            = getopt_long(argc, argv,
                                   short_options, long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'b':
      snprintf(bucket_name, BUCKET_LIST_BUCKETNAME_MAX_SIZE + 1, "%s", optarg);
      if (strlen(bucket_name) < BUCKET_LIST_BUCKETNAME_MIN_SIZE) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
                                  "Bucket name length must be %d or longer.\n", BUCKET_LIST_BUCKETNAME_MIN_SIZE);
      }
      if (check_bucket_name(bucket_name) != OK) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "Bucket name is invalid.\n");
      }
      is_output_object = true;
      break;
    case 'd':
      snprintf(drive_name, DEVICE_NAME_SIZE + 1, "%s", optarg);
      is_drive_specified = true;
      break;
    case 'F': //Force
      is_force_enabled = true;
      break;
    case 'f': //full dump
      is_full_dump_required = true;
      break;
    case 'h':
      print_usage(argv[0]);
      exit(EXIT_SUCCESS);
    case 'i':
      if (check_interval(optarg, &history_interval) != OK) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "Interval value is invalid.\n");
      }
      set_history_interval(history_interval);
      break;
    case 'L':
      sscanf(optarg, "%u", &structure_level);
      if (structure_level > MAX_LEVEL_OPT_VALUE) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "Only 0 or 1 is allowed to be option of \"Level\".\n");
      }
      is_output_object = true;
      break;
    case 'l':
      is_output_list = true;
      break;
    case 'o':
      snprintf(object_key, MAX_KEY_SIZE + 1, "%s", optarg);
      is_output_object = true;
      break;
    case 'r':
      is_resume_dump_required = true;
      break;
    case 'O':
      snprintf(object_id, UUID_SIZE + 1, "%s", optarg);
      if (strcmp(object_id, "latest") != 0 && strcmp(object_id, "all") != 0) {
// comment out for tapes other than those made by Fujifilm.
//        if (check_uuid_format(object_id, "Object UUID", "main") != OK) {
//          ret |=
//              output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
//                  "Object_id is invalid. \"latest\", \"all\", or UUID with v4 format is allowed.\n");
//        }
       is_output_object = true;
      }
      break;
    case 's':
      snprintf(save_path, OUTPUT_PATH_SIZE + 1, "%s", optarg);
      set_obj_save_path(save_path);
      break;
    case 'v':
      snprintf(verbose_level, OUTPUT_PATH_SIZE + 1, "%s", optarg);
      break;
    default:
      break;
    }
  }
  //   Update verbose level.
  set_vl(verbose_level);

  // Step #1 Arguments check (Default setting, Required options and Collision check)
  // Default setting: If save_path is not specified, set the application path as default.
  if (strlen(save_path) < 1) {
    if (getcwd(save_path, sizeof(save_path))==NULL) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
             "Failed to get the application path. Specify a valid path with --save-path option.\n");
    }
  }
  // Required options and Collision check
  if (check_arguments(is_drive_specified, is_output_list, is_resume_dump_required,
                      is_full_dump_required, is_output_object, bucket_name, object_key, object_id, structure_level) != OK) {
    exit(EXIT_FAILURE); // Error reason will be output in the above function.
  }
  // Step #1-2 Initialize (=delete temporary files which were stored at the previous execution.)
  if (delete_files_in_directory(TEMP_PATH, NULL) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "Temporary files could not be deleted at %s.\n", TEMP_PATH);
  }

  // Step #2: Check if the disk space is greater than 100GB if --force is not specified.
  set_force_flag(is_force_enabled);
  if (check_disk_space(save_path, 0) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "Failed to check the disk space.\n");
  }

  // Step #3: Check if the tape drive user specified is accessible.
  if (open_drive(drive_name, &fd_tape, &scparam, &sense_data, &syserr, mamvci, &mamhta) != 0) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "Tape Drive is not accessible.\n");
  }
  // Get the barcode stored in MAM. Specify "null" if could not get or the first byte is space.
  if (mamhta.Data.barcode[0] != ' ') {
    strncpy(barcode_id, mamhta.Data.barcode, BARCODE_SIZE);
  }
  //printf(" barcode_id    =%s\n barcode in mam=%s\n", barcode_id, mamhta.Data.barcode); // for DEBUG

  char marker_file_path[MAX_PATH + 1] = { 0 };
  sprintf(marker_file_path, "%s/%s/reference_partition/OTFLabel", save_path, barcode_id);
  struct stat stat_mf;
  if (stat(marker_file_path, &stat_mf) == OK) {
    set_marker_file_flg(ON);
  }

  if (get_marker_file_flg()) {
    char marker_file_back[MAX_PATH + 1] = { 0 };
    char marker_file_ref[MAX_PATH + 1]  = { 0 };
    sprintf(marker_file_back, "%s/%s/reference_partition/*", save_path, barcode_id);
    sprintf(marker_file_ref, "reference_partition/");
    cp_dir(marker_file_back, marker_file_ref);
  }

  // Step #5 and #6: Check if this tape is formatted in OTFormat.
  if (check_reference_partition_lable(mamvci, &mamhta, &total_fm_num_in_rp) != 0) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "This tape is not formatted in OTFormat.\n");
  }
  // MEMO: After check_reference_partition_lable is complete correctly, tape position shall be the beginning of the first PR.


  // Step #7-1: Get the total number of PR(s) in Reference Partition.
  if (get_pr_num(&total_pr_num_in_rp)==NG) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L43_INFO, "Failed to get the number of Partial References.\n");
  } else {
    ret |= output_accdg_to_vl(OUTPUT_DEBUG, DISPLAY_HEADER_AND_L43_INFO, "main: pr_num=%lu\n", total_pr_num_in_rp);
  }
  // Step #7-2: Read all of PR(s) in Reference Partition, and output them to /tmp/object_reader/PR_<number>.
  for (uint64_t cur_pr_num = 0; cur_pr_num < total_pr_num_in_rp; cur_pr_num++) {
    snprintf(pr_file_path, OUTPUT_PATH_SIZE + 1, "%sPR_%lu", TEMP_PATH, cur_pr_num);
    if (write_markers_to_file(pr_file_path, ON) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_HEADER_AND_L43_INFO,
          "Failed to write partial reference to file.\n");
    }
  }

  // Step #8-1: Check options
  //   '--resume-dump'              : move to Step #17, then #18
  //   '--full-dump'                : move to Step #18
  //   '--list'                     : move to Step #9
  //   Not specified anything above : move to Step #11
  if (is_full_dump_required == true || is_resume_dump_required == true) {
    // Step #18: full dump
      // While EoD in Data Partition is found...
      // Sequentially read a data from the tape and save it to save_path/<bucket-name>_xxxx/yyyy/
      //  <bucket-name>_xxxx/yyyy/
      //       <object-key>/
      //              <Version-ID>.data
      //              <Version-ID>.meta
      // where, xxxx and yyyy shall be between 1 and 1000.
      // yyyy will increment by 1 for every 1,000 objects.     (When yyyy reaches 1001, reset to 1 and increment xxxx by 1.)
      // xxxx will increment by 1 for every 1,000,000 objects. (Max 1 billion objects is able to be stored in <bucket-name> directory.)

    struct stat stat_buf    = { 0 };
    if(stat(save_path, &stat_buf) != OK) {
      if(mkdir(save_path, stat_buf.st_mode) != OK) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to make directory.\n");
      }
    }
    if (is_full_dump_required == true) {
        if (check_integrity(mamvci, &mamhta, "full_dump" ,scparam, save_path, barcode_id) == NG) {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "Some error has occurred at check_integrity.\n");
        }
    } else if (is_resume_dump_required == true) {
        if (check_integrity(mamvci, &mamhta, "resume_dump" ,scparam, save_path, barcode_id) == NG) {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "Some error has occurred at check_integrity.\n");
        }
    }

    if (get_marker_file_flg() == OFF) {
      char marker_file_back[MAX_PATH + 1] = { 0 };
      char marker_file_ref[MAX_PATH + 1]  = { 0 };
      sprintf(marker_file_back, "%s/%s/reference_partition/", save_path, barcode_id);
      sprintf(marker_file_ref, "reference_partition/*");
      ret |= mk_deep_dir(marker_file_back);
      ret |= cp_dir(marker_file_ref, marker_file_back);
    }

    if (is_full_dump_required == true) {
    	ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_COMMON_INFO, "Full dump is complete.\n");
    } else if (is_resume_dump_required == true) {
    	ret |= output_accdg_to_vl(OUTPUT_INFO, DISPLAY_COMMON_INFO, "Resume dump is complete.\n");
    }
    exit(EXIT_SUCCESS);

  } else if (is_output_list == true) {
    // Step #9: Make a list of each bucket, and output it to save_path/<tape-barcode>/<bucket-name>.lst
    /*
    char* json_test  = (char*)clf_allocate_memory(STR_MAX, "json_test");
    make_key_str_value_pairs(&json_test, "aaa", "111");
    make_key_str_value_pairs(&json_test, "bbbb", "2222");
    make_key_str_value_pairs(&json_test, "c", "3");
    make_key_ulong_int_value_pairs(&json_test, "d", 4444);
    make_key_str_value_pairs(&json_test, "b", "5");
    printf(json_test);
    add_key_value_pairs_to_array_in_json_file("./jsontest", "testtest", json_test);
    */ //for DEBUG

    //delete all existing list files.
    char list_dir[MAX_PATH + 1] = { 0 };
    sprintf(list_dir, "%s/%s/", save_path, barcode_id);
    if (delete_files_in_directory(list_dir, ".lst") == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "Existing list files could not be deleted at %s.\n", list_dir);
    }

    if (check_integrity(mamvci, &mamhta, "output_list" ,scparam, save_path, barcode_id) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "Some error has occurred at check_integrity.\n");
    }
    //Complete all list files by adding "]}"
    ret |= comlete_list_files(list_dir);

    // Step #10-1: Delete temporary data
    if (delete_files_in_directory(TEMP_PATH, NULL) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_COMMON_INFO, "Temporary files could not be deleted at %s.\n", TEMP_PATH);
    }
    // Step #10-2: Check if both Object-key and Bucket are specified.
    //   True  : continue
    //   False : exit(EXIT_SUCCESS);
    ret = output_accdg_to_vl(OUTPUT_INFO, DISPLAY_COMMON_INFO, "Making and output the list file is complete.\n");
    if (strlen(object_key) > 0 && strlen(bucket_name) >= BUCKET_LIST_BUCKETNAME_MIN_SIZE) {
      output_accdg_to_vl(OUTPUT_INFO, DISPLAY_COMMON_INFO, "Continue to read the specified object from the tape.\n");
    } else {
      exit(EXIT_SUCCESS);
    }
  }

  // Step #11-1: Check if the list user specified exists in save_path/<tape-barcode>/<bucket-name>.lst
  //   True  : continue
  //   False : exit(EXIT_FAILURE);
  int get_list_info_flag = 0;
  for (int i = 1; i <= MAX_NUMBER_OF_LISTS; i++) {
    snprintf(list_path, OUTPUT_PATH_SIZE + 1, "%s/%s/%s_%04d.lst", save_path, barcode_id, bucket_name, i);
    if (check_file(list_path) != OK) {
      if (get_list_info_flag == 0) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO,
            "Specify \"--list\" option if you did not make a list before.\n"
            "%sIf already done it, the bucket you specified is not found.\n", INDENT);
      }
      snprintf(list_path, OUTPUT_PATH_SIZE + 1, "%s/%s/%s_%04d.lst", save_path, barcode_id, bucket_name, i - 1);
      break;
    }
  // Step #11-2: Check if the list has the object-key.
  //   True  : continue
  //   False : exit(EXIT_FAILURE);
  // Step #12-1: Get a physical block address of a Packed Object which includes the object.
  //   Refer to objects->block_adress
    if (get_object_info_in_list(object_key, object_id, list_path, &objects) == OK) {
      get_list_info_flag = 1;
    }
  }
  if (objects == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,"The object you specified was not found in the list(%s).\n", list_path);
  }

  if (check_integrity(mamvci, &mamhta, "output_objects_in_object_list", scparam, save_path, barcode_id, objects, bucket_name) == NG) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO, "Some error has occurred at check_integrity.\n");
  }

  if (structure_level == OUTPUT_PACKED_OBJECT) {
    uint32_t residual_cnt                    = 0;
    uint64_t num_of_obj                      = 0;
    uint64_t last_obj_dir_from               = 0;
    uint64_t last_obj_dir_to                 = 0;
    uint64_t last_obj_dir_from_block_address = 0;
    uint64_t last_obj_dir_to_block_address   = 0;
    uint64_t last_data_offset                = 0;
    uint64_t po_size                         = 0;
    char pack_id[UUID_SIZE + 1]              = { '\0' };
    char tape_data[LTOS_BLOCK_SIZE + 1]      = { '\0' };

    ret |= locate_to_tape(objects->block_address);
    if (read_data(LTOS_BLOCK_SIZE, tape_data, &residual_cnt) == NG) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to read data from tape.\n");
    }
    uint8_t* po_header = (uint8_t*)clf_allocate_memory(PO_HEADER_SIZE, "PO Header");
    memmove(po_header, tape_data + PO_IDENTIFIER_SIZE, PO_HEADER_SIZE);
    r64(BIG, po_header + DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE, &num_of_obj, 1);
    last_obj_dir_from = PO_IDENTIFIER_SIZE + PO_HEADER_SIZE + num_of_obj * PO_DIR_SIZE;
    last_obj_dir_to   = PO_IDENTIFIER_SIZE + PO_HEADER_SIZE + (num_of_obj + 1) * PO_DIR_SIZE;
    last_obj_dir_from_block_address = objects->block_address + last_obj_dir_from / LTOS_BLOCK_SIZE;
    last_obj_dir_to_block_address   = objects->block_address + last_obj_dir_to / LTOS_BLOCK_SIZE;
    uuid_unparse(po_header + DIRECTORY_OFFSET_SIZE + DATA_OFFSET_SIZE + NUMBER_OF_OBJECTS_SIZE, pack_id);
    free(po_header);
    po_header = NULL;
    memset(tape_data, 0, LTOS_BLOCK_SIZE);
    /*
    if (last_obj_dir_from_block_address != last_obj_dir_to_block_address) {//debug ==
      // in case of last_object_directory is on single block.
      // move to block, last_data_offset is on.
      locate_to_tape(last_obj_dir_from_block_address);
      // get last_data_offset.
      tape_data = (char*)clf_allocate_memory(LTOS_BLOCK_SIZE, "tape_data");
      if (read_data(LTOS_BLOCK_SIZE, tape_data, &residual_cnt) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to read data from tape.\n");
      }
      uint8_t* po_dir_last_block = (uint8_t*)clf_allocate_memory(LTOS_BLOCK_SIZE, "po_dir_last_block");
      memcpy(po_dir_last_block, tape_data, LTOS_BLOCK_SIZE);
      r64(BIG, po_dir_last_block + ((last_obj_dir_from + OBJECT_ID_SIZE + META_DATA_OFFSET_SIZE) % LTOS_BLOCK_SIZE), &last_data_offset, 1);
      po_size = PO_IDENTIFIER_SIZE + last_data_offset;
      free(po_dir_last_block);
    } else {
    */
      // in case of last_object_directory is on multiple blocks.
      // move to block, a first part of last_data_offset is on.
      ret |= locate_to_tape(last_obj_dir_from_block_address);
      // get a first part of last_data_offset.
      if (read_data(LTOS_BLOCK_SIZE, tape_data, &residual_cnt) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to read data from tape.\n");
      }
      uint8_t* last_po_dir = (uint8_t*)clf_allocate_memory(LTOS_BLOCK_SIZE, "last_po_dir");
      memmove(last_po_dir, tape_data + (last_obj_dir_from % LTOS_BLOCK_SIZE), LTOS_BLOCK_SIZE - (last_obj_dir_from % LTOS_BLOCK_SIZE));
      memset(tape_data, 0, LTOS_BLOCK_SIZE);
      // move to block, a second part of last_data_offset is on.
      locate_to_tape(last_obj_dir_to_block_address);
      // get a second part of last_data_offset.
      if (read_data(LTOS_BLOCK_SIZE, tape_data, &residual_cnt) == NG) {
        ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to read data from tape.\n");
      }
      memmove(last_po_dir + (LTOS_BLOCK_SIZE - (last_obj_dir_from % LTOS_BLOCK_SIZE)), tape_data, PO_DIR_SIZE);
      r64(BIG, last_po_dir + OBJECT_ID_SIZE + META_DATA_OFFSET_SIZE, &last_data_offset, 1);
      po_size = PO_IDENTIFIER_SIZE + last_data_offset;
      free(last_po_dir);
      last_po_dir = NULL;
      memset(tape_data, 0, LTOS_BLOCK_SIZE);
    //}
      // output po.
      long int remained_po_size = po_size;
      ret |= locate_to_tape(objects->block_address);
      int po_first_block_flag = 1;
      char po_path[MAX_PATH + 1] = { 0 };
      sprintf(po_path, "%s/%s.pack", save_path, pack_id);
      while (0 < remained_po_size) {
        if (po_first_block_flag == 1) {
          struct stat stat_buf;
          if (stat(po_path, &stat_buf) == OK) {
            break;
          }
          po_first_block_flag = 0;
        }
        if (read_data(LTOS_BLOCK_SIZE, tape_data, &residual_cnt) == NG) {
          ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Failed to read data from tape.\n");
        }
        ret |= write_object_and_meta_to_file(tape_data, MIN(LTOS_BLOCK_SIZE, remained_po_size), 0, po_path);
        remained_po_size = remained_po_size - LTOS_BLOCK_SIZE;
        memset(tape_data, 0, LTOS_BLOCK_SIZE);
      }

  }

  output_accdg_to_vl(OUTPUT_INFO, DISPLAY_COMMON_INFO, "All of the processes are complete.\n");

  //Close all of the tape devices.
  close(fd_tape);
  fd_tape = ERROR;

  free(objects);
  objects = NULL;
  return ret;
}


#endif // OBJ_READER
