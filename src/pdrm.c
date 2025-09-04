#include "../include/pdrm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    struct semak_mule_response *resp = (struct semak_mule_response *)userdata;

    char *new_data = realloc(resp -> data, resp -> size + total + 1);

    if (!new_data) {
        fprintf(stderr, "realloc failed\n");
        return 0;
    }

    resp -> data = new_data;

    memcpy(&(resp -> data[resp -> size]), ptr, total);
    
    resp -> size += total;
    resp -> data[resp -> size] = '\0';
    
    return total;
}

int pdrm_semak_mule(const char *url, const char *json_payload, struct semak_mule_response *resp) {
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    struct curl_slist *headers = NULL;
    
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
    headers = curl_slist_append(headers, "apikey: j3j389#nklala2");
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