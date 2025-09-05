#include "../include/rmp_wanted.h"

#define PDRM_WANTED__LIST "https://www.rmp.gov.my/orang-dikehendaki"

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

char* rmp_fetch_wanted_html(const char* url) {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {0};

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        chunk.memory = malloc(1);
        chunk.size = 0;

        curl_easy_setopt(curl, CURLOPT_URL, PDRM_WANTED__LIST);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
    
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    
            free(chunk.memory);
            chunk.memory = NULL;
        }
    
        curl_easy_cleanup(curl);
    }
    
    curl_global_cleanup();
    
    return chunk.memory;
}

/**
 * 释放HTML字符串的内存
 * 
 * @param html 指向HTML字符串的指针，需要被释放的内存地址
 * 
 * @return 无返回值
 * 
 * 该函数用于安全地释放通过动态内存分配创建的HTML字符串。
 * 它会检查指针是否为NULL，避免对空指针进行释放操作。
 */
void rmp_free_html(char* html) {
    /* 释放HTML字符串内存 */
    if(html) free(html);
}

int rmp_parse_wanted_list(const char* html, WantedPerson** list) {
    int count = 0;
    *list = NULL;

    const char* ptr = html;
    
    while((ptr = strstr(ptr, "<div class=\"wanted-person\">")) != NULL) {
        *list = realloc(*list, (count+1) * sizeof(WantedPerson));
        if(!*list) return count;

        WantedPerson* wp = &(*list)[count];

        // 通缉犯名字
        const char* name_start = strstr(ptr, "<h3>");
        const char* name_end = strstr(ptr, "</h3>");
        
        if(name_start && name_end && name_end > name_start) {
            size_t len = name_end - (name_start+4);
          
            strncpy(wp -> name, name_start+4, len < MAX_NAME_LEN-1 ? len : MAX_NAME_LEN-1);
            wp -> name[len < MAX_NAME_LEN-1 ? len : MAX_NAME_LEN-1] = '\0';
        } else wp -> name[0] = '\0';

        // 通缉犯年龄
        const char* age_start = strstr(ptr, "<span class=\"age\">");
        const char* age_end = strstr(ptr, "</span>");

        if(age_start && age_end && age_end > age_start) {
            size_t len = age_end - (age_start+19);
            
            strncpy(wp -> age, age_start+19, len < MAX_AGE_LEN-1 ? len : MAX_AGE_LEN-1);
            wp -> age[len < MAX_AGE_LEN-1 ? len : MAX_AGE_LEN-1] = '\0';
        } else wp->age[0] = '\0';

        // 通缉犯照片连接
        const char* photo_start = strstr(ptr, "<img src=\"");
        const char* photo_end = strstr(photo_start ? photo_start+10 : NULL, "\"");

        if(photo_start && photo_end && photo_end > photo_start) {
            size_t len = photo_end - (photo_start+10);
        
            strncpy(wp -> photo_url, photo_start+10, len < MAX_PHOTO_LEN-1 ? len : MAX_PHOTO_LEN-1);
            wp -> photo_url[len < MAX_PHOTO_LEN-1 ? len : MAX_PHOTO_LEN-1] = '\0';
        } else wp -> photo_url[0] = '\0';

        count++;
        ptr = name_end;
    }

    return count;
}