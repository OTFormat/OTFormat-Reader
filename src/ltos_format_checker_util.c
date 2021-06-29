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
 * @file ltos_format_checker_util.c
 * 
 * Common functions to check if a data written in a tape complies with OTFormat.
 */

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include "ltos_format_checker.h"
#include <time.h>
#include <locale.h>
#include <openssl/md5.h>

//for obj_reader
static time_t   lap_start                           = 0;
static time_t   lap_end                             = 0;
static uint32_t history_interval                    = 0;
static char     obj_save_path[OUTPUT_PATH_SIZE + 1] = { '\0' };

int scandir (const char *__restrict __dir,
        struct dirent ***__restrict __namelist,
        int (*__selector) (const struct dirent *),
        int (*__cmp) (const struct dirent **,
          const struct dirent **));

static int read_or_write_ini(const int rw, const char* ini_path, const char* section, const char* key, char** value);
static int update_bucket_info_4_obj_reader(BucketInfo4ObjReader** bucket_info_4_obj_reader);
static int mk_a_dir(const char *dirpath);

/**
 * Set object save path.
 * @param [in] (save_path) Object save path.
 */
void set_obj_save_path(char save_path[OUTPUT_PATH_SIZE + 1]) {
  //obj_save_path = save_path;
  strcpy(obj_save_path, save_path);
}

/**
 * Set measurement start time.
 * @param [in] (lap_s) Measurement start time.
 */
void set_lap_start(time_t lap_s) {
  lap_start = lap_s;
}

/**
 * Set interval specified in command line.
 * @param [in] (history_i) Interval option specified on the command line.
 */
void set_history_interval(uint32_t history_i) {
  history_interval = history_i;
}

/**
 * Get interval tme.
 * @param [in/out] (interval) interval time(sec)
 * @return         (OK/NG)    If success, return OK. Otherwise, return NG.
 */
int get_interval(long int* interval) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_interval\n");

  lap_end = time(NULL);
  *interval = lap_end - lap_start;
  if (history_interval < *interval) {
    lap_start = time(NULL);
  }

  return ret;
}

/**
 * add key and value pairs to array in json file.
 * @param [in] (new_list_flag)   Whether the list file is new(1) or not(0).
 * @param [in] (fp_list)         file pointer of the list file.
 * @param [in] (json_path)       json file path.
 * @param [in] (key_value_pairs) Key and value pairs to add.
 * @return     (OK/NG)           If success, return OK. Otherwise, return NG.
 */
int add_key_value_pairs_to_array_in_json_file(int new_list_flag, FILE* fp_list, const char* json_path,
                                       const char* key_value_pairs) {
  int ret = OK;

  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:add_key_value_pairs_to_array_in_json_file\n");

  struct stat stat_buf = { 0 };
  char* dirpath  = (char*)clf_allocate_memory(strlen(json_path), "dirpath");
  extract_dir_path(json_path, dirpath);
  if(stat(dirpath, &stat_buf) != OK) {
    if(mkdir(dirpath, stat_buf.st_mode) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to make directory.\n");
    }
  }

  if (new_list_flag == 1) {
      char* json_top  = (char*)clf_allocate_memory(sizeof(ARRAY_KEY) + 10, "json_top");
      sprintf(json_top, "{\n\"%s\":[\n{\n", ARRAY_KEY);
      fprintf(fp_list, json_top);
      fprintf(fp_list, key_value_pairs);
      fprintf(fp_list, "\n}\n");
      free(json_top);
      json_top = NULL;
    }
  else {
    fprintf(fp_list, ",\n{\n");
    fprintf(fp_list, key_value_pairs);
    fprintf(fp_list, "\n}\n");
  }
  free(dirpath);
  dirpath = NULL;
  return ret;
}

/**
 * add key and str value pairs to json_obj.
 * @param [in/out] (json_obj) characters of key value pairs.
 * @param [in]     (key)      key to add to json_obj.
 * @param [in]     (value)    str value to add to json_obj.
 * @return         (OK/NG)    If success, return OK. Otherwise, return NG.
 */
int make_key_str_value_pairs(char** json_obj, const char* key, const char* value) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:make_key_value_pairs\n");
  char* key_for_duplicate_check  = (char*)clf_allocate_memory(strlen(key) + 4, "key_for_duplicate_check");
  sprintf(key_for_duplicate_check, "\"%s\":", key);
  if (strstr(*json_obj, key_for_duplicate_check) != NULL) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "The key(%s) is duplicated.\n", key);
  }
  free(key_for_duplicate_check);
  key_for_duplicate_check = NULL;
  if (strlen(*json_obj) == 0) {
    sprintf(*json_obj, "\"%s\":\"%s\"", key, value);
  } else {
    sprintf(*json_obj, "%s,\n\"%s\":\"%s\"", *json_obj, key, value);
  }
  return ret;
}

/**
 * add key and ulong int value pairs to json_obj.
 * @param [in/out] (json_obj) characters of key value pairs.
 * @param [in]     (key)      key to add to json_obj.
 * @param [in]     (value)    ulong int value to add to json_obj.
 * @return         (OK/NG)    If success, return OK. Otherwise, return NG.
 */
int make_key_ulong_int_value_pairs(char** json_obj, const char* key, const uint64_t value) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:make_key_ulong_int_value_pairs\n");
  char* key_for_duplicate_check  = (char*)clf_allocate_memory(strlen(key) + 4, "key_for_duplicate_check");
  sprintf(key_for_duplicate_check, "\"%s\":", key);
  if (strstr(*json_obj, key_for_duplicate_check) != NULL) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "The key(%s) is duplicated.\n", key);
  }
  free(key_for_duplicate_check);
  key_for_duplicate_check = NULL;
  if (strlen(*json_obj) == 0) {
    sprintf(*json_obj, "\"%s\":%ld", key, value);
  } else {
    sprintf(*json_obj, "%s,\n\"%s\":%ld", *json_obj, key, value);
  }
  return ret;
}

/**
 * read property file and find value of specified key.
 * @param [in]  (file_path) property file path.
 * @param [in]  (key)       key.
 * @param [out] (value)     value.
 * @return      (OK/NG)     If success, return OK. Otherwise, return NG.
 */
int read_property(const char* file_path, const char* key, char** value) {
  int ret = OK;
  ret = OK; //= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:read_property\n");

  FILE* fp;
  char search_key[MAX_PATH] = { 0 };
  char str[STR_MAX];
  sprintf(search_key, "%s=",key);
  if ((fp = fopen(file_path, "r")) == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "fopen error(%s).\n", file_path);
  }
  while( fgets(str, STR_MAX, fp) != NULL ) {
    if (strncmp(search_key, str, strlen(search_key)) == 0) {
      char* temp_value  = str_substring(str, strlen(search_key), strlen(str) - strlen(search_key));
      strcpy(*value, temp_value);
      free_safely(&temp_value);
      break;
    }
    memset(str, 0, sizeof(str));
  }
  fclose(fp);
  fp = NULL;

  return ret;
}


