//
// Created by david on 12.10.26.
//

#include "endpoints.h"

#include <string.h>
#include <linux/limits.h>

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

enum MHD_Result Endpoints(struct MHD_Connection *connection,const char *url, const char *method){

  if (strcmp(url, "/") == 0 || strncmp(url, "/static/", 8) == 0) {
    return HandleStatic(connection, url, method);
  }
  return send_error(connection,404,"Endpoint not found");
}
