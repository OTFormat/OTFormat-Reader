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
 * @file object_reader_util.c
 *
 * Common functions to read a data from a tape formatted in OTFormat.
 */

#include "ltos_format_checker.h"

static int is_force_flag = false;


#ifdef OBSOLETE
/**
 * Add L0/Object to the current L1/Packed Object.
 * @param [in]  (current)          Pointer to the current Object.
 * @param [in]  (next)             Pointer to an Object you want to link to the current Object.
 * @return      (OK)               return OK if process is complete w/o any errors.
 */
int add_L0_obj(L0* const current, L0* const next) {
  if (current == NULL || next == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at add_L0_obj");
  }
  current->next_obj = next;
  next->current_po = current->current_po;
  return OK;
}

/**
 * Add L1/Packed Object(PO) to the current L2/Object Commit Marker.
 * @param [in]  (current)          Pointer to the current PO.
 * @param [in]  (next)             Pointer to a PO you want to link to the current PO.
 * @return      (OK)               return OK if process is complete w/o any errors.
 */
int add_L1_po(L1* const current, L1* const next) {
  if (current == NULL || next == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at add_L1_po");
  }
  current->next_po= next;
  next->current_ocm = current->current_ocm;
  return OK;
}

/**
 * Add L2/Object Commit Marker(OCM) to the current L3/Partial Reference.
 * @param [in]  (current)          Pointer to the current OCM.
 * @param [in]  (next)             Pointer to a OCM you want to link to the current OCM.
 * @return      (OK)               return OK if process is complete w/o any errors.
 */
int add_L2_ocm(L2* const current, L2* const next) {
  if (current == NULL || next == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at add_L2_ocm");
  }
  current->next_ocm= next;
  next->current_pr = current->current_pr;
  return OK;
}

/**
 * Add L3/Partial Reference(PR) to the current L4/Full Tape.
 * @param [in]  (current)          Pointer to the current PR.
 * @param [in]  (next)             Pointer to a PR you want to link to the current PR.
 * @return      (OK)               return OK if process is complete w/o any errors.
 */
int add_L3_pr(L3* const current, L3* const next) {
  if (current == NULL || next == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at add_L3_pr");
  }
  current->next_pr= next;
  next->current_tape = current->current_tape;
  return OK;
}

/**
 * Search an Object which has the target KEY from the current Object.
 * @param [in]  (obj)              Pointer to an Object (It's OK not to select the beginning of the PO).
 * @param [in]  (key)              Target KEY.
 * @param [out] (seq_id)           Sequential ID of Object, which has the target KEY, in the PO.
 * @return      (OK)               return OK if Object is found in the current PO.
 */
static int search_object_by_key(const L0* const obj, const char* const key, uint64_t* const seq_id) {

  if (obj == NULL || key == NULL || seq_id == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at search_object");
  }

  //Check if this object has the target KEY.
  if (strlen(obj->key) > 0) {
    if (strncmp(key, obj->key, MIN(strlen(key), strlen(obj->key))) == 0) {
      *seq_id = (obj - obj->current_po->first_obj) + 1;
      return OK;
    }
  }

  // Move on to the next object if it is not terminated.
  if (obj->next_obj == NULL) {
    return NG;
  }
  return search_object_by_key(obj->next_obj, key, seq_id);
}

/**
 * Search an Object which has the target KEY in the specified Packed Object(PO).
 * @param [in]  (po)               Pointer to the specified Packed Object.
 * @param [in]  (key)              the target KEY.
 * @return      (seq_id)           Sequential ID of Object which has the target KEY (Note: 0 means not found)
 */
uint64_t identify_object_by_key(const L1* const po, const char* key) {
  uint64_t seq_id = 0;

  if (po == NULL || key == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at identify_object_seq_id_in_packed_object");
  }

  if (strlen(key) == 0) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
        "Please input a valid Object KEY\n"
        "%sActual: \"%s\"\n", INDENT, key);
  }

  search_object_by_key(po->first_obj, key, &seq_id);

  return seq_id;
}

