#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <microhttpd.h>
#include "../include/pdrm.h"
#include "../include/sspi.h"

// 在这里咱们模拟 HTTP 请求的辅助函数
static enum MHD_Result test_request(const char* url, char** response) {
    struct MHD_Daemon* d;
    struct MHD_Response* r;
    enum MHD_Result ret;
    
    d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 8080, NULL, NULL, &handle_request, NULL, MHD_OPTION_END);
    if (!d) return MHD_NO;
    
    // 模拟请求
    struct MHD_Connection* connection = MHD_create_connection(d);
    ret = handle_request(NULL, connection, url, "GET", "HTTP/1.1", NULL, NULL, NULL);
    
    if (ret == MHD_YES) {
        *response = strdup(MHD_get_response_header(r, MHD_HTTP_HEADER_CONTENT));
    }
    
    MHD_destroy_response(r);
    MHD_stop_daemon(d);

    return ret;
}

// 测试电话号码查询
void test_phone_query(void) {
    printf("测试电话号码查询功能...\n");
    
    char* response;
    enum MHD_Result ret = test_request("/?q=01127829038", &response);
    
    assert(ret == MHD_YES);
    assert(response != NULL);
    assert(strstr(response, "01127829038") != NULL);
    
    free(response);
    printf("电话号码查询测试通过！\n");
}

// 测试身份证号码查询
void test_ic_query(void) {
    printf("测试身份证号码查询功能...\n");
    
    char* response;
    enum MHD_Result ret = test_request("/?id=990101075678", &response);
    
    assert(ret == MHD_YES);
    assert(response != NULL);
    assert(strstr(response, "IC") != NULL);
    
    free(response);
    printf("身份证号码查询测试通过！\n");
}

// 测试错误处理
void test_error_handling(void) {
    printf("测试错误处理...\n");
    
    char* response;
    enum MHD_Result ret;
    
    // 测试无参数
    ret = test_request("/", &response);

    assert(ret == MHD_YES);
    assert(strstr(response, "Missing parameter") != NULL);

    free(response);
    
    // 测试无效参数
    ret = test_request("/?invalid=123", &response);

    assert(ret == MHD_YES);
    assert(strstr(response, "Missing parameter") != NULL);
    
    free(response);
    
    printf("错误处理测试通过！\n");
}

int main(void) {
    printf("开始运行单元测试...\n\n");
    
    test_phone_query();
    test_ic_query();
    test_error_handling();
    
    printf("\n所有测试都通过了！\n");
    return 0;
}