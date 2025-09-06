#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "../include/memory.h"
#include "../include/social.h"


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

void init_curl() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void cleanup_curl() {
    curl_global_cleanup();
}

int check_username(const SocialTarget *target, const char *username) {
    CURL *curl;
    CURLcode res;
    
    struct memory chunk = {0};
    int found = 0;

    curl = curl_easy_init();

    if(curl) {
        char full_url[512];
        snprintf(full_url, sizeof(full_url), target->url_template, username);

        curl_easy_setopt(curl, CURLOPT_URL, full_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "[%s] curl error: %s\n", target -> name, curl_easy_strerror(res));
        } else {
            long status;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

            if(status == 200 && strstr(chunk.data, username) != NULL) {
                printf("[+] %s: username exists at %s\n", target -> name, full_url);
                found = 1;
            } else {
                printf("[-] %s: username not found\n", target -> name);
            }
        }

        curl_easy_cleanup(curl);
        free(chunk.data);
    }

    return found;
}

SocialTarget targets[] = {
    {"GitHub", "https://github.com/%s"},
    {"Twitter", "https://twitter.com/%s"},
    {"Reddit", "https://www.reddit.com/user/%s"}
};

size_t targets_count = sizeof(targets) / sizeof(targets[0]);