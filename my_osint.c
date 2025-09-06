/**
 * @brief 马来西亚 OSINT 半自动查询器 (Malaysian OSINT Semi-Automated Query Tool)
 *
 * 这是一个基于 HTTP 的轻量级服务器，用于辅助查询潜在的诈骗目标信息。
 * 目前支持对 **可疑电话号码** 和 **银行账号** 进行检索，
 * 并通过调用 **马来西亚皇家警察 (PDRM) 官方 API** 验证目标是否涉及诈骗活动。
 *
 * 功能特点:
 * - 提供 RESTful 风格接口，支持 JSON 输入输出
 * - 查询结果基于官方数据源，确保数据可靠性
 * - 可扩展性设计，后续可支持更多数据源 (eCourt、RMP Wanted 等)
 *
 * 使用场景:
 * - 安全研究 / 网络威胁情报 (OSINT)
 * - 风险验证 / 金融风控
 * - 学术研究 / 公益用途
 *
 * ⚠️ 本工具仅供教育、研究与安全用途，请勿用于非法活动。
 *
 * @author  钟智强
 * @version v0.1.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <microhttpd.h>
#include <cjson/cJSON.h>

#include "./include/pdrm.h"
#include "./include/mykad.h"
#include "./include/sspi.h"
#include "./include/sprm.h"
#include "./include/ssm.h"
#include "./include/rmp_wanted.h"
#include "./include/memory.h"
#include "./include/ecourt.h"

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
static enum MHD_Result handle_request(
    void *cls, 
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls) {

    (void) cls; (void) url; (void) version; 
    (void) upload_data; (void) upload_data_size; (void) con_cls;

    char *q = get_param(connection, "q");
    char *id = get_param(connection, "id");
    char *name = get_param(connection, "name");
    char *ssm = get_param(connection, "ssm");

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

    if (!q && !id && !name) {
        const char *msg = "Missing parameter. Use either:\n"
                "  ?q=PHONE_OR_BANK\n"
                "  ?id=IC_NUMBER\n"
                "  ?name=NAME\n";

        struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(msg), (void*)msg, MHD_RESPMEM_PERSISTENT);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, resp);

        MHD_destroy_response(resp);

        return ret;
    }

    if (id) {
        char result_buf[8192];
        struct sspi_response sspi;

        // 在这里，咱们先做 SSPI 检查吧？
        int ok = sspi_check(id, &sspi);
        char *mykad_json = mykad_check(id);

        if (ok != 0) {
            const char *msg = "SSPI request failed\n";
            struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(msg), (void*)msg, MHD_RESPMEM_PERSISTENT);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_GATEWAY, resp);

            MHD_destroy_response(resp);
            sspi_response_free(&sspi);

            free(mykad_json);
            free(id);

            return ret;
        }

        // 马来西亚皇家警察局通缉名单 (PDRM Wanted)
        WantedPerson *wanted_list = NULL;
        WantedPerson wp = {0};

        char *html = rmp_fetch_wanted_html(PDRM_WANTED__LIST);
        int wanted_count = rmp_parse_wanted_list(html, &wanted_list);

        bool is_wanted = false;

        for (int i = 0; i < wanted_count; i++) {
        
            if (strstr(wanted_list[i].name, id)) {
                is_wanted = true;
                wp = wanted_list[i];
        
                break;
            }
        }
        
        rmp_free_html(html);

        // SPRM (马来西亚反贪会) 腐败罪犯名单
        char *sprm_html = sprm_fetch_html(SPRM_URL);
        PesalahList sprm_list = {0};
        PesalahList sprm_found = {0};

        bool is_pesalah = false;

        if (sprm_html) {
            sprm_list = sprm_parse_html(sprm_html);
            free(sprm_html);

            sprm_found = sprm_search(&sprm_list, id);

            if (sprm_found.count > 0) is_pesalah = true;
        }

        snprintf(result_buf, sizeof(result_buf),
            "IC: %s\nSSPI Status: %s\nMyKad Info: %s\nWanted: %s\nSPRM Pesalah: %s\n",
            id,
            strstr(sspi.status, "Tiada halangan") ? "Tiada Halangan" : "Halangan",
            mykad_json ? mykad_json : "{}",
            is_wanted ? "Yes" : "No",
            is_pesalah ? "Yes" : "No"
        );

        // 如果.....如果.....如果是通缉犯，追加详细信息
        if (is_wanted) {
            char wanted_details[512];
            
            snprintf(wanted_details, sizeof(wanted_details),
                "Wanted Person Details:\nName: %s\nAge: %s\nPhoto: %s\n",
                wp.name, wp.age, wp.photo_url
            );

            strncat(result_buf, wanted_details, sizeof(result_buf) - strlen(result_buf) - 1);
        }

        // 如果是贪污罪犯，追加详细信息.....
        if (is_pesalah) {
            for (size_t i = 0; i < sprm_found.count; i++) {
                char sprm_details[1024];
                
                snprintf(sprm_details, sizeof(sprm_details),
                    "SPRM Pesalah Details:\nName: %s\nIC: %s\nEmployer: %s\nPosition: %s\nCase: %s\nLaw: %s\nSentence: %s\n\n",
                    sprm_found.list[i].name,
                    sprm_found.list[i].ic,
                    sprm_found.list[i].employer,
                    sprm_found.list[i].position,
                    sprm_found.list[i].case_no,
                    sprm_found.list[i].law,
                    sprm_found.list[i].sentence
                );
                
                strncat(result_buf, sprm_details, sizeof(result_buf) - strlen(result_buf) - 1);
            }
        }

        struct MHD_Response *resp = MHD_create_response_from_buffer(
            strlen(result_buf), (void*)result_buf, MHD_RESPMEM_MUST_COPY
        );

        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, resp);

        MHD_destroy_response(resp);
        sspi_response_free(&sspi);

        free(mykad_json);
        free(id);
        free(wanted_list);

        if (sprm_list.count > 0) sprm_free_list(&sprm_list);
        if (sprm_found.count > 0) sprm_free_list(&sprm_found);

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

    if (name) {
        char result_buf[8192]; 

        EJudgmentResponse* resp = ejudgment_search(
            name,               // search
            "ALL",              // jurisdictionType
            "",                 // courtCategory
            "",                 // court
            "",                 // judgeName
            "",                 // caseType
            NULL,               // dateOfAPFrom
            NULL,               // dateOfAPTo
            NULL,               // dateOfResultFrom
            NULL,               // dateOfResultTo
            1,                  // currPage
            "DATE_OF_AP_DESC",  // ordering
            3,                  // maxRetries
            3000                // delayBetweenRetries
        );

        const char* json_text = resp ? resp->raw_json : "{}";

        snprintf(result_buf, sizeof(result_buf), "E-Court Search Results for: %s\n%s\n", name, json_text);

        struct MHD_Response *mhd_resp = MHD_create_response_from_buffer(
            strlen(result_buf), (void*)result_buf, MHD_RESPMEM_MUST_COPY
        );

        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, mhd_resp);
        MHD_destroy_response(mhd_resp);

        ejudgment_response_free(resp);

        return ret;
    }

    if (ssm) {
        char result_buf[8192];

        char *formatted = get_ssm_format(ssm);

        if (!formatted) {
            snprintf(result_buf, sizeof(result_buf), "Invalid SSM number: %s\n", ssm);

            struct MHD_Response *mhd_resp = MHD_create_response_from_buffer(
                strlen(result_buf), (void*)result_buf, MHD_RESPMEM_MUST_COPY
            );
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, mhd_resp);
        
            MHD_destroy_response(mhd_resp);
        
            return ret;
        }

        char *entity_desc = ssm_get_entity_code(formatted);

        snprintf(
            result_buf, sizeof(result_buf),
            "SSM Search Results for: %s\n"
            "{\n"
            "  \"ssm_number\": \"%s\",\n"
            "  \"entity_type\": \"%s\"\n"
            "}\n",
            ssm,
            formatted,
            entity_desc ? entity_desc : "Unknown"
        );

        struct MHD_Response *mhd_resp = MHD_create_response_from_buffer(
            strlen(result_buf), (void*)result_buf, MHD_RESPMEM_MUST_COPY
        );

        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, mhd_resp);

        MHD_destroy_response(mhd_resp);

        free(formatted);

        if (entity_desc) free(entity_desc);

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
 * 主函数：启动一个基于 libmicrohttpd 的 HTTP 服务器，用于提供 OSINT 查询服务。
 *
 * 参数:
 *   argc - 命令行参数数量（未使用）
 *   argv - 命令行参数数组（未使用）
 *
 * 返回值:
 *   EXIT_SUCCESS - 服务正常退出
 *   EXIT_FAILURE - 启动失败或发生错误
 */