/**
 * Read or write ini file .
 * @param [in]     (rw)       0:read, 1:write
 * @param [in]     (ini_path) ini file path
 * @param [in]     (section)  section
 * @param [in]     (key)      key
 * @param [in/out] (value)    value
 * @return         (OK/NG)    If success, return OK. Otherwise, return NG.
 */
static int read_or_write_ini(const int rw, const char* ini_path, const char* section, const char* key, char** value) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:read_or_write_ini\n");

  FILE* fp      = NULL;
  FILE* wfp     = NULL;
  Bool add_flag = false;
  char str[STR_MAX];
  char str_back[STR_MAX];
  char* ini_file_temp = (char*)clf_allocate_memory(strlen(ini_path) + 6, "ini_file_temp");
  sprintf(ini_file_temp, "%s_temp", ini_path);
  struct stat stat_buf    = { 0 };
  if(stat(ini_path, &stat_buf) != OK) {
    if ((fp = fopen(ini_path, "w")) == NULL) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "fopen error(%s).\n", ini_path);
    }
    fclose(fp);
    fp = NULL;
  }

  if ((fp = fopen(ini_path, "r")) == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "fopen error(%s).\n", ini_path);
  }
  if ((wfp = fopen(ini_file_temp, "w")) == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "fopen error(%s).\n", ini_file_temp);
  }

  while(1) {
    memset(str, 0, sizeof(str));
    if (fgets(str, STR_MAX, fp) == NULL) { // If there is no specified section.
      if (rw == 0) {
        ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "section(%s) not found.\n", section);
      } else if (rw == 1) {
        if (add_flag == false) {
          memset(str, 0, sizeof(str));
          sprintf(str, "[%s]\n%s=%s\n", section, key, *value);
          fputs(str, wfp); // Add the specified key and section.
        }
      }
      fclose(wfp);
      wfp = NULL;
      fclose(fp);
      fp  = NULL;
      remove(ini_path);
      rename(ini_file_temp, ini_path);
      free(ini_file_temp);
      ini_file_temp = NULL;
      return ret;
    }
    if (!strcmp(str, "\n")) {
      continue;
    }
    if (!strncmp(str, "[", 1)) {
      char* temp_section  = str_substring(str, 1, strlen(str) - 3);
      if (!strncmp(temp_section, section, strlen(section))) {
        free_safely(&temp_section);
        fputs(str, wfp); // Write the existing section specified.
        while(1) { // Check keys of the existing section specified.
          memset(str, 0, sizeof(str));
          if (fgets(str, STR_MAX, fp) == NULL) { // When the specified key does not exist in the existing last section specified.
            if (rw == 0) {
              ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "key(%s) is not found at the section(%s).\n", key, section);
            }
            if (add_flag == false) {
              memset(str, 0, sizeof(str));
              sprintf(str, "%s=%s\n", key, *value);
              fputs(str, wfp);
            }
            fclose(wfp);
            wfp = NULL;
            fclose(fp);
            fp  = NULL;
            remove(ini_path);
            rename(ini_file_temp, ini_path);
            free(ini_file_temp);
            ini_file_temp = NULL;
            return ret;
          }
          if (!strncmp(str, "[", 1)) {
            memmove(str_back, str, sizeof(str));
            memset(str, 0, sizeof(str));
            sprintf(str, "%s=%s\n", key, *value);
            if (add_flag == false) {
              fputs(str, wfp); // When the specified key does not exist in the existing other than the last section specified.
            }
            fputs(str_back, wfp); // Write the next section.
            add_flag = true;
            break;
          }
          if (!strcmp(str, "\n")) {
            continue;
          }
          char* temp_str  = str_substring(str, strlen(key), 1);
          if ((!strncmp(key, str, strlen(key))) && (!strncmp(temp_str, "=", 1))) { // When the specified section and key exists.
            free_safely(&temp_str);
            if (rw == 0) {
              char* temp_value  = str_substring(str, strlen(key) + 1, strlen(str) - strlen(key) - 1);
              strcpy(*value, temp_value);
              free_safely(&temp_value);
              free(ini_file_temp);
              ini_file_temp = NULL;
              return ret;
            } else if (rw == 1) {
              memset(str, 0, sizeof(str));
              sprintf(str, "%s=%s\n", key, *value);
              fputs(str, wfp); // Update the existing key specified.
              add_flag = true;
              }
          } else {
            fputs(str, wfp); // Write the existing key not specified.
          }
          free_safely(&temp_str);
          }
      } else {
        fputs(str, wfp); // Write the existing section not specified.
      }
      free_safely(&temp_section);
    } else {
      fputs(str, wfp); // Write the key of the existing section not specified.
    }
  }

  fclose(wfp);
  wfp = NULL;
  fclose(fp);
  fp  = NULL;
  remove(ini_path);
  rename(ini_file_temp, ini_path);
  free(ini_file_temp);
  ini_file_temp = NULL;
  return ret;
}

/**
 * Output history file.
 * @param [in]     (tape_id)  barcode id
 * @param [in]     (pr_cnt)   What number is the pr, which include the last output object, from the beginning.
 * @param [in]     (ocm_cnt)  What number is the ocm, which include the last output object, from the beginning.
 * @param [in]     (po_cnt)   What number is the po, which include the last output object, from the beginning.
 * @param [in/out] (obj_cnt)  How many objects have been read.
 * @return         (OK/NG)    If success, return OK. Otherwise, return NG.
 */
int output_history(const char* tape_id, const uint64_t pr_cnt, const uint64_t ocm_cnt, const uint64_t po_cnt, const uint64_t obj_cnt) {
  int ret = OK;
  long int interval = 0;
  get_interval(&interval);
  if ((interval < history_interval) && (check_disk_space(obj_save_path, 0) == OK)) {
    return ret;
  }

  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:output_history\n");

  char* char_pr_cnt  = (char*)clf_allocate_memory(STR_MAX, "char_pr_cnt");
  char* char_ocm_cnt = (char*)clf_allocate_memory(STR_MAX, "char_ocm_cnt");
  char* char_po_cnt  = (char*)clf_allocate_memory(STR_MAX, "char_po_cnt");
  char* char_obj_cnt = (char*)clf_allocate_memory(STR_MAX, "char_obj_cnt");

  sprintf(char_pr_cnt, "%lu", pr_cnt);
  sprintf(char_ocm_cnt, "%lu", ocm_cnt);
  sprintf(char_po_cnt, "%lu", po_cnt);
  sprintf(char_obj_cnt, "%lu", obj_cnt);

  read_or_write_ini(1, "./history.log", tape_id, "PR", &char_pr_cnt);
  read_or_write_ini(1, "./history.log", tape_id, "OCM", &char_ocm_cnt);
  read_or_write_ini(1, "./history.log", tape_id, "PO", &char_po_cnt);
  read_or_write_ini(1, "./history.log", tape_id, "Object number", &char_obj_cnt);

  free(char_pr_cnt);
  char_pr_cnt   = NULL;
  free(char_ocm_cnt);
  char_ocm_cnt  = NULL;
  free(char_po_cnt);
  char_po_cnt   = NULL;
  free(char_obj_cnt);
  char_obj_cnt  = NULL;

  if (interval < history_interval) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_COMMON_INFO, "No disk space(%s).\n", obj_save_path);
  }

  return ret;
}

