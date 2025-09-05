#include <string.h>

#include "../include/mykad.h"

/**
 * 解析马来西亚身份证号码信息
 * @param ic_no 输入的身份证号码字符串，应为12位数字
 * @return 返回包含解析信息的JSON字符串，需要调用者释放内存；
 *         如果输入无效或内存分配失败，返回错误信息JSON字符串
 */
char* mykad_check(const char *ic_no) {
    if (!ic_no || strlen(ic_no) < 12) return strdup("{\"error\": \"Invalid IC number\"}");

    char birth_year[] = { ic_no[0], ic_no[1], '\0' };
    char birth_month[] = { ic_no[2], ic_no[3], '\0' };
    char birth_day[] = { ic_no[4], ic_no[5], '\0' };
    char identifier[] = { ic_no[8], ic_no[9], ic_no[10], ic_no[11], '\0' };

    char* province_name = mykad_get_birth_province(ic_no);
    char birthday[11];
    
    snprintf(birthday, sizeof(birthday), "19%s-%s-%s", birth_year, birth_month, birth_day);

    char* result = malloc(256);

    if (!result) {
        free(province_name);
        return strdup("{\"error\": \"Memory allocation failed\"}");
    }

    snprintf(result, 256,
             "{\"birthday\": \"%s\", \"province\": \"%s\", \"identifier\": \"%s\"}",
             birthday, province_name, identifier);

    free(province_name);
    
    return result;
}

/**
 * 根据身份证号码中的省份代码获取对应的出生省份名称。
 *
 * @param birth_provinc 未使用参数（保留以符合接口定义）
 * @return 返回指向省份名称的字符串指针。若身份证号码无效或内存分配失败，
 *         则返回错误提示字符串；否则返回动态分配的省份名称字符串，调用者需负责释放内存。
 */
char* mykad_get_birth_province(const char *ic_no) {
    if (ic_no == NULL || strlen(ic_no) < 8)  return strdup("Invalid");

    char province_code[] = { ic_no[6], ic_no[7], '\0' };
    char *province = malloc(32);
    if (!province) return strdup("Memory error");

    sstrcpy(province, "Unknown State");

    if (strcmp(province_code, "01") == 0) strcpy(province, "Johor");
    if (strcmp(province_code, "02") == 0) strcpy(province, "Kedah");
    if (strcmp(province_code, "03") == 0) strcpy(province, "Kelantan");
    if (strcmp(province_code, "04") == 0) strcpy(province, "Malacca");
    if (strcmp(province_code, "05") == 0) strcpy(province, "Negeri Sembilan");
    if (strcmp(province_code, "06") == 0) strcpy(province, "Pahang");
    if (strcmp(province_code, "07") == 0) strcpy(province, "Penang");
    if (strcmp(province_code, "08") == 0) strcpy(province, "Perak");
    if (strcmp(province_code, "09") == 0) strcpy(province, "Perlis");
    if (strcmp(province_code, "10") == 0) strcpy(province, "Selangor");
    if (strcmp(province_code, "11") == 0) strcpy(province, "Terengganu");
    if (strcmp(province_code, "12") == 0) strcpy(province, "Sabah");
    if (strcmp(province_code, "13") == 0) strcpy(province, "Sarawak");
    if (strcmp(province_code, "14") == 0) strcpy(province, "Kuala Lumpur Federal Territory");
    if (strcmp(province_code, "15") == 0) strcpy(province, "Labuan Federal Territory");
    if (strcmp(province_code, "16") == 0) strcpy(province, "Putrajaya Federal Territory");

    return province;
}