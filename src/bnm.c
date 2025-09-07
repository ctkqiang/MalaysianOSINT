#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "../include/bnm.h"

#define BNM_SCAM_ALERT_LIST "https://www.bnm.gov.my/financial-consumer-alert-list"

static size_t write_cb(void *p, size_t size, size_t nmemb, void *u) {
    size_t tot = size * nmemb;
    struct buf *b = u;
    char *np = realloc(b -> data, b -> size + tot + 1);

    if (!np) return 0;
    
    b -> data = np;
    
    memcpy(b -> data + b -> size, p, tot);
    
    b -> size += tot;
    b -> data[b -> size] = '\0';
    
    return tot;
}

int bnm_fetch_alerts(BNMAlertEntry **entries, size_t *count) {
    CURL *c = curl_easy_init();
    
    if (!c) return -1;

    struct buf b = {0};

    curl_easy_setopt(c, CURLOPT_URL, BNM_SCAM_ALERT_LIST);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "Mozilla/5.0  (Windows NT 10.0; Win64; x64; rv:89.0) Gecko/20100101 Firefox/89.0");
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    
    CURLcode res = curl_easy_perform(c);
    
    curl_easy_cleanup(c);
    
    if (res != CURLE_OK) free(b.data); return -2; 

    // 简单用字符串找 <td>……</td>
    const char *p = b.data;
    size_t cap = 16;

    *entries = calloc(cap, sizeof(BNMAlertEntry));
    *count = 0;

    while ((p = strstr(p, "<td")) && *count < cap) {
        // 下一项
        char *end = strstr(p, "</td>");
    
        if (!end) break;
    
        char *fragment = strndup(p, end - p);
    
        // 把 <td> 标签剔除 (抠 name, website, date)
        char *d = fragment;
    
        // 简化：提取第一个 link 和文本
        char *href = NULL, *text = NULL;
    
        if ((href = strstr(d, "href=\""))) {
            href += 6;
            char *hq = strchr(href, '"');
    
            *hq = 0;
    
            if ((text = strstr(hq + 1, ">"))) {
                text = strdup(text + 1);
                char *te = strchr(text, '<');

                if (te) *te = 0;
            }
        }
    
        (*entries)[*count].website = href ? strdup(href) : strdup("");
        (*entries)[*count].name = text ? text : strdup("");
        (*entries)[*count].date = strdup("");  // ........简化处理
        (*count)++;
    
        free(fragment);
    
        p = end + 5;
    
        if (*count == cap) {
            cap *= 2;
            *entries = realloc(*entries, cap * sizeof(BNMAlertEntry));
        }
    }

    free(b.data);
    
    return 0;
}

void bnm_free_alerts(BNMAlertEntry *entries, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free(entries[i].name);
        free(entries[i].website);
        free(entries[i].date);
    }
    
    free(entries);
}