//
// Created by david on 12.10.26.
//
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "helpers.h"

#include <stdlib.h>
#include <linux/limits.h>

enum MHD_Result send_error(struct MHD_Connection *connection,
                           unsigned int status_code,
                           const char *message) {
  struct MHD_Response *response = MHD_create_response_from_buffer(
      strlen(message), (void *)message, MHD_RESPMEM_PERSISTENT);
  if (!response) {
    return MHD_NO;
  }

  enum MHD_Result ret = MHD_queue_response(connection, status_code, response);
  MHD_destroy_response(response);
  return ret;
}

// Helper function to resolve MIME type by file extension
static const char *get_mime_type_from_extension(const char *filepath) {
    const char *dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) {
        return "application/octet-stream";
    }

    const char *ext = dot + 1;

    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) return "text/html";
    if (strcasecmp(ext, "css")  == 0) return "text/css";
    if (strcasecmp(ext, "js")   == 0) return "text/javascript";
    if (strcasecmp(ext, "json") == 0) return "application/json";
    if (strcasecmp(ext, "png")  == 0) return "image/png";
    if (strcasecmp(ext, "jpg")  == 0 || strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "gif")  == 0) return "image/gif";
    if (strcasecmp(ext, "svg")  == 0) return "image/svg+xml";
    if (strcasecmp(ext, "pdf")  == 0) return "application/pdf";
    if (strcasecmp(ext, "txt")  == 0) return "text/plain";
    if (strcasecmp(ext, "wasm") == 0) return "application/wasm";

    return "application/octet-stream";
}

int resolve_safe_path(const char *relative_path, const char *env_var_name, char *out_path) {
    if (!relative_path || !env_var_name || !out_path) {
        return 500;
    }

    const char *env_base = getenv(env_var_name);
    if (!env_base) {
        fprintf(stderr, "Error: Environment variable '%s' is not set.\n", env_var_name);
        return 500;
    }

    // 1. Resolve base directory canonical path (resolves symlinks and relative parts)
    char resolved_base[PATH_MAX];
    if (realpath(env_base, resolved_base) == NULL) {
        perror("Error resolving base directory");
        return 500;
    }

    // 2. Combine base directory and requested file path
    char target_buffer[PATH_MAX];
    if (relative_path[0] == '/') {
        snprintf(target_buffer, sizeof(target_buffer), "%s%s", resolved_base, relative_path);
    } else {
        snprintf(target_buffer, sizeof(target_buffer), "%s/%s", resolved_base, relative_path);
    }

    // 3. Resolve target file canonical path
    if (realpath(target_buffer, out_path) == NULL) {
        return 404; // File does not exist
    }

    // 4. Verify resolved path stays inside resolved_base
    size_t base_len = strlen(resolved_base);
    if (strncmp(out_path, resolved_base, base_len) != 0) {
        return 403; // Attempted directory escape
    }

    // 5. Prevent prefix boundary vulnerabilities (e.g., /static_secret matching /static)
    if (out_path[base_len] != '\0' && out_path[base_len] != '/') {
        return 403;
    }

    return 200;
}

enum MHD_Result send_file(struct MHD_Connection *connection, const char *filepath, const char *mime_type) {

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Failed to open %s\n", filepath);
        perror("Error opening file");
        return send_error(connection, MHD_HTTP_NOT_FOUND, "404 not found");
    }

    struct stat sbuf;
    if (fstat(fd, &sbuf) < 0) {
        perror("Error stating file");
        close(fd);
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "500 Internal Server Error");
    }

    // Creates response directly from the file descriptor.
    // libmicrohttpd takes ownership of 'fd' and will close it automatically.
    struct MHD_Response *response = MHD_create_response_from_fd((uint64_t)sbuf.st_size, fd);
    if (!response) {
        close(fd);
        return send_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "500 Internal Server Error");
    }

    // Automatically determine MIME type if requested or if NULL
    const char *final_mime = mime_type;
    if (final_mime == NULL || strcmp(final_mime, "auto") == 0) {
        final_mime = get_mime_type_from_extension(filepath);
    }

    // Set MIME content-type header
    MHD_add_response_header(response, "Content-Type", final_mime);

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}