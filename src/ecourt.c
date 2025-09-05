#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <unistd.h>

#include "../include/memory.h"
#include "../include/ecourt.h"

#define BASE_URL "https://efs.kehakiman.gov.my"
#define SEARCH_ENDPOINT "/EJudgmentWeb/Search"

/**
 * CURL写回调函数，用于接收HTTP响应数据并存储到动态分配的内存中
 * @param contents 指向接收到的数据块的指针
 * @param size 每个数据元素的大小（字节数）
 * @param nmemb 数据元素的数量
 * @param userp 指向用户自定义数据结构的指针（这里是struct memory）
 * @return 返回实际处理的数据大小，如果失败返回0
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *)userp;
    
    char *ptr = realloc(mem -> data, mem -> size + realsize + 1);

    if(!ptr) return 0;

    mem -> data = ptr;
    
    memcpy(&(mem -> data[mem -> size]), contents, realsize);
    
    mem -> size += realsize;
    mem -> data[mem -> size] = 0;
    
    return realsize;
}

static char* send_post_with_retry(const char* url, const char* post_data, int maxRetries, int delay_ms) {
    CURL *curl;
    CURLcode res;

    int attempts = 0;
    struct memory chunk;
    
    chunk.data = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {
        free(chunk.data);
        return NULL;
    }

    while (attempts < maxRetries) {
        chunk.size = 0;
    
        res = curl_easy_setopt(curl, CURLOPT_URL, url);
    
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            break; 
        } else {
            fprintf(stderr, "Request Failed (Retry %d/%d): %s\n", attempts+1, maxRetries, curl_easy_strerror(res));
            attempts++;
            
            if (attempts < maxRetries) {
                usleep(delay_ms * 1000);
            } 

            free(chunk.data);
            chunk.data = NULL;
        }
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    
    return chunk.data;
}

EJudgmentResponse* ejudgment_search(
    const char* search,
    const char* jurisdictionType,
    const char* courtCategory,
    const char* court,
    const char* judgeName,
    const char* caseType,
    const char* dateOfAPFrom,
    const char* dateOfAPTo,
    const char* dateOfResultFrom,
    const char* dateOfResultTo,
    int currPage,
    const char* ordering,
    int maxRetries,
    int delayBetweenRetries
) {
    char post_body[MAX_RESULT_LEN];
    snprintf(post_body, sizeof(post_body),
        "{"
            "\"Param\":{"
                "\"Search\":\"%s\","
                "\"JurisdictionType\":\"%s\","
                "\"CourtCategory\":\"%s\","
                "\"Court\":\"%s\","
                "\"JudgeName\":\"%s\","
                "\"CaseType\":\"%s\","
                "\"DateOfAPFrom\":\"%s\","
                "\"DateOfAPTo\":\"%s\","
                "\"DateOfResultFrom\":\"%s\","
                "\"DateOfResultTo\":\"%s\","
                "\"CurrPage\":%d,"
                "\"Ordering\":\"%s\""
            "}"
        "}",
        search,
        jurisdictionType,
        courtCategory,
        court,
        judgeName,
        caseType,
        dateOfAPFrom ? dateOfAPFrom : "",
        dateOfAPTo ? dateOfAPTo : "",
        dateOfResultFrom ? dateOfResultFrom : "",
        dateOfResultTo ? dateOfResultTo : "",
        currPage,
        ordering
    );

    char url[512];
    snprintf(url, sizeof(url), "%s%s", BASE_URL, SEARCH_ENDPOINT);

    char* raw_response = send_post_with_retry(url, post_body, maxRetries, delayBetweenRetries);

    if (!raw_response) return NULL;

    EJudgmentResponse* resp = malloc(sizeof(EJudgmentResponse));

    resp -> raw_json = raw_response;
    resp -> total_results = -1;
    
    return resp;
}

void ejudgment_response_free(EJudgmentResponse* resp) {
    if (!resp) return;
    if (resp -> raw_json) free(resp -> raw_json);
    
    free(resp);
}

char* ecourt_open_document(const char* documentId) {
    char* url = malloc(512);
    snprintf(url, 512, "https://efs.kehakiman.gov.my/EFSWeb/DocDownloader.aspx?DocumentID=%s&Inline=true", documentId);
    return url;
}