/**
 * Search an Object which has the target UUID from the current Object.
 * @param [in]  (obj)              Pointer to an Object (It's OK not to select the beginning of the PO).
 * @param [in]  (uuid)             Target UUID of Object.
 * @param [out] (seq_id)           Sequential ID of Object, which has the target KEY, in the PO.
 * @return      (OK)               return OK if Object is found in the current PO.
 */
int search_object_by_uuid(const L0* const obj, const char* const uuid, uint64_t* const seq_id) {

  if (obj == NULL || uuid == NULL || seq_id == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at search_object");
  }

  //Check if this object has the target UUID.
  if (obj->id != NULL) {
    if (memcmp(uuid, obj->id, UUID_BIN_SIZE) == 0) {
      *seq_id = (obj - obj->current_po->first_obj) + 1;
      return OK;
    }
  }

  // Move on to the next object if it is not terminated.
  if (obj->next_obj == NULL) {
    return NG;
  }
  return search_object_by_uuid(obj->next_obj, uuid, seq_id);
}

/**
 * Search an Object which has the target UUID in the specified Packed Object(PO).
 * @param [in]  (po)               Pointer to the specified Packed Object.
 * @param [in]  (uuid)             Target UUID of Object (Binary data)
 * @return      (seq_id)           Sequential ID of Object which has the target KEY (Note: 0 means not found)
 */
uint64_t identify_object_by_uuid(const L1* const po, const char* uuid) {
  uint64_t seq_id = 0;

  if (po == NULL || uuid == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Null pointer is detected at identify_object_seq_id_in_packed_object");
  }

  if (strlen(uuid) == 0) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
        "Please input a valid Object UUID\n"
        "%sActual: \"%s\"\n", INDENT, uuid);
  }

  search_object_by_uuid(po->first_obj, uuid, &seq_id);

  return seq_id;
}
#endif // OBSOLETE

/**
 * Check if the file is able to be opened.
 * @param [in]  (filename) File path.
 * @return      (OK) return OK if file is opened w/o errors.
 */
int check_file(const char* const filename) {
  static FILE* fp = NULL;
  int ret = NG;

  // Check arguments.
  if (filename == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "Invalid argument at check_file. filename %d is NULL.\n", filename);
  }

  fp = fopen(filename, "r");
  if (fp != NULL) {
    fclose(fp);
    fp = NULL;
    ret = OK;
  } else {
    output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO,
                      "Failed to open file(%s). %s\n", filename, strerror(errno));
  }
  return ret;
}


/**
 * get information, which has the object key, in the list file.
 * @param [in]  (object_key) Object key user wants to get.
 * @param [in]  (object_id)  Object id user wants to get.
 * @param [in]  (list_path)  File path.
 * @param [out] (objects)    Structure of each object which has the Object key.
 * @return      (OK)         return OK if at least an object is found.
 */