/**
 * Get history information from history.log.
 * @param [out]    (tape_id)  barcode id
 * @param [out]    (pr_cnt)   What number is the pr, which include the last output object, from the beginning.
 * @param [out]    (ocm_cnt)  What number is the ocm, which include the last output object, from the beginning.
 * @param [out]    (po_cnt)   What number is the po, which include the last output object, from the beginning.
 * @param [in/out] (obj_cnt)  How many objects have been read.
 * @return         (OK/NG)    If success, return OK. Otherwise, return NG.
 */
int get_history(const char* tape_id, uint64_t* pr_cnt, uint64_t* ocm_cnt, uint64_t* po_cnt, uint64_t* obj_cnt) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_history\n");

  struct stat stat_buf    = { 0 };
  if(stat("./history.log", &stat_buf) != OK) {
    return NG;
  }
  char* char_pr_cnt  = (char*)clf_allocate_memory(STR_MAX, "char_pr_cnt");
  char* char_ocm_cnt = (char*)clf_allocate_memory(STR_MAX, "char_ocm_cnt");
  char* char_po_cnt  = (char*)clf_allocate_memory(STR_MAX, "char_po_cnt");
  char* char_obj_cnt = (char*)clf_allocate_memory(STR_MAX, "char_obj_cnt");
  read_or_write_ini(0, "./history.log", tape_id, "PR", &char_pr_cnt);
  read_or_write_ini(0, "./history.log", tape_id, "OCM", &char_ocm_cnt);
  read_or_write_ini(0, "./history.log", tape_id, "PO", &char_po_cnt);
  read_or_write_ini(0, "./history.log", tape_id, "Object number", &char_obj_cnt);
  *pr_cnt  = atol(char_pr_cnt);
  *ocm_cnt = atol(char_ocm_cnt);
  *po_cnt  = atol(char_po_cnt);
  *obj_cnt = atol(char_obj_cnt);
  free(char_pr_cnt);
  char_pr_cnt  = NULL;
  free(char_ocm_cnt);
  char_ocm_cnt = NULL;
  free(char_po_cnt);
  char_po_cnt  = NULL;
  free(char_obj_cnt);
  char_obj_cnt = NULL;

  return ret;
}

/**
 * Initialize BucketInfo4ObjReader .
 * @param [in/out] (bucket_info_4_obj_reader) bucket info structure
 * @param [in]     (obj_reader_saveroot)      object save path
 * @return         (OK/NG)                    If success, return OK. Otherwise, return NG.
 */
int initialize_bucket_info_4_obj_reader(BucketInfo4ObjReader** bucket_info_4_obj_reader, char* obj_reader_saveroot) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:initialize_bucket_info_4_obj_reader\n");
  struct dirent** namelist = NULL;
  int bucket_num = scandir(obj_reader_saveroot, &namelist, NULL, NULL);
  if (bucket_num == -1) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "scandir error(%s).\n", obj_reader_saveroot);
  }

  struct stat stat_buf       = { 0 };
  char dirpath[MAX_PATH + 1] = { 0 };

  for (int i = 0; i < bucket_num; ++i) {
    for (int j = 1000; 0 < j; j--) {
      memset(dirpath, 0, sizeof(dirpath));
      sprintf(dirpath, "%s/%s/%04d", obj_reader_saveroot, namelist[i]->d_name, j);
      if (!stat(dirpath, &stat_buf)) {//bucketname\xxxxパスが存在する
        for (int k = 1000; 0 < k; k--) {
          memset(dirpath, 0, sizeof(dirpath));
          sprintf(dirpath, "%s/%s/%04d/%04d", obj_reader_saveroot, namelist[i]->d_name, j, k);
          if (!stat(dirpath, &stat_buf)) {//bucketname\xxxx\yyyyパスが存在する。
            add_bucket_info_4_obj_reader(bucket_info_4_obj_reader, namelist[i]->d_name, 1000, j, k);
            j = -1;
            k = -1;
          }
        }
      }
    }
    free(namelist[i]);
    namelist[i] = NULL;
  }
  free(namelist);
  namelist = NULL;
  return ret;
}

/**
 * Add bucket info.
 * @param [in/out] (bucket_info_4_obj_reader) bucket info
 * @param [in]     (bucket_name)              bucket name
 * @param [in]     (savepath_dir_number)      savepath_dir_number
 * @param [in]     (savepath_sub_dir_number)  savepath_sub_dir_number
 * @return         (OK/NG)                    If success, return OK. Otherwise, return NG.
 */
int add_bucket_info_4_obj_reader(BucketInfo4ObjReader** bucket_info_4_obj_reader,
                                 char* bucket_name, int obj_reader_saved_counter, int savepath_dir_number, int savepath_sub_dir_number) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:add_bucket_info_4_obj_reader\n");
  struct BucketInfo4ObjReader* add_bucket_info = (BucketInfo4ObjReader*)clf_allocate_memory(sizeof( struct BucketInfo4ObjReader), "add_bucket_info");
  strcpy((add_bucket_info->bucket_name), bucket_name);
  add_bucket_info->obj_reader_saved_counter = obj_reader_saved_counter;
  add_bucket_info->savepath_dir_number      = savepath_dir_number;
  add_bucket_info->savepath_sub_dir_number  = savepath_sub_dir_number;
  add_bucket_info->next                     = NULL;

  if ((*bucket_info_4_obj_reader) == NULL) {
    (*bucket_info_4_obj_reader) = add_bucket_info;
    return ret;
  }
  struct BucketInfo4ObjReader* current;
  current = *bucket_info_4_obj_reader;
  while(1) {
    if (!strcmp(current->bucket_name, bucket_name)) {
      free(add_bucket_info);
      add_bucket_info = NULL;
      break;
    } else if (current->next == NULL){
      current->next = add_bucket_info;
      break;
    }
    current = current->next;
  }
  return ret;
}

/**
 * Get bucket info.
 * @param [in]  (bucket_info_4_obj_reader) bucket info
 * @param [in]  (bucket_name)              bucket name
 * @param [out] (savepath_dir_number)      xxxx ({save_path}\{bucket name}\xxxx\yyyy)
 * @param [out] (savepath_sub_dir_number)  yyyy ({save_path}\{bucket name}\xxxx\yyyy)
 * @return      (OK/NG)                    If success, return OK. Otherwise, return NG.
 */
