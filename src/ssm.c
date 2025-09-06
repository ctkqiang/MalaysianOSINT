#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "../include/ssm.h"

/**
 * 获取SSM格式化字符串
 * 
 * 该函数接收一个SSM号码字符串，提取其中的数字字符并格式化为12位数字字符串
 * 
 * @param ssm_number 输入的SSM号码字符串，可以包含非数字字符
 * @return 成功时返回12位数字组成的字符串（需要调用者释放内存），失败时返回NULL
 */
char* get_ssm_format(const char *ssm_number) {
    if (ssm_number == NULL) return NULL;

    char *normalized = malloc(13);
    int j = 0;

    for (int i = 0; ssm_number[i] != '\0'; i++) {
        if (isdigit((unsigned char) ssm_number[i])) {
            if (j < 12) normalized[j++] = ssm_number[i];
        }
    }

    normalized[j] = '\0';

    if (j != 12) {
        free(normalized);
        return NULL;
    }

    return normalized;
}

/**
 * 根据SSM编号获取实体代码对应的描述
 * @param ssm_number SSM编号字符串，应为12位长度
 * @return 返回实体代码对应的描述字符串，需要调用者释放内存；
 *         如果SSM编号无效或未找到匹配的实体代码，返回相应的错误描述
 */
char* ssm_get_entity_code(const char *ssm_number) {
    if (ssm_number == NULL || strlen(ssm_number) != 12) {
        return strdup("Invalid SSM number");
    }

    char entity_code[3];
    
    entity_code[0] = ssm_number[4];
    entity_code[1] = ssm_number[5];
    entity_code[2] = '\0';

    struct { const char *code; const char *desc; } mapping[] = {
        {"01", "Local Company"},
        {"02", "Foreign Company"},
        {"03", "Business Entity"},
        {"04", "Local Limited Liability Partnership (LLP)"},
        {"05", "Foreign Limited Liability Partnership (LLP)"},
        {"06", "Local Liability Partnership for Professional Practice"}
    };

    for (int i = 0; i < sizeof(mapping) / sizeof(mapping[0]); i++) {
        if (strcmp(entity_code, mapping[i].code) == 0) {
            return strdup(mapping[i].desc); 
        }
    }

    return strdup("Unknown Entity");
}