int get_object_info_in_list(const char* const object_key, const char* const object_id, const char* const list_path, object_list** objects) {
  int ret                      = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:get_object_info_in_list\n");

  // Check arguments.
  if (object_key == NULL || object_id == NULL || list_path == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "Invalid argument at get_object_key_in_list. object_key %s or object_id %s or list_path %s is NULL.\n", object_key, object_id, list_path);
  }

  FILE *fp = fopen(list_path, "r") ;
  if (fp == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "Failed to open file. path=%s, error=%s\n", list_path, strerror(errno));
  }

  char readline[STR_MAX]           = {'\0'};
  char json_object_key[STR_MAX]    = {'\0'};
  char json_last_modified[STR_MAX] = {'\0'};
  char json_version_id[STR_MAX]    = {'\0'};
  char json_content_md5[STR_MAX]   = {'\0'};
  char json_object_id[STR_MAX]     = {'\0'};
  char json_size[STR_MAX]          = {'\0'};
  char json_block_address[STR_MAX] = {'\0'};
  char json_offset[STR_MAX]        = {'\0'};
  char json_meta_size[STR_MAX]     = {'\0'};
  int obj_list_flag                = 0;
  int end_comma_flag               = 0;
  int add_object_list_key_flag     = 0;
  int add_object_list_oid_flag     = 0;
  struct object_list* current      = NULL;
  struct object_list* latest       = (object_list *)clf_allocate_memory(sizeof(struct object_list), "latest");
  while ( fgets(readline, STR_MAX, fp) != NULL ) {
    if (obj_list_flag == 1) {
      if (strncmp("]", readline, strlen("]")) == 0) { // Read all objects' info in ObjectList.
    	 if (*objects == NULL) { // In case the specified object key was not found in the list.
    		 free(latest);
    		 latest = NULL;
    		 break;
    	 }
        if (strcmp(object_id, "latest") == 0) {
          current = *objects;
          *latest  = *(*objects);
          while(current->next != NULL) {
            if (compare_time_string(latest->last_mod_date, current->next->last_mod_date) < 0){
              *latest = *(current->next);
            }
            current = current->next;
          }
          (*objects) = latest;
          (*objects)->next = NULL;

        }
        break;
      }
      end_comma_flag = 0;
      if (readline[strlen(readline) - 2 ] == COMMA_ASCII) {
        end_comma_flag = 1;
      }
      if (strncmp("\"object_key\":", readline, strlen("\"object_key\":")) == 0) {
    	 char *object_key_in_list  = str_substring(readline, strlen("\"object_key\":") + 1 , strlen(readline) - strlen("\"object_key\":") - 3 - end_comma_flag);
        sprintf(json_object_key, "%s", object_key_in_list);
        free(object_key_in_list);
        object_key_in_list = NULL;
        if (strcmp(json_object_key, object_key) == 0) {
          add_object_list_key_flag = 1;
        }
      } else if (strncmp("\"size\":", readline, strlen("\"size\":")) == 0) {
    	 char *size = str_substring(readline, strlen("\"size\":") , strlen(readline) - strlen("\"size\":") - 1 - end_comma_flag);
        sprintf(json_size, "%s", size);
        free(size);
        size = NULL;
      } else if (strncmp("\"last_modified\":", readline, strlen("\"last_modified\":")) == 0) {
    	 char *last_modified = str_substring(readline, strlen("\"last_modified\":") + 1 , strlen(readline) - strlen("\"last_modified\":") - 3 - end_comma_flag);
        sprintf(json_last_modified, "%s", last_modified);
        free(last_modified);
        last_modified = NULL;
      } else if (strncmp("\"version_id\":", readline, strlen("\"version_id\":")) == 0) {
    	 char *version_id = str_substring(readline, strlen("\"version_id\":") + 1 , strlen(readline) - strlen("\"version_id\":") - 3 - end_comma_flag);
        sprintf(json_version_id, "%s", version_id);
        free(version_id);
        version_id = NULL;
      } else if (strncmp("\"content_md5\":", readline, strlen("\"content_md5\":")) == 0) {
    	 char *content_md5 = str_substring(readline, strlen("\"content_md5\":") + 1 , strlen(readline) - strlen("\"content_md5\":") - 3 - end_comma_flag);
        sprintf(json_content_md5, "%s", content_md5);
        free(content_md5);
        content_md5 = NULL;
      } else if (strncmp("\"object_id\":", readline, strlen("\"object_id\":")) == 0) {
    	 char *object_id_in_list = str_substring(readline, strlen("\"object_id\":") + 1 , strlen(readline) - strlen("\"object_id\":") - 3 - end_comma_flag);
        sprintf(json_object_id, "%s", object_id_in_list);
        free(object_id_in_list);
        object_id_in_list = NULL;
        if (strcmp(object_id, "all") == 0 || strcmp(object_id, "latest") == 0 || strcmp(object_id, json_object_id) == 0) {
          add_object_list_oid_flag = 1;
        }
      } else if (strncmp("\"block_address\":", readline, strlen("\"block_address\":")) == 0) {
    	 char *block_address = str_substring(readline, strlen("\"block_address\":") , strlen(readline) - strlen("\"block_address\":") - 1 - end_comma_flag);
        sprintf(json_block_address, "%s", block_address);
        free(block_address);
        block_address = NULL;
      } else if (strncmp("\"offset\":", readline, strlen("\"offset\":")) == 0) {
    	 char *offset = str_substring(readline, strlen("\"offset\":") , strlen(readline) - strlen("\"offset\":") - 1 - end_comma_flag);
        sprintf(json_offset, "%s", offset);
        free(offset);
        offset = NULL;
      } else if (strncmp("\"meta_size\":", readline, strlen("\"meta_size\":")) == 0) {
    	 char *meta_size = str_substring(readline, strlen("\"meta_size\":") , strlen(readline) - strlen("\"meta_size\":") - 1 - end_comma_flag);
        sprintf(json_meta_size, "%s", meta_size);
        free(meta_size);
        meta_size = NULL;
      } else if (strncmp("}", readline, strlen("}")) == 0) {
        if (add_object_list_key_flag == 1 && add_object_list_oid_flag == 1) {
          add_object_list_key_flag = 0;
          add_object_list_oid_flag = 0;
          struct object_list* add_object = (object_list *)clf_allocate_memory(sizeof(struct object_list), "add_object");
          add_object->block_address = atol(json_block_address);
          add_object->data_offset = atol(json_offset);
          strcpy((add_object->id), json_object_id);
          //add_object->is_delete_marker
          strcpy((add_object->key), json_object_key);
          strcpy((add_object->last_mod_date), json_last_modified);
          strcpy((add_object->md5), json_content_md5);
          add_object->meta_offset = atol(json_offset);
          add_object->metadata_size = atol(json_meta_size);
          //strcpy((add_object->po_id),
          add_object->size = atol(json_size);
          strcpy((add_object->verson_id), json_version_id);
          add_object->next                     = NULL;
          if (*objects == NULL) {
            *objects = add_object;
          } else {
            current = *objects;
            while(1) {
              if (current->next == NULL){
                current->next = add_object;
                break;
              }
              current = current->next;
            }
          }
          if (strcmp(object_id, "all") != 0 && strcmp(object_id, "latest") != 0) {
            break;
          }
        }
      }
    }
    if (strncmp("\"ObjectList\":[", readline, strlen("\"ObjectList\":[")) == 0) {
      obj_list_flag = 1;
    }
    memset(readline, 0, sizeof(readline));
 }

  fclose(fp);
  fp = NULL;
  return ret;
}