int main(int argc, char **argv) {
    int ch;
    struct MHD_Daemon *daemon;

    // 启动 HTTP 服务器守护进程，监听指定端口并处理请求
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &handle_request, NULL, MHD_OPTION_END);

    // 检查服务器是否成功启动
    if (!daemon) {
        fprintf(stderr, "[错误] 无法启动 API 服务器，请检查端口 %d 是否被占用。\n", PORT);
        return EXIT_FAILURE;
    }

    // 打印程序信息和使用说明
    printf("\n");
    printf("███╗   ███╗██╗   ██╗ ██████╗ ███████╗██╗███╗   ██╗████████╗\n");
    printf("████╗ ████║╚██╗ ██╔╝██╔═══██╗██╔════╝██║████╗  ██║╚══██╔══╝\n");
    printf("██╔████╔██║ ╚████╔╝ ██║   ██║███████╗██║██╔██╗ ██║   ██║   \n");
    printf("██║╚██╔╝██║  ╚██╔╝  ██║   ██║╚════██║██║██║╚██╗██║   ██║   \n");
    printf("██║ ╚═╝ ██║   ██║   ╚██████╔╝███████║██║██║ ╚████║   ██║   \n");
    printf("╚═╝     ╚═╝   ╚═╝    ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═══╝   ╚═╝   \n");
    printf("马来西亚 OSINT 半自动查询系统\n");
    printf("==================================================\n");
    printf("           Malaysian OSINT Query System\n");
    printf("==================================================\n");
    printf("👨💻 作者: 钟智强 <johnmelodymel@qq.com>\n");
    printf("🔖 版本: v0.1.0 (编译: %s %s)\n", __DATE__, __TIME__);
    printf("==================================================\n");
    printf("⚠️  法律声明: 根据1997年电脑犯罪法令第5条文\n");
    printf("   仅限授权安全研究及反诈骗调查使用\n");
    printf("   严禁非法用途，违者必究\n");
    printf("==================================================\n");
    printf("🤝 政府合作: PDRM/MCMC/移民局欢迎技术合作\n");
    printf("==================================================\n");
    printf("🟢 服务状态: 已启动\n");
    printf("🌐 服务地址: http://localhost:%d\n", PORT);
    printf("==================================================\n");
    printf("📡 查询接口:\n");
    printf("==================================================\n");
    printf("1. 马来西亚皇家警察(PDRM)反钱驴检查系统 (Semak Mule)\n");
    printf("   http://localhost:%d/?q=0123456789\n", PORT);
    printf("--------------------------------------------------\n");
    printf("2. 移民局身份证信息查询 (SSPI)\n");
    printf("   http://localhost:%d/?id=1234567890\n", PORT);
    printf("--------------------------------------------------\n");
    printf("3. 马来西亚法庭记录查询 (ECourt)\n");
    printf("   http://localhost:%d/?name=姓名\n", PORT);
    printf("--------------------------------------------------\n");
    printf("4. 马来西亚皇家警察(PDRM)通缉名单核查 (PDRM Wanted List)\n");
    printf("   http://localhost:%d/?wanted=身份证号\n", PORT);
    printf("--------------------------------------------------\n");
    printf("5. 马来西亚公司注册资料查询 (SSM)\n");
    printf("   http://localhost:%d/?ssm=202001012345\n", PORT);
    printf("==================================================\n");
    printf("⌨️  操作指令:\n");
    printf("   q → 安全关闭    r → 重新加载\n");
    printf("   h → 帮助信息    \n");
    printf("==================================================\n");
    printf("💡 提示: 使用浏览器或curl访问上述接口进行查询\n");
    printf("==================================================\n\n");

    // 等待用户输入 'q' 或 'Q' 来安全关闭服务器
    while ((ch = getchar()) != EOF) {
        switch (ch) {
            case 'q':
            case 'Q':
                printf("[提示] 正在关闭服务器...\n");
                return 0;
                break;
            case 'h':
            case 'H':
                printf("[帮助] 可用查询接口:\n");
                printf(" 1. ?q=电话号码或银行账户\n");
                printf(" 2. ?id=身份证号码\n");
                printf(" 3. ?name=姓名\n");
                printf(" 4. ?wanted=身份证号\n");
                break;
            case 'r': 
            case 'R':
                printf("[提示] 正在重新加载配置/重启程序...\n");

                char *args[] = { "./mo", NULL }; 
                execv(args[0], args);
                perror("重启失败"); 

                exit(1);
                break;
            default:
                break;
                
        }
    }

    MHD_stop_daemon(daemon);
    printf("[完成] 服务器已停止。\n");

    return EXIT_SUCCESS;
}
