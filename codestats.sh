#!/bin/bash

# 定义要统计的文件扩展名（可以根据需要修改）
FILE_TYPES="*.js *.ts *.py *.java *.c *.cpp *.h *.hpp *.html *.css *.go *.rs *.php *.rb *.swift *.kt"

echo "📁 Starting code analysis..."
echo "================================================"

# 临时文件用于存储结果
temp_file=$(mktemp)

# 查找并统计文件
find . -type f \( -name "*.js" -o -name "*.ts" -o -name "*.py" -o -name "*.java" \
                 -o -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \
                 -o -name "*.html" -o -name "*.css" -o -name "*.go" -o -name "*.rs" \
                 -o -name "*.php" -o -name "*.rb" -o -name "*.swift" -o -name "*.kt" \) \
                 ! -path "*/node_modules/*" ! -path "*/.git/*" ! -path "*/venv/*" ! -path "*/dist/*" \
                 -exec wc -l {} + > "$temp_file"

# 计算总数和文件数
total_lines=$(awk '{sum += $1} END {print sum}' "$temp_file")
total_files=$(grep -c "^[[:space:]]*[0-9]\+[[:space:]]" "$temp_file")

# 按文件类型统计
echo ""
echo "📊 LANGUAGE BREAKDOWN:"
echo "----------------------------------------"
awk '
BEGIN {
    # 映射扩展名到语言名称
    lang_names[".js"] = "JavaScript"
    lang_names[".ts"] = "TypeScript"
    lang_names[".py"] = "Python"
    lang_names[".java"] = "Java"
    lang_names[".c"] = "C"
    lang_names[".cpp"] = "C++"
    lang_names[".h"] = "C Header"
    lang_names[".hpp"] = "C++ Header"
    lang_names[".html"] = "HTML"
    lang_names[".css"] = "CSS"
    lang_names[".go"] = "Go"
    lang_names[".rs"] = "Rust"
    lang_names[".php"] = "PHP"
    lang_names[".rb"] = "Ruby"
    lang_names[".swift"] = "Swift"
    lang_names[".kt"] = "Kotlin"
}
{
    # 提取行数和文件名
    lines = $1
    filename = $2
    
    # 获取文件扩展名
    split(filename, parts, ".")
    ext = "." parts[length(parts)]
    
    # 统计每种语言的行数
    if (ext in lang_names) {
        lang = lang_names[ext]
        lang_lines[lang] += lines
        lang_files[lang]++
    }
}
END {
    # 打印每种语言的统计
    for (lang in lang_lines) {
        printf "🏷️  %-15s: %6d lines (%3d files)\n", lang, lang_lines[lang], lang_files[lang]
    }
}' "$temp_file"

echo "----------------------------------------"

# 显示最大的文件
echo ""
echo "📈 TOP 5 LARGEST FILES:"
echo "----------------------------------------"
grep "^[[:space:]]*[0-9]\+[[:space:]]" "$temp_file" | sort -nr | head -5 | awk '{
    # 提取文件名（去掉前面的路径）
    split($2, parts, "/")
    filename = parts[length(parts)]
    printf "📄 %-25s: %6d lines\n", filename, $1
}'

# 清理临时文件
rm -f "$temp_file"

echo "================================================"
echo "🎯 TOTAL: $total_lines lines of code in $total_files files"
echo "================================================"