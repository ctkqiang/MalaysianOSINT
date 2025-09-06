#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "../include/memory.h"
#include "../include/company.h"


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
    struct memory *resp = (struct memory *)userdata;

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

static char *extract_between(const char *src, const char *start, const char *end) {
    char *p1 = strstr(src, start);
 
    if (!p1) return NULL;
    p1 += strlen(start);
 
    char *p2 = strstr(p1, end);
    if (!p2) return NULL;

    size_t len = p2 - p1;
    char *out = malloc(len + 1);
 
    strncpy(out, p1, len);
 
    out[len] = '\0';
 
    return out;
}

int company_search(const char *keyword, struct company_entry **results, size_t *count) {
    CURL *curl;
    CURLcode res;
    
    struct memory chunk = {0};

    curl = curl_easy_init();
    char url[512];
    
    if (!curl) {
        fprintf(stderr, "CURL init failed\n");
        return -1;
    }

    snprintf(url, sizeof(url), "https://malaysiayp.com/?s=%s&location-address=&a=true", keyword);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

    res = curl_easy_perform(curl);
 
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_cleanup(curl);

    char *name = extract_between(chunk.data, "<h3>", "</h3>");
    char *category = extract_between(chunk.data, "<span class=\"item-category\">", "</span>");
    char *address = extract_between(chunk.data, "<span class=\"value\">", "</span>");
    char *website = extract_between(chunk.data, "<div class=\"item-web\">", "</div>");

    *results = malloc(sizeof(struct company_entry));
    (*results)[0].name = name ? name : strdup("");
    (*results)[0].category = category ? category : strdup("");
    (*results)[0].address = address ? address : strdup("");
    (*results)[0].website = website ? website : strdup("");
    (*results)[0].source = strdup("MalaysiaYP");
 
    *count = 1;

    free(chunk.data);
 
    return 0;
}

void company_free_results(struct company_entry *results, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free(results[i].name);
        free(results[i].category);
        free(results[i].address);
        free(results[i].website);
        free(results[i].source);
    }
 
    free(results);
}