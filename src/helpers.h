//
// Created by david on 12.10.26.
//

#ifndef WEB_HELPERS_H
#define WEB_HELPERS_H

#include <microhttpd.h>

enum MHD_Result send_error(struct MHD_Connection *connection,unsigned int status_code,const char *message);
enum MHD_Result send_file(struct MHD_Connection *connection, const char *filepath, const char *mime_type);

/**
 * Resolves a requested file path safely within a root folder defined by an environment variable.
 *
 * @param relative_path The requested URL path or relative file path.
 * @param env_var_name  The environment variable holding the base path (e.g., "STATIC").
 * @param out_path      Buffer of at least PATH_MAX bytes to receive the resolved absolute path.
 * @return HTTP status code: 200 (OK), 403 (Forbidden), 404 (Not Found), or 500 (Server Error).
 */
int resolve_safe_path(const char *relative_path, const char *env_var_name, char *out_path);

#endif //WEB_HELPERS_H
