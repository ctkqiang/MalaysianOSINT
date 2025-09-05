#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>

#include "../include/rmp_wanted.h"
#include "../include/sspi.h"
#include "../include/mykad.h"
#include "../include/memory.h"

#ifndef PDRM_WANTED__LIST
#define PDRM_WANTED__LIST "https://www.rmp.gov.my/orang-dikehendaki"
#endif


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

/**
 * 从指定URL获取HTML内容
 * @param url 要获取HTML内容的URL地址
 * @return 返回获取到的HTML内容字符串，需要调用者负责释放内存；如果获取失败则返回NULL
 */
char* rmp_fetch_wanted_html(const char* url) {
    CURL *curl;
    CURLcode res;

    struct memory chunk = {0};

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if(curl) {
        chunk.data = malloc(1);
        chunk.size = 0;
    
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
    
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.data);
    
            chunk.data = NULL;
        }
    
        curl_easy_cleanup(curl);
    }
    
    curl_global_cleanup();
    
    return chunk.data;
}

/**
 * 释放HTML字符串的内存
 * 
 * @param html 指向要释放的HTML字符串的指针
 * 
 * 该函数用于安全地释放通过动态内存分配创建的HTML字符串。
 * 它会检查指针是否为NULL，避免对空指针进行释放操作。
 */
void rmp_free_html(char* html) {
    /* 释放HTML字符串内存 */
    if(html) free(html);
}

/**
 * 从HTML内容中解析通缉人员列表
 * 
 * @param html  包含通缉人员信息的HTML字符串
 * @param list  指向WantedPerson结构体数组的指针，用于存储解析出的通缉人员信息
 * 
 * @return 返回解析到的通缉人员数量
 * 
 * 该函数会解析HTML中所有class为"wanted-person"的div元素，
 * 提取其中的姓名、年龄和照片URL信息，并动态分配内存存储这些信息。
 */
int rmp_parse_wanted_list(const char* html, WantedPerson** list) {
    int count = 0;
    *list = NULL;
    const char* ptr = html;
    
    while((ptr = strstr(ptr, "<div class=\"wanted-person\">")) != NULL) {
        *list = realloc(*list, (count+1) * sizeof(WantedPerson));
    
        if(!*list) return count;

        WantedPerson* wp = &(*list)[count];

        const char* name_start = strstr(ptr, "<h3>");
        const char* name_end = strstr(ptr, "</h3>");
    
        if(name_start && name_end && name_end > name_start) {
            size_t len = name_end - (name_start+4);
    
            strncpy(wp -> name, name_start+4, len < MAX_NAME_LEN-1 ? len : MAX_NAME_LEN-1);
            wp -> name[len < MAX_NAME_LEN-1 ? len : MAX_NAME_LEN-1] = '\0';
        } else wp -> name[0] = '\0';

        const char* age_start = strstr(ptr, "<span class=\"age\">");
        const char* age_end = strstr(ptr, "</span>");
        
        if(age_start && age_end && age_end > age_start) {
            size_t len = age_end - (age_start+19);
        
            strncpy(wp -> age, age_start+19, len < MAX_AGE_LEN-1 ? len : MAX_AGE_LEN-1);
            wp -> age[len < MAX_AGE_LEN-1 ? len : MAX_AGE_LEN-1] = '\0';
        } else wp -> age[0] = '\0';

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

/**
 * 处理ID请求，整合SSPI、MyKad和RMP通缉名单的查询结果，返回JSON格式的响应。
 *
 * @param id 身份证号码字符串，作为查询的主键。
 * @return 返回一个动态分配的JSON字符串，包含查询结果。调用者需负责释放内存。
 *         如果出错，返回的JSON中包含error字段。
 */
char* handle_id_request(const char* id) {
    struct sspi_response sspi;
    int ok = sspi_check(id, &sspi);

    if(ok != 0) {
        char *msg = strdup("{\"error\":\"SSPI request failed\"}");
        return msg;
    }

    char *mykad_json = mykad_check(id);

    WantedPerson *wanted_list = NULL;

    char *html = rmp_fetch_wanted_html(PDRM_WANTED__LIST);
    int wanted_count = 0;
    bool is_wanted = false;

    WantedPerson wp = {0};

    if(html) {
        wanted_count = rmp_parse_wanted_list(html, &wanted_list);

        for(int i=0; i<wanted_count; i++) {
            
            if(strstr(wanted_list[i].name, id)) {
                is_wanted = true;
                wp = wanted_list[i];

                break;
            }
        }
        rmp_free_html(html);
        if(wanted_list) free(wanted_list);
    }

    size_t buf_len = 1024 + (mykad_json ? strlen(mykad_json) : 0);
    char *result_buf = malloc(buf_len);
    
    snprintf(result_buf, buf_len,
        "{\n"
            "  \"id\":\"%s\",\n"
            "  \"sspi_status\":\"%s\",\n"
            "  \"mykad_info\":%s,\n"
            "  \"rmp_wanted\":%s,\n"
            "  \"wanted_name\":\"%s\",\n"
            "  \"wanted_age\":\"%s\",\n"
            "  \"wanted_photo\":\"%s\"\n"
        "}",
        id,
        strstr(sspi.status, "Tiada halangan") ? "Tiada Halangan" : "Halangan",
        mykad_json ? mykad_json : "{}",
        is_wanted ? "true" : "false",
        is_wanted ? wp.name : "",
        is_wanted ? wp.age : "",
        is_wanted ? wp.photo_url : ""
    );

    sspi_response_free(&sspi);
    
    if(mykad_json) free(mykad_json);

    return result_buf;
}
