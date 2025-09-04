/**
 * @file pdrm.c
 * @brief 马来西亚皇家警察（PDRM）Semak Mule API客户端实现
 * 
 * 本模块提供与马来西亚皇家警察（PDRM）Semak Mule（"检查钱驴"）API交互的C接口，
 * 该API是国家金融欺诈预防系统的一部分，专门针对钱驴（money mule）活动。
 * 
 * 钱驴系统主要用于检测和防止利用银行账户进行非法资金转移的犯罪行为，
 * 通常涉及诈骗集团利用他人账户洗钱。金融机构和商家可通过此API验证
 * 交易是否存在钱驴风险。
 * 
 * 本模块实现了与PDRM Semak Mule API的交互，包括发送HTTP请求、
 * 处理响应数据和错误处理。
 * 
 */
#include "../include/pdrm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define ORIGIN "https://semakmule.rmp.gov.my/"

/**
* 警告：这儿为了图省事直接把密钥写死了，正式项目别学！
* 线上环境一定要从配置或环境变量读取，不然分分钟被黑
* 
* 话说PDRM SemakMule虽然用的是公钥验证
* 但这么设计确实有点让人看不懂，可能他们有别的考虑吧
*/
#define PUBLIC_KEY "j3j389#nklala2"  

/**
 * CURL写回调函数，用于接收HTTP响应数据并存储到用户定义的结构体中
 * 
 * @param ptr 指向接收到的数据的指针
 * @param size 每个数据元素的大小（字节数）
 * @param nmemb 数据元素的数量
 * @param userdata 指向用户数据的指针，这里应该是semak_mule_response结构体
 * 
 * @return 返回实际处理的数据大小（字节数），如果处理失败返回0
 */
static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    struct semak_mule_response *resp = (struct semak_mule_response *)userdata;

    // 重新分配内存以容纳新接收的数据
    char *new_data = realloc(resp -> data, resp -> size + total + 1);

    if (!new_data) {
        fprintf(stderr, "realloc failed\n");
        return 0;
    }

    resp -> data = new_data;

    // 将新数据复制到已分配的内存空间中
    memcpy(&(resp -> data[resp -> size]), ptr, total);
    
    resp -> size += total;
    resp -> data[resp -> size] = '\0';
    
    return total;
}

int pdrm_semak_mule(const char *url, const char *json_payload, struct semak_mule_response *resp) {
    CURL *curl;
    CURLcode res;

    char api_key[100];

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    struct curl_slist *headers = NULL;
    snprintf(api_key, "apikey: %s", PUBLIC_KEY);

    if (!url || !json_payload || !resp) return -1;
    
    resp -> data = malloc(1);
    resp -> size = 0;

    if (!(resp -> data)) return -1;

    if (!(curl)) {
        free(resp -> data);
        return -1;
    }

    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Accept-Language: zh-CN,zh;q=0.9");
    headers = curl_slist_append(headers, "Connection: keep-alive");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Origin: https://semakmule.rmp.gov.my");
    headers = curl_slist_append(headers, "Referer: https://semakmule.rmp.gov.my/");
    headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36 QQBrowser/19.5.5.207");
    headers = curl_slist_append(headers, api_key);
    headers = curl_slist_append(headers, "sec-ch-ua: \"Chromium\";v=\"123\", \"Not:A-Brand\";v=\"8\"");
    headers = curl_slist_append(headers, "sec-ch-ua-mobile: ?0");
    headers = curl_slist_append(headers, "sec-ch-ua-platform: \"macOS\"");
    headers = curl_slist_append(headers, "Cookie: *");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) resp);

    res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return (res == CURLE_OK) ? 0 : -1;
}

/**
 * 释放semak_mule_response结构体中的数据内存
 * 
 * 该函数用于安全地释放semak_mule_response结构体中动态分配的数据内存，
 * 并将相关字段重置为初始状态。
 * 
 * @param resp 指向semak_mule_response结构体的指针
 */
void pdrm_semak_mule_response_free(struct semak_mule_response *resp) {
    // 检查响应结构体及其数据指针是否有效 
    if (resp && resp->data) {
        // 释放数据内存 
        free(resp->data);

        // 重置数据指针和大小字段 
        resp -> data = NULL;
        resp -> size = 0;
    }
}