/**
 * Set both meta and data offset to object_list structure based on information in PO directory.
 * @param [in]  (PO_directory) pointer to the string before reading from tape.
 * @param [in]  (object) L0 parameters will be stored in this structure
 * @return      (OK) return OK if file is opened w/o errors.
 */
/* Obsolete
int get_object_info_from_directory(const char* const PO_directory, object_list* const object) {
  int ret = NG;
  uuid_t object_id_uuid;
  const uint64_t num_of_obj_in_PO    = (sizeof(PO_directory)) / PO_DIR_SIZE;
  char object_id[UUID_SIZE + 1]      = { '\0' };
  uint32_t object_directory_offset   = 0;

  // Check arguments.
  if (PO_directory == NULL || object == NULL) {
    output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO,
                       "Invalid argument at get_object_info_from_directory. PO_directory %s or object %s is NULL.\n", PO_directory, object);
  }

  // Read Objects Directory.
  for (int objnum = 1; objnum < num_of_obj_in_PO + 1; objnum++) {
    // Read Object ID.
    memmove(object_id_uuid, PO_directory + object_directory_offset, OBJECT_ID_SIZE);
    uuid_unparse(object_id_uuid, object_id);
    object_directory_offset += OBJECT_ID_SIZE;

    if (object->id == object_id) {
      // Read Meta Data Offset.
      r64(BIG, (uint8_t*)(PO_directory + object_directory_offset), &(object->meta_offset), 1);
      object_directory_offset += META_DATA_OFFSET_SIZE;
      // Read Object Data Offset.
      r64(BIG, (uint8_t*)(PO_directory + object_directory_offset), &(object->data_offset), 1);
      object_directory_offset += META_DATA_OFFSET_SIZE;
      return OK;

    } else {
      object_directory_offset += META_DATA_OFFSET_SIZE + OBJECT_DATA_OFFSET_SIZE;
    }
  }
  return ret;
}
*/


/**
 * Set force flag(-F).
 * @param [in] (is_force_enabled) Force option specified on the command line.
 */