int get_bucket_info_4_obj_reader(BucketInfo4ObjReader** bucket_info_4_obj_reader, char* bucket_name,
                                 int* savepath_dir_number, int*  savepath_sub_dir_number) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_bucket_info_4_obj_reader\n");

  struct BucketInfo4ObjReader* current = NULL;
  current = (*bucket_info_4_obj_reader);
  while(1) {
    if (!strcmp(current->bucket_name, bucket_name)) {
      update_bucket_info_4_obj_reader(&current);
      *savepath_dir_number = current->savepath_dir_number;
      *savepath_sub_dir_number = current->savepath_sub_dir_number;
      break;
    } else if (current->next == NULL){
      break;
    }
    current = current->next;
  }
  return ret;
}

/**
 * Update bucket info.
 * @param [in/out] (bucket_info_4_obj_reader) bucket info
 * @return         (OK/NG)                    If success, return OK. Otherwise, return NG.
 */
static int update_bucket_info_4_obj_reader(BucketInfo4ObjReader** bucket_info_4_obj_reader) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:update_bucket_info_4_obj_reader\n");

  if (OBJ_READER_MAX_SAVE_NUM <= (*bucket_info_4_obj_reader)->obj_reader_saved_counter) {
    (*bucket_info_4_obj_reader)->obj_reader_saved_counter = 0;
    if (OBJ_READER_MAX_SAVE_NUM <= (*bucket_info_4_obj_reader)->savepath_sub_dir_number) {
      (*bucket_info_4_obj_reader)->savepath_sub_dir_number = 1;
      if (OBJ_READER_MAX_SAVE_NUM <= (*bucket_info_4_obj_reader)->savepath_dir_number) {
        if ((*bucket_info_4_obj_reader)->savepath_dir_number == OBJ_READER_MAX_SAVE_NUM) {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_COMMON_INFO,
                                    "The maximum number of files that can be saved in the bucket(%s) has been exceeded.\n",
                                    (*bucket_info_4_obj_reader)->bucket_name);
    	   }
        (*bucket_info_4_obj_reader)->savepath_dir_number      = OBJ_READER_MAX_SAVE_NUM + 1;
        (*bucket_info_4_obj_reader)->savepath_sub_dir_number  = OBJ_READER_MAX_SAVE_NUM;
        (*bucket_info_4_obj_reader)->obj_reader_saved_counter = OBJ_READER_MAX_SAVE_NUM -1;
      } else {
        (*bucket_info_4_obj_reader)->savepath_dir_number++;
      }
    } else {
      (*bucket_info_4_obj_reader)->savepath_sub_dir_number++;
    }
  }
  (*bucket_info_4_obj_reader)->obj_reader_saved_counter++;

  return ret;
}

/**
 * Make a bottom layer directory.
 * @param [in]  (dirpath) directory path
 * @return      (OK/NG)   If success, return OK. Otherwise, return NG.
 */
static int mk_a_dir(const char *dirpath) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:stat_mkdir\n");

  struct stat stat_buf    = { 0 };
  if(stat(dirpath, &stat_buf) != OK) {
    if(mkdir(dirpath, stat_buf.st_mode) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to make directory.\n");
    }
  }

  return ret;
}

/**
 * Make directories from full path.
 * @param [in]  (dirpath) directory path
 * @return      (OK/NG)   If success, return OK. Otherwise, return NG.
 */
int mk_deep_dir(const char *dirpath) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:extract_dir_path\n");

  char *p   = NULL;
  char *buf = NULL;

  buf = (char*)clf_allocate_memory(strlen(dirpath) + 4, "dirpath");
  strcpy(buf, dirpath);
  for (p = strchr(buf + 1, '/'); p; p = strchr(p + 1, '/')) {
    *p = '\0';
    if (mk_a_dir(buf) != 0) {
      ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "stat_mkdir error.\n");
    }
    *p = '/';
  }
  free(buf);
  buf = NULL;

  return ret;
}

/**
 * Copy directory.
 * @param [in]  (dirpath_from) directory path
 * @param [in]  (dirpath_to)   directory path
 * @return      (OK/NG)        If success, return OK. Otherwise, return NG.
 */
int cp_dir(const char *dirpath_from, const char *dirpath_to) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:cp_dir\n");
  char command_buff[COMMAND_SIZE + 1]       = { '\0' };

  sprintf(command_buff, "cp -rf %s %s", dirpath_from, dirpath_to);
  if (WEXITSTATUS(system(command_buff)) != OK) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to copy (%s) to (%s).\n"
                              , dirpath_from, dirpath_to);
    return ret;
  }

  return ret;
}

/**
 * Extract directory path from file path.
 * @param [in]  (filepath) file path
 * @param [out] (dirpath)  directory path
 * @return      (OK/NG)    If success to extract directory path, return OK. Otherwise, return NG.
 */
int extract_dir_path(const char* restrict filepath, char* dirpath) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:extract_dir_path\n");

  char* dir_end = strrchr(filepath, '/');
  if (dir_end == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "filename only.\n");
  }
  for (uint32_t i = 0; i < strlen(filepath); i++) {
    dirpath[i] = filepath[i];
    if (filepath + i == dir_end) {
      break;
    }
  }
  return ret;
}

/**
 * Get result of md5sum.
 * @param [in]  (src)   pointer to the data you want to get a md5sum.
 * @param [out] (hash)  result of md5sum
 * @return      (OK/NG) If success to get a md5sum, return OK. Otherwise, return NG.
 */
static int get_md5(const unsigned char const *src, char *hash) {
  int ret                             = OK;
  MD5_CTX c                           = { 0 };
  unsigned char md[MD5_DIGEST_LENGTH] = { 0 };

  // NOTE: MD5_Init, MD5_Update, MD5_Final return 1 for success, 0 otherwise.
  if (MD5_Init(&c) != 1) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Failed to initialize in get_md5 function.\n");
  }
  if (MD5_Update(&c, src, strlen((char*) src)) != 1) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Failed to update a data in get_md5 function.\n");
  }
  if (MD5_Final(md, &c) != 1) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "Failed to acquire a hash in get_md5 function.\n");
  }

  for(int i = 0; i < MD5_DIGEST_LENGTH ; i++) {
    sprintf(&hash[i * 2], "%02x", (unsigned int)md[i]);
  }
  return ret;
}

/**
 * OBSOLETE: Use get_md5sum function instead.
 * Get result of md5sum.
 * @param [in]  (meta_data) meta data
 * @param [out] (hash)      result of md5sum
 * @return      (OK/NG)     If success to get object size, return OK. Otherwise, return NG.
 */
