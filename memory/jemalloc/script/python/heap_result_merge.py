'''
Author: 胡益华

Date: 2023-02-19

Usage: python3 heap_result_merge.py [plat] [dir_path]

Description:
将内存报告文件按文件名中数字从小到大排列，并将内容逐一追加写入一个文件，方便后续处理
'''
import os
import re
import sys

def main(plat, file_dir):
    file_report = f"{file_dir}/{plat}_report.txt"
    if os.path.isfile(file_report):
        os.remove(file_report)

    # 匹配文件名中的数字部分，并按照数字从小到大排序
    file_list           = os.listdir(file_dir)
    sorted_file_names   = sorted(file_list, key=lambda x: int(re.findall(r'\d+', x)[0]))
    line_breaks         = os.linesep

    print("\033[0;34m\nFile merging\033[0m")
    with open(file_report, "a") as output_file:
        for file in sorted_file_names:
            file_path = os.path.join(file_dir, file)
            print(file_path)

            # 读取文件内容，并追加写入{plat}_report.txt
            with open(file_path, "r") as input_file:
                content = input_file.read()
            content += line_breaks
            output_file.write(content)

    print(f"\033[0;34m\nMerged file:\n{file_report}\033[0m")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"\033[0;31m\nUsage: python3 heap_result_merge.py [plat] [dir_path]\n\033[0m")
        sys.exit(-1)

    plat        = sys.argv[1]
    file_dir    = sys.argv[2]

    main(plat, file_dir)

    sys.exit(0)
