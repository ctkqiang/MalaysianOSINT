/**
 * 马来西亚 OSINT 半自动查询器
 * 这是一个用于查询可疑电话号码和银行账号的 HTTP 服务器
 * 通过调用马来西亚皇家警察(PDRM)的 API 来验证目标是否涉及诈骗活动
 * 
 * @brief 马来西亚 OSINT 半自动查询器
 * @author 钟智强
 * @date 2023-12-15
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <microhttpd.h>
#include <cjson/cJSON.h>

#include "./include/pdrm.h"
#include "./include/mykad.h"
#include "./include/sspi.h"

#define PORT 8080

/**
 * 从 HTTP 请求中获取指定参数的值
 * @param conn MHD_Connection 连接对象
 * @param key 参数名
 * @return 参数值的副本(需要手动释放)，如果参数不存在返回 NULL
 */
static char *get_param(struct MHD_Connection *conn, const char *key) {
    const char *val = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, key);
    return val ? strdup(val) : NULL;
}

/**
 * HTTP 请求处理函数
 * 处理所有进入的 HTTP 请求，目前只支持 GET 方法
 * 主要功能是接收查询参数并调用 PDRM API 进行查询
 * @param cls 未使用
 * @param connection MHD_Connection 连接对象
 * @param url 请求 URL
 * @param method 请求方法
 * @param version HTTP 版本
 * @param upload_data 上传数据(未使用)
 * @param upload_data_size 上传数据大小(未使用)
 * @param con_cls 未使用
 * @return MHD_Result 处理结果
 */
static enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                                    const char *url,
                                    const char *method,
                                    const char *version,
                                    const char *upload_data,
                                    size_t *upload_data_size,
                                    void **con_cls)
{    (void) cls; (void) url; (void) version; 
    (void) upload_data; (void) upload_data_size; (void) con_cls;

    char client_ip[INET6_ADDRSTRLEN] = {0};
    const union MHD_ConnectionInfo *conn_info = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

    if (conn_info && conn_info -> client_addr) {
        struct sockaddr *sa = (struct sockaddr *) conn_info -> client_addr;
        if (sa -> sa_family == AF_INET) {
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa) -> sin_addr), client_ip, sizeof(client_ip));
        }
        
        if (sa -> sa_family == AF_INET6) {
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa) -> sin6_addr), client_ip, sizeof(client_ip));
        } 
        
        snprintf(client_ip, sizeof(client_ip), "未知地址族");
    } else {
        snprintf(client_ip, sizeof(client_ip), "未知IP");
    }

    printf("[访问日志] IP: %s | 方法: %s | 路径: %s\n", client_ip, method, url);

    if (strcmp(method, "GET") != 0) {
        const char *msg = "Only GET supported\n";
        struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(msg), (void*)msg, MHD_RESPMEM_PERSISTENT);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, resp);

        MHD_destroy_response(resp);
        return ret;
    }

    char *q = get_param(connection, "q");
    char *id = get_param(connection, "id");

    if (!q && !id) {
        const char *msg = "Missing parameter. Use either:\n"
                          "  ?q=PHONE_OR_BANK\n"
                          "  ?id=IC_NUMBER\n";

        struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(msg), (void*)msg, MHD_RESPMEM_PERSISTENT);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, resp);

        MHD_destroy_response(resp);

        return ret;
    }

    if (id) {
        char result_buf[1024];
        struct sspi_response sspi;

        int ok = sspi_check(id, &sspi);

        free(id);

        if (ok != 0) {
            const char *msg = "SSPI request failed\n";
            struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(msg), (void*)msg, MHD_RESPMEM_PERSISTENT);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_GATEWAY, resp);
            MHD_destroy_response(resp);
            return ret;
        }

        char *mykad_json = mykad_check(id);

        snprintf(result_buf, sizeof(result_buf),
            "IC: %s\nSSPI Status: %s\nMyKad Info: %s\n",
            id,
            strstr(sspi.status, "Tiada halangan") ? "Tiada Halangan" : "Halangan",
            mykad_json
        );

        struct MHD_Response *resp = MHD_create_response_from_buffer(
            strlen(result_buf), (void*)result_buf, MHD_RESPMEM_MUST_COPY
        );
        
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, resp);

        MHD_destroy_response(resp);
        sspi_response_free(&sspi);

        free(mykad_json);

        return ret;
    }

    if (q) {
        char payload[512];
        
        snprintf(payload, sizeof(payload),
                 "{\"data\":{\"category\":\"telefon\",\"bankAccount\":\"%s\","
                 "\"telNo\":\"%s\",\"companyName\":\"\",\"captcha\":\"\"}}",
                 q, q);

        struct semak_mule_response resp;
        int ok = pdrm_semak_mule("https://semakmule.rmp.gov.my/api/mule/get_search_data.php", payload, &resp);

        if (ok != 0) {
            free(q);
        
            const char *msg = "Remote request failed\n";
            struct MHD_Response *r = MHD_create_response_from_buffer(strlen(msg), (void*)msg, MHD_RESPMEM_PERSISTENT);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_GATEWAY, r);
        
            MHD_destroy_response(r);
        
            return ret;
        }

        cJSON *root = cJSON_Parse(resp.data);
        
        if (!root) {
            free(q);

            pdrm_semak_mule_response_free(&resp);
        
            const char *msg = "Failed to parse JSON response\n";
            struct MHD_Response *r = MHD_create_response_from_buffer(strlen(msg), (void*)msg, MHD_RESPMEM_PERSISTENT);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_GATEWAY, r);
        
            MHD_destroy_response(r);
        
            return ret;
        }

        int count = cJSON_GetObjectItem(root, "count") -> valueint;
        int reported = 0;

        cJSON *table_data = cJSON_GetObjectItem(root, "table_data");
        
        if (cJSON_IsArray(table_data) && cJSON_GetArraySize(table_data) > 0) {
            cJSON *row = cJSON_GetArrayItem(table_data, 0);
        
            if (cJSON_IsArray(row) && cJSON_GetArraySize(row) > 1) {
                reported = cJSON_GetArrayItem(row, 1) -> valueint;
            }
        }

        char *pretty = cJSON_Print(root);
        size_t buf_len = strlen(pretty) + 256;
        char *explain = malloc(buf_len);
        
        snprintf(explain, buf_len,
                 "Actually response:\n\n%s\n\n"
                 "Explanation:\nThe number %s has been searched %d times.\n"
                 "There are %d cases reported to PDRM.\n",
                 pretty, q, count, reported);

        struct MHD_Response *r = MHD_create_response_from_buffer(strlen(explain), explain, MHD_RESPMEM_MUST_FREE);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, r);
        
        MHD_destroy_response(r);

        free(pretty);
        free(q);
        
        cJSON_Delete(root);
        pdrm_semak_mule_response_free(&resp);
        
        return ret;
    }

    // 理论上不会到这里, 但是以防万一, 还是返回 400， 嘻嘻
    const char *msg = "Unhandled request\n";
    struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(msg), (void*)msg, MHD_RESPMEM_PERSISTENT);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, resp);

    MHD_destroy_response(resp);
    return ret;
}



/**
 * 主函数
 * 启动 HTTP 服务器并显示欢迎信息
 * 按 q 键可以优雅地关闭服务器
 */
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
    printf(" Note: If you are PDRM, MCMC or a government entity seeking\n");
    printf("       solutions like this, we are open for collaboration.\n");
    printf("============================================================\n");
    printf(" 服务已成功启动。\n");
    printf(" 运行地址:   http://localhost:%d\n", PORT);
    printf("------------------------------------------------------------\n");
    printf(" 可用接口 (API Endpoints):\n");
    printf("   1. PDRM 查询 (电话/银行账号):\n");
    printf("        http://localhost:%d/?q=0123456789\n", PORT);
    printf("   2. 移民局 SSPI 查询 (身份证号码):\n");
    printf("        http://localhost:%d/?id=1234567890\n", PORT);
    printf("------------------------------------------------------------\n");
    printf(" 操作说明:\n");
    printf("   - 在浏览器或 curl 中访问上述接口\n");
    printf("   - 输入 'q' 并回车以安全关闭服务\n");
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
