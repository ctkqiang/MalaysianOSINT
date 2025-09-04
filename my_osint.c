#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include <cjson/cJSON.h>

#include "./src/pdrm.c"

#define PORT 8080

static char *get_param(struct MHD_Connection *conn, const char *key) {
    const char *val = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, key);
    return val ? strdup(val) : NULL;
}

static int handle_request(void *cls, struct MHD_Connection *connection,
                          const char *url,
                          const char *method,
                          const char *version,
                          const char *upload_data,
                          size_t *upload_data_size,
                          void **con_cls
) {

    char payload[512];
    

    (void) cls; (void) url; (void) version; (void) upload_data; (void) upload_data_size; (void) con_cls;

    if (strcmp(method, "GET") != 0) {
        const char *msg = "Only GET supported at the moment\n";
        
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
        
        MHD_destroy_response(response);
        
        return ret;
    }

    snprintf(payload, sizeof(payload),
             "{\"data\":{\"category\":\"telefon\",\"bankAccount\":\"%s\","
             "\"telNo\":\"%s\",\"companyName\":\"\",\"captcha\":\"\"}}",
             search, search);

    struct mule_response resp;
    int status = mule_post("https://semakmule.rmp.gov.my/api/mule/get_search_data.php", payload, &resp);

    if (status != 0) {
        free(search);

        const char *msg = "Remote request failed\n";
        
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_BAD_GATEWAY, response);
        
        MHD_destroy_response(response);
        return ret;
    }

    cJSON *root = cJSON_Parse(resp.data);
    char *pretty = NULL;
    char *explain = NULL;

    if (root) {
        int count = cJSON_GetObjectItem(root, "count")->valueint;
        cJSON *table_data = cJSON_GetObjectItem(root, "table_data");
        int reported = 0;
        if (cJSON_IsArray(table_data) && cJSON_GetArraySize(table_data) > 0) {
            cJSON *row = cJSON_GetArrayItem(table_data, 0);
            if (cJSON_IsArray(row) && cJSON_GetArraySize(row) > 1) {
                reported = cJSON_GetArrayItem(row, 1)->valueint;
            }
        }

        pretty = cJSON_Print(root);

        size_t explain_len = 256;

        explain = malloc(explain_len);

        snprintf(explain, explain_len,
                 "Actually response:\n\n%s\n\n"
                 "Explanation:\nThe number %s has been searched %d times.\n"
                 "There are %d cases reported to PDRM.\n",
                 pretty, search, count, reported);

        cJSON_Delete(root);
    } else {
        explain = strdup("Failed to parse JSON response\n");
    }

    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(explain), (void *)explain, MHD_RESPMEM_MUST_FREE);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

    MHD_destroy_response(response);

    free(pretty);
    free(search);
    
    mule_response_free(&resp);

    return ret;
}

int main(int argc, char **argv) {
    return 0;
}