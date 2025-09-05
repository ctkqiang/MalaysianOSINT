#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "../include/sspi.h"

#define SSPI_URL "https://sspi.imi.gov.my/sspi/index.php?page=sspi/bm"

/**
 * CURL写回调函数，用于将接收到的数据写入内存缓冲区
 * @param contents 指向接收到的数据的指针
 * @param size 每个数据元素的大小
 * @param nmemb 数据元素的数量
 * @param userp 指向用户自定义数据结构的指针（这里是一个memory结构体）
 * @return 返回实际处理的数据大小，如果失败返回0
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *)userp;

    // 重新分配内存以容纳新数据
    char *ptr = realloc(mem -> data, mem -> size + realsize + 1);

    if (!ptr) return 0;
    
    mem -> data = ptr;

    // 将新数据复制到内存缓冲区
    memcpy(&(mem -> data[mem -> size]), contents, realsize);
    
    mem -> size += realsize;
    mem -> data[mem -> size] = 0;

    return realsize;
}

/**
 * 检查SSPI身份信息
 * @param ic_no 身份证号码字符串
 * @param resp 用于存储响应结果的结构体指针
 * @return 0表示成功，-1表示curl初始化失败，-2表示curl执行失败
 */
int sspi_check(const char *ic_no, struct sspi_response *resp) {
    CURL *curl;
    CURLcode res;
    struct memory chunk = {0};

    // 初始化curl句柄 
    curl = curl_easy_init();

    if (!curl) return -1;

    char postfields[128];
    
    // 构造POST请求数据 
    snprintf(postfields, sizeof(postfields), "txtIcNo=%s&btnSemak=Semak", ic_no);

    // 设置curl选项 
    curl_easy_setopt(curl, CURLOPT_URL, SSPI_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // 执行HTTP请求 
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return -2;
    }

    resp -> raw_html = chunk.data;

    // 解析HTML响应，提取状态码信息 
    const char *needle = "<span id=\"lblStatuscode\"";
    char *p = strstr(resp -> raw_html, needle);
    
    if (p) {
        p = strchr(p, '>');
    
        if (p) {
            p++;

            char *end = strstr(p, "</span>");
    
            if (end) {
                size_t len = end - p;
    
                resp -> status = malloc(len + 1);
                strncpy(resp->status, p, len);
                resp -> status[len] = '\0';
            }
        }
    }

    // 如果未找到状态信息，则设置默认值 
    if (!(resp -> status)) {
        resp -> status = strdup("Unknown");
    }

    // 清理curl资源 
    curl_easy_cleanup(curl);
    
    return 0;
}

/**
 * 释放SSPI响应结构体中的动态分配内存
 * 
 * 该函数负责释放struct sspi_response结构体中动态分配的内存资源，
 * 包括原始HTML内容和状态信息字符串。在释放内存后，相应的指针
 * 将变为无效，不应再被访问。
 * 
 * @param resp 指向要释放内存的SSPI响应结构体的指针
 * 
 * 注意：该函数不释放resp结构体本身，仅释放其内部动态分配的成员
 */
void sspi_response_free(struct sspi_response *resp) {
    if (resp -> raw_html) free(resp -> raw_html);
    if (resp -> status) free(resp -> status);
}