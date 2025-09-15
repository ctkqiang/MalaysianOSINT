#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "../include/bnm.h"

#define BNM_SCAM_ALERT_LIST "https://www.bnm.gov.my/financial-consumer-alert-list"

static size_t write_cb(void *p, size_t size, size_t nmemb, void *u) {
    size_t tot = size * nmemb;
    struct buf *b = u;
    char *np = realloc(b->data, b->size + tot + 1);

    if (!np) return 0;

    b -> data = np;
    
    memcpy(b -> data + b -> size, p, tot);
    
    b -> size += tot;
    b -> data[b -> size] = '\0';
    
    return tot;
}

static char *strip_tags(const char *html) {
    size_t len = strlen(html);
    char *out = malloc(len + 1);

    if (!out) return NULL;
    
    size_t j = 0;
    
    int in_tag = 0;
    
    for (size_t i = 0; i < len; i++) {
        if (html[i] == '<') {
            in_tag = 1;

            // crude newline injection for <br> / <p>
            if (!strncmp(&html[i], "<br", 3) || !strncmp(&html[i], "<p", 2)) {
                out[j++] = '\n';
            }
            
            continue;
        }

        if (html[i] == '>') { in_tag = 0; continue; }
        if (!in_tag) out[j++] = html[i];
    }

    out[j] = '\0';
    
    return out;
}

int bnm_fetch_alerts(BNMAlertEntry **entries, size_t *count) {
    *entries = NULL;
    *count = 0;

    CURL *c = curl_easy_init();
    if (!c) return -1;

    struct buf b = {0};

    curl_easy_setopt(c, CURLOPT_URL, BNM_SCAM_ALERT_LIST);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "Mozilla/5.0");

    CURLcode res = curl_easy_perform(c);
    curl_easy_cleanup(c);

    if (res != CURLE_OK) {
        free(b.data);
        return -2;
    }

    const char *tbody = strstr(b.data, "<tbody");
    if (!tbody) { free(b.data); return -3; }

    const char *p = tbody;
    size_t cap = 16;
    *entries = calloc(cap, sizeof(BNMAlertEntry));

    while ((p = strstr(p, "<tr"))) {
        char *row_end = strstr(p, "</tr>");

        if (!row_end) break;

        size_t row_len = row_end - p;
        char *row_html = malloc(row_len + 1);

        memcpy(row_html, p, row_len);
        row_html[row_len] = '\0';

        BNMAlertEntry entry = {0};

        char *col = row_html;

        for (int i = 0; i < 3; i++) {
            char *td_start = strstr(col, "<td");
            if (!td_start) break;
            td_start = strchr(td_start, '>');
        
            if (!td_start) break;
            td_start++;

            char *td_end = strstr(td_start, "</td>");
            if (!td_end) break;

            size_t col_len = td_end - td_start;
            char *col_html = malloc(col_len + 1);
        
            memcpy(col_html, td_start, col_len);
            col_html[col_len] = '\0';

            char *plain = strip_tags(col_html);
            free(col_html);

            if (i == 0) entry.name = plain;
            if (i == 1) entry.website = plain;
            if (i == 2) entry.date = plain;

            col = td_end + 5;
        }

        if (*count == cap) {
            cap *= 2;
            *entries = realloc(*entries, cap * sizeof(BNMAlertEntry));
        }
        
        (*entries)[*count] = entry;
        (*count)++;

        free(row_html);
        p = row_end + 5;
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
