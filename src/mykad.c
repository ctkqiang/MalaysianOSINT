#include <string.h>

#include "../include/mykad.h"

/**
 * 解析马来西亚身份证号码信息
 * @param ic_no 输入的身份证号码字符串，应为12位数字
 * @return 返回包含解析信息的JSON字符串，需要调用者释放内存；
 *         如果输入无效或内存分配失败，返回错误信息JSON字符串
 */
char* mykad_check(const char *ic_no) {
    // 验证输入参数有效性
    if (ic_no == NULL || strlen(ic_no) < 12) return "{\"error\": \"Invalid IC number\"}";

    // 从身份证号码中提取各个字段信息
    char birth_year[] = { ic_no[0], ic_no[1], '\0' };
    char birth_month[] = { ic_no[2], ic_no[3], '\0' };
    char birth_day[] = { ic_no[4], ic_no[5], '\0' };
    char birth_province_code[] = { ic_no[6], ic_no[7], '\0' };
    char identifier[] = { ic_no[8], ic_no[9], ic_no[10], ic_no[11], '\0' };

    // 获取出生省份名称并构造生日字符串
    char* province_name = mykad_get_birth_province(ic_no);
    char birthday[11];

    snprintf(birthday, sizeof(birthday), "19%s-%s-%s", birth_year, birth_month, birth_day);

    // 构造并返回JSON格式的结果
    char* result = malloc(200);
    if (result == NULL) {
        free(province_name);
        return "{\"error\": \"Memory allocation failed\"}";
    }

    snprintf(result, 200,  "{\"birthday\": \"%s\", \"province\": \"%s\", \"identifier\": \"%s\"}", birthday, province_name, identifier);

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
char* mykad_get_borth_province(char* birth_provinc[]) {
    if (ic_no == NULL || strlen(ic_no) < 8)  return "Invalid";

    // 提取身份证号码中表示省份的两位代码
    char province_code[] = { ic_no[6], ic_no[7], '\0' };
    
    // 分配用于存储省份名称的内存空间
    char* province = malloc(20);

    if (province == NULL) return "Memory error";
    
    // 默认设置为未知州属，适用于无匹配情况（可能为非马来西亚国民）
    strcpy(province, "未知州属");

    // 根据省份代码匹配并设置对应的省份名称
    if (strcmp(province_code, "01") == 0) strcpy(province, "柔佛州");
    if (strcmp(province_code, "02") == 0) strcpy(province, "吉打州");
    if (strcmp(province_code, "03") == 0) strcpy(province, "吉兰丹州");
    if (strcmp(province_code, "04") == 0) strcpy(province, "马六甲州");
    if (strcmp(province_code, "05") == 0) strcpy(province, "森美兰州");
    if (strcmp(province_code, "06") == 0) strcpy(province, "彭亨州");
    if (strcmp(province_code, "07") == 0) strcpy(province, "槟城州");
    if (strcmp(province_code, "08") == 0) strcpy(province, "霹雳州");
    if (strcmp(province_code, "09") == 0) strcpy(province, "玻璃市州");
    if (strcmp(province_code, "10") == 0) strcpy(province, "雪兰莪州");
    if (strcmp(province_code, "11") == 0) strcpy(province, "登嘉楼州");
    if (strcmp(province_code, "12") == 0) strcpy(province, "沙巴州");
    if (strcmp(province_code, "13") == 0) strcpy(province, "砂拉越州");
    if (strcmp(province_code, "14") == 0) strcpy(province, "吉隆坡联邦直辖区");
    if (strcmp(province_code, "15") == 0) strcpy(province, "纳闽联邦直辖区");
    if (strcmp(province_code, "16") == 0) strcpy(province, "布城联邦直辖区");

    return province;
}