//
// Created by david on 12.10.26.
//

#include "endpoints.h"

#include <string.h>
#include <linux/limits.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

#include "helpers.h"

static enum MHD_Result HandleStatic(struct MHD_Connection *connection, const char *url, const char *method) {
  if (strcmp(method, "GET") != 0) {return send_error(connection, 405, "Method Not Allowed");}

  char file_path[PATH_MAX];
  const char *target_url = (strcmp(url, "/") == 0) ? "/static/index.html" : url;

  int status = resolve_safe_path(target_url, "STATIC", file_path);
  if (status != 200) {
    return send_error(connection, status, status == 404 ? "Not Found" : "Forbidden");
  }

  const char *mime = (strcmp(url, "/") == 0) ? "text/html" : "auto";
  return send_file(connection, file_path, mime);
}

/**
 * endpoint for /api/programs
 *
 */
static enum MHD_Result HandlePrograms(struct MHD_Connection *connection, const char *url, const char *method){
  const char* directory = getenv("VIDEOS");
  if (!directory) {return send_error(connection,MHD_HTTP_INTERNAL_SERVER_ERROR, "Server error");}
  DIR *dir = opendir(directory);
  if (dir == NULL) {
    return send_error(connection,MHD_HTTP_INTERNAL_SERVER_ERROR, "Server error 2");
  }

  struct dirent *entry;

  cJSON* root = cJSON_CreateArray();
  // Postupné čtení jednotlivých položek složky
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
    {
      continue;
    }
    cJSON* item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "soubor", entry->d_name);
    cJSON_AddStringToObject(item, "nazev", entry->d_name);
    //printf("%s\n", entry->d_name);
    cJSON_AddItemToArray(root, item);
  }

  char *json_buffer = cJSON_PrintUnformatted(root);
  if (json_buffer == NULL) {
    perror("JSON root");
    return send_error(connection,MHD_HTTP_INTERNAL_SERVER_ERROR,"JSON");
  }

  struct MHD_Response *response;
  enum MHD_Result ret;

  // 1. Vytvoření odpovědi z bufferu
  response = MHD_create_response_from_buffer(
      strlen(json_buffer),
      (void *)json_buffer,
      MHD_RESPMEM_MUST_COPY
  );

  if (!response) {
    return MHD_NO;
  }

  // 2. Nastavení HTTP hlavičky pro JSON
  MHD_add_response_header(response, "Content-Type", "application/json");

  // 3. Zařazení odpovědi do fronty pro odeslání
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

  // 4. Uvolnění reference (libmicrohttpd drží interní referenci do dokončení přenosu)
  MHD_destroy_response(response);
  free(json_buffer);
  closedir(dir);

  return ret;
}

enum MHD_Result Endpoints(struct MHD_Connection *connection,const char *url, const char *method){

  if (strcmp(url, "/") == 0 || strncmp(url, "/static/", 8) == 0) {
    return HandleStatic(connection, url, method);
  }
  else if (strcmp(url,"/api/programs") == 0)
  {
    return HandlePrograms(connection, url, method);
  }
  return send_error(connection,404,"Endpoint not found");
}
