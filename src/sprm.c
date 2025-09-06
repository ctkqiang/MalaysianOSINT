#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <regex.h>

#include "../include/sprm.h"
#include "../include/memory.h"

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

/**
 * 通过HTTP请求获取指定URL的HTML内容
 * @param url 要获取HTML内容的URL地址
 * @return 返回获取到的HTML内容字符串，需要调用者负责释放内存，失败时返回NULL
 */
char *sprm_fetch_html(const char *url) {
    if (!url) return NULL;

    CURL *curl;
    CURLcode res;
    struct memory chunk;

    chunk.data = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (!curl) return NULL;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    if (res != CURLE_OK) {
        free(chunk.data);
        return NULL;
    }

    return chunk.data;
}

/**
 * 从HTML字符串中解析违规者信息列表
 * @param html 包含违规者信息的HTML字符串
 * @return 返回解析得到的违规者列表结构体
 */
PesalahList sprm_parse_html(const char *html) {
    PesalahList plist = {0};

    if (!html) return plist;

    const char *pattern = "<div class=\"col-md-3 div-pesalah\".*?</div>\\s*</div>";
    
    regex_t regex;
    regcomp(&regex, pattern, REG_ICASE | REG_NEWLINE | REG_EXTENDED);

    regmatch_t match[1];
    const char *cursor = html;

    while (regexec(&regex, cursor, 1, match, 0) == 0) {
        size_t len = match[0].rm_eo - match[0].rm_so;
        char *block = strndup(cursor + match[0].rm_so, len);

        plist.list = realloc(plist.list, sizeof(Pesalah) * (plist.count + 1));
        Pesalah *p = &plist.list[plist.count];
    
        memset(p, 0, sizeof(Pesalah));

        char *img = strstr(block, "<img src=\"");

        if (img) sscanf(img, "<img src=\"%511[^\"]", p->image_url);

        if (strstr(block, "Seksyen 165")) {
            strncpy(p->law, "Seksyen 165 Kanun Keseksaan", sizeof(p->law) - 1);
        }

        free(block);

        plist.count++;
        cursor += match[0].rm_eo;
    }

    regfree(&regex);
    return plist;
}

/**
 * 释放PesalahList结构体中的列表内存
 * @param plist 指向PesalahList结构体的指针
 * @return 无返回值
 * 
 * 该函数负责释放plist中list成员指向的动态内存，
 * 并将list指针置为NULL，count计数器置为0
 */
void sprm_free_list(PesalahList *plist) {
    if (plist -> list) free(plist -> list);
 
    plist -> list = NULL;
    plist -> count = 0;
}

/**
 * 打印所有违法者的信息
 * @param plist 指向PesalahList结构体的常量指针，包含违法者列表信息
 */
void sprm_print_all(const PesalahList *plist) {
    for (size_t i = 0; i < plist -> count; i++) {
        Pesalah *p = &plist -> list[i];
        printf("[%zu] %s | IC: %s | Law: %s | Sentence: %s\n", i+1, p -> name, p -> ic, p -> law, p -> sentence);
    }
}

/**
 * 在违规者列表中搜索包含指定关键词的记录
 * @param plist 指向要搜索的违规者列表的指针
 * @param keyword 要搜索的关键词
 * @return 返回包含所有匹配记录的新列表，需要调用者负责释放内存
 */
PesalahList sprm_search(const PesalahList *plist, const char *keyword) {
    PesalahList results = {0};

    if (!keyword || !(plist)) return results;

    for (size_t i = 0; i < plist -> count; i++) {
        Pesalah *p = &plist -> list[i];

        if ((p -> name[0] && strstr(p ->name, keyword)) ||
            (p -> ic[0] && strstr(p -> ic, keyword)) ||
            (p -> law[0] && strstr(p -> law, keyword))) {

            results.list = realloc(results.list, sizeof(Pesalah) * (results.count + 1));
            results.list[results.count] = *p;
            results.count++;
        }
    }

    return results;
}

/**
 * 将PesalahList结构体转换为JSON格式字符串
 * @param plist 指向PesalahList结构体的指针，包含要转换的数据列表
 * @return 返回动态分配的JSON格式字符串，调用者需要负责释放内存
 */
static char *sprm_list_to_json(const PesalahList *plist) {
    size_t offset = 0;
    size_t bufsize = 1024;

    char entry[2048];
    char *json = malloc(bufsize);
    
    offset += snprintf(json + offset, bufsize - offset, "[");

    for (size_t i = 0; i < plist -> count; i++) {
        Pesalah *p = &plist -> list[i];
    
        snprintf(entry, sizeof(entry),
            "{\"name\":\"%s\",\"ic\":\"%s\",\"state\":\"%s\",\"employer\":\"%s\","
            "\"position\":\"%s\",\"case_no\":\"%s\",\"charge\":\"%s\",\"law\":\"%s\","
            "\"sentence\":\"%s\",\"image_url\":\"%s\"}",
            p -> name, p -> ic, p -> state, p -> employer,
            p -> position, p -> case_no, p -> charge, p -> law,
            p -> sentence, p -> image_url
        );

        size_t need = strlen(entry) + 2;

        if (offset + need >= bufsize) {
            bufsize *= 2;
            json = realloc(json, bufsize);
        }

        offset += snprintf(json + offset, bufsize - offset, "%s%s", (i>0?",":""), entry);
    }

    snprintf(json + offset, bufsize - offset, "]");
    return json;
}

/**
 * 获取并处理SPRM数据的主函数
 * 
 * 该函数负责从指定URL获取HTML数据，解析其中的违规者信息，
 * 并将结果转换为JSON格式返回。
 * 
 * @return char* 返回JSON格式的字符串，包含解析后的数据或错误信息
 *               调用者需要负责释放返回字符串的内存
 */
char *sprm_run(void) {
    char *html = sprm_fetch_html(SPRM_URL);

    if (!html) return strdup("{\"error\":\"Failed to fetch SPRM data\"}");

    PesalahList list = sprm_parse_html(html);
    free(html);

    char *json = sprm_list_to_json(&list);
    sprm_free_list(&list);
 
    return json;
}