//static int get_hash(const char* const meta_data, char* hash) {
//  int ret = OK;
//  FILE* out = NULL;
//  char* command_buff = (char*)clf_allocate_memory(META_MAX_SIZE + 1, "command_buff");
//  char result_buff[32 + 1]  = { '\0' };
//
//  sprintf(command_buff, "echo -n \"%s\" | md5sum >%s", meta_data, MD5SUM_CMD_LOG_PATH);
//  if (strlen(command_buff) > META_MAX_SIZE) {
//	  ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_COMMON_INFO, "Meta size(%d) is too big. It must not be bigger than %d.\n", strlen(meta_data), META_MAX_SIZE - strlen(MD5SUM_CMD_LOG_PATH) - strlen("echo -n \"\" | md5sum >"));
//  }
//  system(command_buff);
//  free(command_buff);
//  command_buff = NULL;
//  out = fopen(MD5SUM_CMD_LOG_PATH,"r");
//  if (out == NULL) {
//    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_COMMON_INFO, "Failed to run \"md5sum\" commmand.\n");
//  }
//  fgets(result_buff, sizeof(result_buff), out);
//  sprintf(hash, result_buff);
//  (void) fclose(out);
//  out = NULL;
//  return ret;
//}

/**
 * Get object size from metadata.
 * @param [in]  (meta_data)   meta data
 * @param [out] (object_size) object size
 * @param [out] (object_key)  object key
 * @param [out] (object_id)   object id
 * @return      (OK/NG)       If success to get object size, return OK. Otherwise, return NG.
 */
int get_element_from_metadata(const char* const meta_data, uint64_t* object_size, char* object_key, char* object_id,
                              char* last_modified, char* version_id, char* content_md5) {
  int ret = OK;
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_element_from_metadata\n");

  json_object* jobj_from_meta_data = NULL;
  jobj_from_meta_data = json_tokener_parse(meta_data);
  json_object_object_foreach(jobj_from_meta_data, key, val) {
    if (!strcmp(key, "Size") && object_size != NULL) {
      *object_size = json_object_get_int64(val);
    } else if (!strcmp(key, "Key") && object_key != NULL) {
      strcpy(object_key, json_object_get_string(val));
    } else if (!strcmp(key, "VendorFujifilmMetadata")) {
// comment out for tapes other than those made by Fujifilm.
//      json_object_object_foreach(val, vendor_fujifilm_key, vendor_fujifilm_val) {
//        if (!strcmp(vendor_fujifilm_key, "x-coldstorage-uuid") && object_id != NULL) {
//          strcpy(object_id, json_object_get_string(vendor_fujifilm_val));
//        } else if (!strcmp(vendor_fujifilm_key, "content-md5") && content_md5 != NULL) {
//          strcpy(content_md5, json_object_get_string(vendor_fujifilm_val));
//        }
//      }
    } else if (!strcmp(key, "LastModifiedTime") && last_modified != NULL) {
      strcpy(last_modified, json_object_get_string(val));
    } else if (!strcmp(key, "Version") && version_id != NULL) {
      strcpy(version_id, json_object_get_string(val));
    }
  }

  ret |= get_md5((unsigned char*) meta_data, object_id);

  json_object_put(jobj_from_meta_data);
  jobj_from_meta_data = NULL;

  return ret;
}

/**
 * Read the file, store it in memory and return it.
 * Be sure to release the returned buffer.
 * @param [in]  (name)      Pointer of a packed object's folder path.
 * @param [out] (file_size) Pointer of a packed object's size.
 * @return      (buf)       Pointer of a packed object.On error, NULL is returned.
 */
unsigned char* read_file(const char* const name, off_t* const file_size) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:read_file(%s)\n", name);
  if ((name == NULL) || (file_size == NULL)) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT,
                       "Invalid arguments at read_file: name = 0x%X, file_size = 0x%X\n", name, file_size);
    return NULL;
  }

  FILE* fp = fopen(name, "rb");
  if (fp == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Can't open file: %s\n", name);
    return NULL;
  }

  // Get file size.
  struct stat s_str = { 0 };
  stat(name, &s_str);
  const off_t size = s_str.st_size;
  *file_size = size;

  // Store the file in memory.
  unsigned char* const buf = (unsigned char*)clf_allocate_memory(size, "file");
  fread((void*)buf, size, 1, fp);
  fclose(fp);
  fp = NULL;

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :read_file(%s)\n", name);
  return buf;
}

/**
 * Check version of the UUID is 4 or not.
 * @param [in]  (uuid)     Target UUID to check.
 * @param [in]  (owner)    Owner of the UUID. Just for log message.
 * @param [in]  (location) Location of the UUID. Just for log message.
 * @return      (OK/NG)    If version of the UUID is 4, return OK. Otherwise, return NG.
 */
static int check_uuid_version(const char* const uuid, const char* const owner, const char* const location) {
  int ret = OK;

  static const int version_offset    = 14;
  const char version                 = uuid[version_offset];
  static const char expected_version = '4';
  if (version != expected_version) {
    ret |= output_accdg_to_vl(strstr(location, LOCATION_MAM) == location ? OUTPUT_WARNING : OUTPUT_ERROR, DEFAULT,
                              "%s UUID (%s) version %c in %s should be %c.\n",
                              owner, uuid, version, location, expected_version);
  }

  return ret;
}

/**
 * Check variant of the UUID is Variant-1 or not.
 * @param [in]  (uuid)     Target UUID to check.
 * @param [in]  (owner)    Owner of the UUID. Just for log message.
 * @param [in]  (location) Location of the UUID. Just for log message.
 * @return      (OK/NG)    If variant of the UUID is Variant-1, return OK. Otherwise, return NG.
 */
static int check_uuid_variant(const char* const uuid, const char* const owner, const char* const location) {
  int ret = OK;

  static const int variant_offset            = 19;
  const char variant                         = uuid[variant_offset];
  static const char* const expected_variants = "89ABab"; // i.e. 0b10xx
  if (!strchr(expected_variants, variant)) {
    ret |= output_accdg_to_vl(strstr(location, LOCATION_MAM) == location ? OUTPUT_WARNING : OUTPUT_ERROR, DEFAULT,
                              "%s UUID (%s) value %c which include variants in %s should be one of \"%s\".\n",
                              owner, uuid, variant, location, expected_variants);
  }

  return ret;
}

/**
 * Check UUID has invalid character or not.
 * @param [in]  (uuid)     Target UUID to check.
 * @param [in]  (owner)    Owner of the UUID. Just for log message.
 * @param [in]  (location) Location of the UUID. Just for log message.
 * @return      (OK/NG)    If there is no invalid character in the UUID, return OK. Otherwise, return NG.
 */
