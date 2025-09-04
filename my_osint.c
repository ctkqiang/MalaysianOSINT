#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include <cjson/cJSON.h>

#include "./include/pdrm.h"


#define PORT 8080

static char *get_param(struct MHD_Connection *conn, const char *key) {
    const char *val = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, key);
    return val ? strdup(val) : NULL;
}

static enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                                      const char *url,
                                      const char *method,
                                      const char *version,
                                      const char *upload_data,
                                      size_t *upload_data_size,
                                      void **con_cls)
{
    char payload[512];
    char *search = NULL;

    (void) cls; (void) url; (void) version; (void) upload_data; (void) upload_data_size; (void) con_cls;

    if (strcmp(method, "GET") != 0) {
        const char *msg = "Only GET supported at the moment\n";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);

        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
        MHD_destroy_response(response);

        return ret;
    }

    search = get_param(connection, "q");
    
    if (!search) {
        const char *msg = "Missing parameter ?q=\n";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);

        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
    
        return ret;
    }

    snprintf(payload, sizeof(payload),
            "{\"data\":{\"category\":\"telefon\",\"bankAccount\":\"%s\","
            "\"telNo\":\"%s\",\"companyName\":\"\",\"captcha\":\"\"}}",
            search, search);

    struct semak_mule_response resp;
    
    int status = pdrm_semak_mule("https://semakmule.rmp.gov.my/api/mule/get_search_data.php", payload, &resp);

    if (status != 0) {
        free(search);
    
        const char *msg = "Remote request failed\n";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);

        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_GATEWAY, response);
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
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

    MHD_destroy_response(response);
    
    free(pretty);
    free(search);
    
    pdrm_semak_mule_response_free(&resp);

    return ret;
}


int main(int argc, char **argv) {
    int ch;
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &handle_request, NULL, MHD_OPTION_END);

    if (!daemon) {
        fprintf(stderr, "[错误] 无法启动 API 服务器，请检查端口 %d 是否被占用。\n", PORT);
        return EXIT_FAILURE;
    }

    printf("\n============================================================\n");
    printf("   马来西亚 OSINT 半自动查询器 (Malaysian OSINT)\n");
    printf("============================================================\n");
    printf(" 作者: 钟智强 <johnmelodymel@qq.com>\n");
    printf(" 版本: v0.1.0 (built on %s %s)\n", __DATE__, __TIME__);
    printf("------------------------------------------------------------\n");
    printf(" [警告] 根据马来西亚法律，禁止将本工具用于恶意用途。\n");
    printf("        作者不对任何滥用行为承担责任。\n");
    printf("        本工具仅供反诈骗、追踪骗子研究与教育使用。\n");
    printf("------------------------------------------------------------\n");
    printf(" Note: If you are PDRM or a government entity seeking\n");
    printf("       solutions like this, we are open for collaboration.\n");
    printf("============================================================\n");
    printf(" 服务已成功启动。\n");
    printf(" 运行地址:   http://localhost:%d\n", PORT);
    printf(" 使用说明:\n");
    printf("   1. 在浏览器或命令行工具中访问上述地址\n");
    printf("   2. 在 URL 后添加查询参数 ?q=电话号码 或 银行账号\n");
    printf("      例如: http://localhost:%d/?q=0123456789\n", PORT);
    printf("   3. 输入 'q' 并回车以安全关闭服务\n");
    printf("============================================================\n\n");

    while ((ch = getchar()) != EOF) {
        if (ch == 'q' || ch == 'Q') {
            printf("[提示] 正在关闭服务器...\n");
            break;
        }
    }

    MHD_stop_daemon(daemon);
    printf("[完成] 服务器已停止。\n");

    return EXIT_SUCCESS;
}
