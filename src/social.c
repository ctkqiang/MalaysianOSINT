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