static int check_uuid_character(const char* const uuid, const char* const owner, const char* const location) {
  int ret = OK;

  for (int uuid_offset = 0; uuid_offset < UUID_SIZE; uuid_offset++) {
    const char uuid_ascii = *(uuid + uuid_offset);
    if (!isxdigit(uuid_ascii)){
      static const int separator_offsets[]  = { 8, 13, 18, 23 };
      static const int max_separator_offset = sizeof(separator_offsets) / sizeof(separator_offsets[0]);
      int separator_offset                  = 0;
      for (separator_offset = 0; separator_offset < max_separator_offset; separator_offset++) {
        if (uuid_offset == separator_offsets[separator_offset]) {
          break;
        }
      }

      if (separator_offset == max_separator_offset // This means the uuid_ascii is not a separator.
       || uuid_ascii != '-') {                     // This means the separator is not '-' i.e. 0x2D.
        ret |= output_accdg_to_vl(strstr(location, LOCATION_MAM) == location ? OUTPUT_WARNING : OUTPUT_ERROR, DEFAULT,
                                  "%s UUID (%s) format in %s is not correct.\n"
                                  "%sYou can not use character '%c'.\n", owner, uuid, location, INDENT, uuid[uuid_offset]);
      }
    }
  }

  return ret;
}

/**
 * Check UUID(version 4) format.
 * @param [in]  (uuid)     Target UUID to check.
 * @param [in]  (owner)    Owner of the UUID. Just for log message.
 * @param [in]  (location) Location of the UUID. Just for log message.
 * @return      (OK/NG)    If the UUID format is correct, return OK. Otherwise, return NG.
 */
int check_uuid_format(const char* const uuid, const char* const owner, const char* const location) {
  int ret = OK;

  ret |= check_uuid_version(uuid, owner, location);
  ret |= check_uuid_variant(uuid, owner, location);
  ret |= check_uuid_character(uuid, owner, location);

  return ret;
}

/**
 * Check Optional UUID format i.e. accept 0 filled UUID.
 * @param [in]  (uuid)     Target UUID to check.
 * @param [in]  (owner)    Owner of the UUID. Just for log message.
 * @param [in]  (location) Location of the UUID. Just for log message.
 * @return      (OK/NG)    If the UUID format is correct, return OK. Otherwise, return NG.
 */
int check_optional_uuid_format(const char* const uuid, const char* const owner, const char* const location) {
  int ret = OK;

#ifndef FORMAT_031
  if (is_null_filled(uuid, sizeof(uuid))) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "The format of Pool Group ID is not correct.It can not be zero filled.\n");
  }
#endif
  if (strcmp(uuid, ZERO_FILLED_UUID)) {
    ret |= check_uuid_format(uuid, owner, location);
  }
#ifndef FORMAT_031
  else {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DEFAULT, "The format of Pool Group ID is not correct.It can not be zero filled.\n");
  }
#endif
  return ret;
}

#ifdef OBSOLETE
/**
 * Write objects to files.
 * @param [in]  (unpackedobjpath)  Save path of unpacked object.
 * @param [in]  (data_buf)         Pointer of a packed object.
 * @param [in]  (current_position) Current position of a pointer.
 * @param [in]  (num_of_obj)       Number of objects on the tape.
 * @param [out] (objects)          Pointer of a LTOS object.
 * @return      (OK/NG)            Whether the file output was correct or not.
 */
int cpof_write_objects_to_files(const char* const unpackedobjpath, const unsigned char* const data_buf,
                                const uint64_t* const current_position, const int* const num_of_obj,
                                const LTOSObject* const objects) {
  for (int objnum = 0; objnum < *num_of_obj; objnum++) {
    struct stat statBuf = { 0 };
    if (stat(unpackedobjpath, &statBuf) != 0) {
      mkdir(unpackedobjpath,0777);
    }

    /* object data */
    const unsigned int object_size = (objects + (objnum + 1))->meta_data_offset - (objects + objnum)->object_data_offset;
    if (cpof_write_an_object_to_a_file(EMPTY, unpackedobjpath, objnum,
      data_buf + (*current_position) + ((objects + objnum)->object_data_offset), object_size, objects) == NG) {
      output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Can't write %d th object to file.\n", objnum);
      return NG;
    }

    /* meta data */
    const unsigned int meta_size = (objects + objnum)->object_data_offset - (objects + objnum)->meta_data_offset;
    if (cpof_write_an_object_to_a_file(JSON_EXT, unpackedobjpath, objnum,
      data_buf + *current_position + (objects + objnum)->meta_data_offset, meta_size, objects) == NG) {
      output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Can't write %d th metadata to file.\n", objnum);
      return NG;
    }
  }
  return OK;
}
#endif // OBSOLETE

#ifdef OBSOLETE
/**
 * Write an object to a file.
 * @param [in]  (obj_ext)         "" or ".json"
 * @param [in]  (unpackedobjpath) Save path of unpacked object.
 * @param [in]  (objnum)          Object number.
 * @param [in]  (object_buf)      Pointer of a packed object.
 * @param [in]  (object_size)     Size of an object on the tape.
 * @param [in]  (objects)         Pointer of a LTOS object.
 * @return      (OK/NG)           Whether the file output was correct or not.
 */
int cpof_write_an_object_to_a_file(const char* const obj_ext, const char* const unpackedobjpath, const int objnum,
                                          const unsigned char* const object_buf, const unsigned int object_size, const LTOSObject* const objects) {
  char object_name[UNPACKED_OBJ_PATH_SIZE + 1] = { '\0' };
  sprintf(object_name,"%s%s%s",unpackedobjpath, ((objects + objnum)->object_id), obj_ext);

  FILE* fp_object = fopen(object_name, "wb");;
  if (fp_object == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Can't open file: %s\n", object_name);
    return NG;
  }
  fwrite(object_buf, object_size, 1, fp_object);

  fclose(fp_object);
  fp_object = NULL;
  return OK;
}
#endif // OBSOLETE

/**
 * Check UTC format.
 * @param [in]  (utc)   UTC like "0123-45-67T89:01:23.456789Z"
 * @return      (OK/NG) If the format is correct or not.
 */