void set_force_flag(int is_force_enabled) {
	is_force_flag = is_force_enabled;
}


/**
 * Check if the disk space (GiB) at specified path is greater than the total size of MIN_REQUIRED_DISK_SPACE_GiB and specified size.
 * @param [in]  (path)      Path you want to check.
 * @param [in]  (data_size) Size you want to write.(Byte)
 * @return      (OK/NG)     OK  :The disk space (GiB) at specified path is greater than MIN_REQUIRED_DISK_SPACE_GiB.
 *                          NG  :Some error has occurred while checking the disk space,
 *                               or the disk space (GiB) at specified path is less than MIN_REQUIRED_DISK_SPACE_GiB.
 */
int check_disk_space(const char* const path, const uint64_t data_size) {

  int ret  = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:check_disk_space\n");
  FILE* out = NULL;
  char df_command_buff[PATH_MAX + DISK_SPACE_COMMAND_SIZE + 1]  = { '\0' };
  char result[MAX_DISK_SPACE_LENGTH + 1]   = { '\0' };
  uint64_t size = 0;

  if (is_force_flag == true) {
	  return ret;
  }
  sprintf(df_command_buff, "df %s | tail -n 1 | awk '{print $4 }' >%s", path, DF_CMD_LOG_PATH);
  system(df_command_buff);
  out = fopen(DF_CMD_LOG_PATH,"r");
  if (out == NULL) {
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_COMMON_INFO, "Failed to run \"df\" commmand to get a disk space.\n", path);
  }
  while (!feof(out)) {
    fgets(result, sizeof(result), out);
  }
  (void) fclose(out);
  out = NULL;

  size = (atol(result) * 1024); // KiB to Byte conversion

  if (size == 0) { // It means that "result" is not digit or disk space = 0;
    ret |= output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_COMMON_INFO, "Disk space specified is zero.\n");
  }

  if (size < MIN_REQUIRED_DISK_SPACE_GiB * pow(1024, 3) + data_size) {
    output_accdg_to_vl(OUTPUT_ERROR, DISPLAY_ALL_INFO,
           "Disk space is not large enough: %ld (GiB).\n"
           "%sSpecify --Force option or keep >%d GiB disk space.\n", (uint64_t)(size / pow(1024, 3)), INDENT, MIN_REQUIRED_DISK_SPACE_GiB);
    //Don't specify "OUTPUT_ERROR" as an argument of "output_accdg_to_vl" because, history.log should be output even if exit option(--continue) is specified.
    ret = NG;
  }
  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "Disk space (GiB) = %ld\n", (uint64_t)(size / pow(1024, 3)));
  return ret;
}

/**
 * Complete all list files by adding "]}".
 * @param [in]  (list_dir) A directory which include list files.
 * @return      (OK) return OK if all of the files are completed.
 */
int comlete_list_files(const char* const list_dir) {
  int ret = output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "start:comlete_list_files\n");
  DIR* dp = NULL;
  char filepath[OUTPUT_PATH_SIZE + 1] = "";
  struct dirent* ent;

  if ((dp = opendir(list_dir)) == NULL) {
    if (errno == ENOENT) {
      return ret;
    } else {
      ret |= output_accdg_to_vl(OUTPUT_SYSTEM_ERROR, DISPLAY_ALL_INFO, "Failed to open directory(%s).\n", list_dir);
    }
  }
  while ((ent = readdir(dp)) != NULL) {
    int complete_flag = 0;
    if (strcmp(ent->d_name + strlen(ent->d_name) - strlen(".lst"), ".lst") == 0) {
    	complete_flag = 1;
    }
    sprintf(filepath, "%s%s", list_dir, ent->d_name);
    if (complete_flag == 1) {
      FILE* fp_list_for_complete;
      fp_list_for_complete = fopen(filepath,"ab");
      fprintf(fp_list_for_complete, "]\n}");
      fclose(fp_list_for_complete);
      fp_list_for_complete = NULL;
    }
  }
  closedir(dp);
  dp = NULL;

  ret |= output_accdg_to_vl(OUTPUT_TRACE, DISPLAY_ALL_INFO, "end  :comlete_list_files\n");
  return ret;
}
