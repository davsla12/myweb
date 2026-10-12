//
// Created by david on 12.10.26.
//
#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>

#include "endpoints.h"
#include "helpers.h"

#define PORT 8888

struct connection_info_struct {
    int connection_type; // 0 = GET, 1 = POST
    struct MHD_PostProcessor *postprocessor;
    char *username;
    char *password;
    int response_sent;   // Příznak zabraňující opakovanému odeslání odpovědi
};

// Pomocná funkce pro odeslání HTML stránky s korektní UTF-8 hlavičkou
static enum MHD_Result send_page(struct MHD_Connection *connection, const char *page, int status_code) {
    struct MHD_Response *response;
    enum MHD_Result ret;

    response = MHD_create_response_from_buffer(strlen(page), (void *) page, MHD_RESPMEM_MUST_COPY);
    if (!response)
        return MHD_NO;

    MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");

    ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

// Callback pro zpracování POST dat
static enum MHD_Result iterate_post(void *coninfo_cls, enum MHD_ValueKind kind,
                                    const char *key, const char *filename,
                                    const char *content_type, const char *transfer_encoding,
                                    const char *data, uint64_t off, size_t size) {
    struct connection_info_struct *con_info = coninfo_cls;

    if (strcmp(key, "username") == 0) {
        char *new_username = realloc(con_info->username, off + size + 1);
        if (!new_username)
            return MHD_NO;

        con_info->username = new_username;
        memcpy(con_info->username + off, data, size);
        con_info->username[off + size] = '\0';
    }
    else if (strcmp(key, "password") == 0) {
        char *new_password = realloc(con_info->password, off + size + 1);
        if (!new_password)
            return MHD_NO;

        con_info->password = new_password;
        memcpy(con_info->password + off, data, size);
        con_info->password[off + size] = '\0';
    }

    return MHD_YES;
}

// Hlavní callback pro zpracování HTTP požadavků
static enum MHD_Result answer_to_connection(void *cls, struct MHD_Connection *connection,
                                            const char *url, const char *method,
                                            const char *version, const char *upload_data,
                                            size_t *upload_data_size, void **con_cls) {
    // 1. První volání pro nové připojení
    if (*con_cls == NULL) {
        struct connection_info_struct *con_info = calloc(1, sizeof(struct connection_info_struct));
        if (con_info == NULL)
            return MHD_NO;

        if (strcmp(method, "POST") == 0) {
            con_info->postprocessor = MHD_create_post_processor(connection, 1024, iterate_post, con_info);
            if (con_info->postprocessor == NULL) {
                free(con_info);
                return MHD_NO;
            }
            con_info->connection_type = 1;
        } else {
            con_info->connection_type = 0;
        }

        *con_cls = (void *) con_info;
        return MHD_YES;
    }

    printf("method: %s url: %s\n",method,url);
    printf("Size %lu bytes\n",*upload_data_size);
    printf("Data: %s\n",upload_data);

    return Endpoints(connection, url, method);
}

// Funkce pro úklid paměti po dokončení spojení
static void request_completed_callback(void *cls, struct MHD_Connection *connection,
                                       void **con_cls, enum MHD_RequestTerminationCode toe) {
    struct connection_info_struct *con_info = *con_cls;

    if (con_info == NULL)
        return;

    // Pokud šlo o POST požadavek, zničení postprocessoru
    if (con_info->postprocessor != NULL) {
        MHD_destroy_post_processor(con_info->postprocessor);
    }

    // Uvolnění alokovaných řetězců
    if (con_info->username != NULL) {
        free(con_info->username);
    }

    if (con_info->password != NULL) {
        free(con_info->password);
    }

    // Uvolnění hlavní struktury spojení
    free(con_info);
    *con_cls = NULL;
}

int main() {
    struct MHD_Daemon *daemon;

    // Pro otestování bez TLS (HTTP):
    daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,
        PORT,
        NULL, NULL,
        &answer_to_connection, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, &request_completed_callback, NULL,
        MHD_OPTION_END
    );

    if (daemon == NULL) {
        fprintf(stderr, "Chyba: Nelze spustit HTTP server.\n");
        return 1;
    }

    printf("Server běží na http://localhost:%d\nStiskněte Enter pro ukončení.\n", PORT);
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}