int check_utc_format(const char* const utc) {
  int ret           = OK;
  const int utc_len = strlen(utc);

  if (utc_len != UTC_LENGTH) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO,
                                "Length of Time format is not correct.\n"
                                "%sActual value  : %d\n"
                                "%sExpected value: %d\n", INDENT, utc_len, INDENT, UTC_LENGTH);
    return ret;
  }

  for (int utc_offset = 0; utc_offset < utc_len; utc_offset++) {
    const char utc_ascii = *(utc + utc_offset);
    if (utc_offset == UTC_T) {
      if (utc_ascii != 'T') {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "\"T\" does not exist at Byte %d.\n", UTC_T);
      }
    } else if (utc_offset == UTC_COLON1 || utc_offset == UTC_COLON2) {
      if (utc_ascii != ':') {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "\":\" does not exist at Byte %d or %d.\n", UTC_COLON1, UTC_COLON2);
      }
    } else if (utc_offset == UTC_DOT) {
      if (utc_ascii != '.') {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "\".\" does not exist at Byte %d.\n", UTC_DOT);
      }
    } else if (utc_offset == UTC_Z) {
      if (utc_ascii != 'Z') {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "\"Z\" does not exist at Byte %d.\n", UTC_Z);
      }
    } else {
      if (utc_offset == UTC_HYPHEN1 || utc_offset == UTC_HYPHEN2) {
        if (utc_ascii != '-') {
          ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "\"-\" does not exist at Byte %d or %d.\n", UTC_HYPHEN1, UTC_HYPHEN2);
         }
      } else if (!isdigit(utc_ascii)) {
        ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_HEADER_INFO, "No digit [%s] is found at Byte %d.\n", utc_ascii, utc_offset);
      }
    }
    if (ret == NG) {
      break;
    }
  }
  return ret;
}

/**
 * Return Partition Name.
 * @param [in]  (part) Partition number.
 * @return      Return partition name i.e. "reference" or "data"
 */
char* clf_get_partition_name(const int part) {
  return part == REFERENCE_PARTITION ? "Reference" : (part == DATA_PARTITION ? "Data" : "Unknown");
}

/**
 * Allocate memory
 * @param [in]  (size)   Size to allocate memory.
 * @param [in]  (object) Object name to allocate memory. Just for error log.
 * @return      Return a pointer to the allocated memory.
 */
void* clf_allocate_memory(const size_t size, const char* const object) {
  void* const p = calloc(size, 1);
  if (p == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "Failed to allocate %d bytes for %s.\n", size, object);
  }
  return p;
}

/**
 * Check read bytes.
 * @param [in]  (actual)   Actual read bytes.
 * @param [in]  (expected) Expected read bytes.
 * @param [in]  (object)   Read object name. Just for error log.
 * @param [in]  (filename) Read file name. Just for error log.
 * return       Return OK when actual read size is equal to expected read size. Otherwise, exit() is called via output_accdg_to_vl().
 */
int clf_check_read_data(const size_t actual, const size_t expected,
                        const char* const object, const char* const filename) {
  int ret = OK;
  if (actual != expected) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                              "Failed to read %s from file %s.\n", object, filename);
  }
  return ret;
}

/**
 * Do NOT close the file handler for performance enhancement. File handler will be closed in open_file.
 * @param [in]  (stream) File pointer to close.
 * @return      Return 0.
 */
int clf_close_file(FILE* const stream) {
  if (stream != NULL) {
//For performance enhancement, fclose() is called in clf_open_file() function instead of here.
//    fclose(stream);
//    stream = NULL;
  }
  return 0;
}

/**
 * Wrapper function of fopen().
 * @param [in]  (filename) File path.
 * @param [in]  (mode)     mode to open file.
 * @param [in]  (index)    1 for read_marker_file. 0 for others.
 * @return      Return file pointer.
 */
static FILE* open_file(const char* filename, const char* mode, const int index) {
  static FILE* fp[2] = { NULL };
  static char prev_filename[2][4096] = { { '\0' } };

  // Check arguments.
  if (filename == NULL || mode == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "Invalid argument at open_file. filename %d and/or mode %d is NULL.\n", filename, mode);
  }
  if (index < 0 || 2 <= index) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "Invalid argument at open_file. index %d should be 0 or 1.\n", index);
  }

  if (strcmp(filename, prev_filename[index])) {
    // Close file pointer to avoid resource leak.
    if (fp[index]) {
      const int ret = fclose(fp[index]);
      fp[index]     = NULL;
      if (ret == EOF) {
        output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                           "Failed to close file. error=%s\n", strerror(errno));
      }
    }

    strcpy(prev_filename[index], filename);
    fp[index] = fopen(filename, mode);
    if (fp[index] == NULL) {
      output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                         "Failed to open file. path=%s, mode=%s, error=%s\n",
                         filename, mode, strerror(errno));
    }
  }

  return fp[index];
}

/**
 * Wrapper function of fopen() for functions w/o read_marker_file.
 * @param [in]  (filename) File path.
 * @param [in]  (mode)     mode to open file.
 * @return      Return file pointer.
 */
FILE* clf_open_file(const char* filename, const char* mode) {
  return open_file(filename, mode, 0);
}

/**
 * Wrapper function of fopen() only for read_marker_file. This is required because tow files are open at the same time.
 * @param [in]  (filename) File path.
 * @param [in]  (mode)     mode to open file.
 * @return      Return file pointer.
 */
FILE* clf_open_alt_file(const char* filename, const char* mode) {
  return open_file(filename, mode, 1);
}

/**
 * Wrapper function of fread().
 * @param [in]  (ptr)    Pointer of buffer to store read data.
 * @param [in]  (size)   Object size.
 * @param [in]  (nobj)   Number of maximum objects to read.
 * @param [in]  (stream) File pointer to read.
 * @return      Return number of read objects.
 */
size_t clf_read_file(void* const ptr, const size_t size, const size_t nobj, FILE* const stream) {
  const size_t ret = fread(ptr, size, nobj, stream);
  if (feof(stream)) {
    clearerr(stream);
  } else {
    const int error = ferror(stream);
    if (error) {
      output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                         "Failed to read file. error=%s\n", strerror(error));
    }
  }
  return ret;
}

/**
 * Write object and meta.
 * @param [in]  (data)        Binary data.
 * @param [in]  (object_size) Size of an object on the tape.
 * @param [in]  (str_offset)  From where to start reading the binary data.
 * @param [in]  (filepath)    File path of file.
 * @return      (OK/NG)       Whether the file output was correct or not.
 */
int write_object_and_meta_to_file(const char* data, const uint64_t object_size, const uint64_t str_offset, const char* filepath) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:write_object_and_meta_to_file(%s)\n",filepath);

  struct stat stat_buf = { 0 };
  char* dirpath  = (char*)clf_allocate_memory(strlen(filepath), "dirpath");
  extract_dir_path(filepath, dirpath);
  if(stat(dirpath, &stat_buf) != OK) {
    if(mkdir(dirpath, stat_buf.st_mode) != OK) {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to make directory.\n");
    }
  }
  free(dirpath);
  dirpath = NULL;

  FILE* fp_object = fopen(filepath, "ab");
  if (fp_object == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DEFAULT, "Can't open file: %s\n", filepath);
  }
  if (0 < object_size) {
    if (fwrite(data + str_offset, object_size, 1, fp_object) < 1) {
	    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to output %s.Disk space is likely to insufficient.\n", filepath);
    }
  }

  fclose(fp_object);
  fp_object = NULL;
  return ret;
}

