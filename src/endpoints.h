//
// Created by david on 12.10.26.
//

#ifndef WEB_ENDPOINTS_H
#define WEB_ENDPOINTS_H

#include <microhttpd.h>

/**
 * Handles endpoints
 *
 * @param connection
 * @param url
 * @param method
 * @return HTTP Status
*/
enum MHD_Result Endpoints(struct MHD_Connection *connection,const char *url, const char *method);

#endif //WEB_ENDPOINTS_H