int get_tape_generation(void* scparam, char tape_gen[2]) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_tape_generation\n");
  uint8_t volume_personality[9];
  uint8_t dxferp[0x8000] = {0};
  uint32_t resid = 0;
  ST_SPTI_REQUEST_SENSE_RESPONSE sb = {0};
  ST_SYSTEM_ERRORINFO syserr = {0};

  const BOOL rc = spti_log_sense(scparam, 0x17, 0x45, sizeof(dxferp),
                                  &dxferp, &resid, &sb, &syserr);
  if (rc == FALSE) {
    ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "spti_log_sense error.\n");
  }
  memmove(volume_personality, dxferp + 8, 9);
  if (strncmp((char *)volume_personality, "Ultrium-", 8) == 0) {
    tape_gen[0] = 'L';
    strncpy(tape_gen + 1, (char *)volume_personality + 8, 1);
  } else if (strncmp((char *)volume_personality , "LTO", 3) == 0) {
    strncpy(tape_gen, (char *)volume_personality + 3, 2);
  } else {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Volume personality is not correct.(%s)\n", volume_personality);
  }

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :get_tape_generation\n");
  return ret;
}

/**
 * Compare which is the latest time.
 * @param [in] (first_time_string)  first_time_string
 * @param [in] (second_time_string) second_time_string
 * @return     (diff_time)          Difference between first_time_string and second_time_string.
 *                                  The return value is negative if the second_time_string is the latest.
 */
double compare_time_string(const char *first_time_string, const char *second_time_string) {
  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start  :compare_time_string\n");

  double    diff_time   = 0.0;
  struct tm first_time  = {0};
  struct tm second_time = {0};
  strptime(first_time_string, "%Y-%m-%dT%H:%M:%S.", &first_time);
  strptime(second_time_string, "%Y-%m-%dT%H:%M:%S.", &second_time);
  time_t first_time_t = mktime(&first_time);
  time_t second_time_t = mktime(&second_time);
  diff_time = difftime(first_time_t, second_time_t);

  output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :compare_time_string\n");
  return diff_time;
}

/**
 * Get bucket name.
 * @param [in]  (bucket_list) bucket_list
 * @param [in]  (bucket_id)   bucket_id
 * @param [out] (bucket_name) bucket_name
 * @return      (OK/NG)       If succeeded or not.
 */
int get_bucket_name(const char *bucket_list, const char *bucket_id, char **bucket_name) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_bucket_name\n");

  char *internal_bucket_list                  = (char*)clf_allocate_memory(strlen(bucket_list) + 1, "internal_bucket_list");
  char  bucket_id_reformed[UUID_SIZE + 1 + 2] = { '\0' };
  uint32_t len_bucket_list                    = strlen(bucket_list);

  strcpy(internal_bucket_list, bucket_list);
  sprintf(bucket_id_reformed, "\"%s\"", bucket_id);
  int         multi_loop_flag        = 0;
  while (strlen(bucket_id_reformed) < len_bucket_list) {
    char        *kakko                 = strstr(internal_bucket_list + 1, "}");
    char        *bucket_name_temp      = (char*)clf_allocate_memory(strlen(internal_bucket_list), "bucket_name_temp");
    int          ptr_from              = 1;
    int          ptr_to                = 1;
    ptr_to = kakko - internal_bucket_list;
    const char *bucket_info_set  = str_substring(internal_bucket_list, ptr_from + multi_loop_flag, ptr_to - multi_loop_flag);
    const char *bucket_list_temp = str_substring(internal_bucket_list, ptr_to, len_bucket_list);
    char       *bucket_id_temp   = (char*)clf_allocate_memory(strlen(bucket_info_set) + 1, "bucket_id_temp");
    strcpy(internal_bucket_list, bucket_list_temp);
// bucket_list = str_substring(bucket_list, ptr_to, len_bucket_list);
    len_bucket_list = strlen(internal_bucket_list);

    extract_json_element(bucket_info_set, "BucketID", &bucket_id_temp);
    if (strcmp(bucket_id_temp, bucket_id_reformed) == 0) {
      extract_json_element(bucket_info_set, "BucketName", &bucket_name_temp);
      char * bucket_name_substr_temp = str_substring(bucket_name_temp, 1, strlen(bucket_name_temp) -2);
      strcpy(*bucket_name, bucket_name_substr_temp);
      free((char*)bucket_list_temp);
      bucket_list_temp = NULL;
      free((char*)bucket_info_set);
      bucket_info_set = NULL;
      free_safely(&bucket_name_substr_temp);
      free_safely(&bucket_name_temp);
      free_safely(&bucket_id_temp);
      break;
    }
    if (strlen(bucket_id_reformed) < len_bucket_list) {
      free((char*)bucket_list_temp);
      bucket_list_temp = NULL;
      free((char*)bucket_info_set);
      bucket_info_set = NULL;
      free_safely(&bucket_name_temp);
      free_safely(&bucket_id_temp);
      //ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to get bucket name.\n");
    }
    free((char*)bucket_list_temp);
    bucket_list_temp = NULL;
    free((char*)bucket_info_set);
    bucket_info_set = NULL;
    free_safely(&bucket_name_temp);
    free_safely(&bucket_id_temp);
    multi_loop_flag = 1;
  }

  free_safely(&internal_bucket_list);
  ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end:get_bucket_name\n");
  return ret;
}

/**
 * Free allocated memory safely.
 * @param [in] (str) allocated memory.
 */
void free_safely(char ** str) {
  //output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:free_safely\n");

    free(*str);
    *str = NULL;

  //output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :free_safely\n");
}

/**
 * Extract json element.
 * @param [in]  (json_data)    json_data.
 * @param [in]  (json_key)     json_key.
 * @param [out] (json_element) json_element.
 * @return      (OK/NG)        If succeeded or not.
 */
int extract_json_element(const char *json_data, const char *json_key, char **json_element) {
  int ret = OK; // = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:extract_json_element\n");

  //json_data = {"BucketList":[{"BucketID":"ff619b8c-1511-464d-b278-d402a9f075b8","BucketName":"20200325"}]}
  //json_key = BucketList


  const char  *tmp_json_element      = NULL;
  json_object *json_data_from_string = NULL;

  json_data_from_string = json_tokener_parse(json_data);
  if (json_data_from_string != NULL) {
    json_object_object_foreach(json_data_from_string, key, val) {
      if (strcmp(key, json_key) == 0) {
        tmp_json_element = json_object_to_json_string_ext(val, JSON_C_TO_STRING_PLAIN);
      }
    }
  }
  if (tmp_json_element != NULL) {
    strncpy(*json_element, tmp_json_element, strlen(tmp_json_element));
  }
  if (json_data_from_string != NULL) json_object_put(json_data_from_string), json_data_from_string = NULL;
  //ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end:extract_json_element\n");
  return ret;
}
