'''
Author: 胡益华

Creation Date: 2024-08-19

Updated Date: 2024-08-19

Usage: python3 je_analyse.py

Description: 
此脚本用于分析开源工具 jemalloc 生成的 heap 文件

Notice:
(1) jemalloc 官方提供了 jeprof 脚本，此脚本需要在环境变量中
(2) 运行脚本前需配置 ../config/je_config.cfg 中的参数，详见配置文件中的注释
'''

import os
import sys
import configparser
import subprocess
import threading
import queue
import glob
import info
from pathlib import Path
from inotify_simple import INotify, flags

global g_script_dir
global g_project_dir
g_script_dir    = os.path.dirname(os.path.abspath(__file__))
g_project_dir   = f"{os.path.dirname(os.path.abspath(__file__))}/../.."

class JeAnalysisErrorCode:
    ERR_INVALID_INPUT_PARAMMETER    = -10
    ERR_INVALID_PLATFORM            = -20
    ERR_INVALID_HEAP_FILE           = -30

class HeapAnalyzer:
    def __init__(self, config_file):
        self.config                     = self.load_config(config_file)
        self.plat                       = self.config.get('Default', 'plat')
        self.tool                       = self.config.get('Default', 'tool') or ""

        self.heap_dir                   = Path(self.config.get('DependentFilePath', 'heap_dir'))
        self.binary_file                = self.config.get('DependentFilePath', 'binary_file')
        self.share_library              = self.config.get('DependentFilePath', 'share_library') or "./"

        self.dot_enable                 = self.config.getboolean('ResultType', 'dot_enable', fallback=True)
        self.txt_enable                 = self.config.getboolean('ResultType', 'txt_enable', fallback=True)

        self.real_anlys_enable          = self.config.getboolean('RealTimeNatureOfAnalysis', 'real_anlys_enable', fallback=True)
        self.real_anlys_thread_count    = min(max(1, self.config.getint('RealTimeNatureOfAnalysis', 'real_anlys_thread_count', fallback=2)), 8)

        # 分析结果目录
        self.anlys_result_dot_dir   = f"{g_project_dir}/anlys_result/{self.plat}/dot"
        self.anlys_result_txt_dir   = f"{g_project_dir}/anlys_result/{self.plat}/txt"
        os.makedirs(self.anlys_result_dot_dir, mode=0o777, exist_ok=True)
        os.makedirs(self.anlys_result_txt_dir, mode=0o777, exist_ok=True)

        self.anlys_cmd = ""

    def load_config(self, config_file):
        config = configparser.ConfigParser()
        config.read(config_file)
        return config

    def get_pid_of_heap(self, heap_file):
        heap_file_rev   = heap_file[::-1]
        pid_rev         = heap_file_rev.split('.')[3]
        return pid_rev[::-1]
    
    def get_arbitrary_pid_of_heap(self):
        heap_list   = glob.glob(os.path.join(self.heap_dir, '*.heap'))
        if 0 < len(heap_list):
            heap_file = heap_list[0]
            return self.get_pid_of_heap(heap_file)

        info.info_red(f"No heap files found in: {self.heap_dir}")
        info.error_exit_print(JeAnalysisErrorCode.ERR_INVALID_HEAP_FILE)

    def plat_anlys_cmd(self, heap_file):
        tool = f"--tools={self.tool}" if self.tool else ""

        return f"jeprof --show_bytes \
            {self.binary_file} \
            {heap_file} \
            --lib_prefix={self.share_library} \
            {tool}"

    def heap_process(self, enable, result_type, result_path):
        if enable:
            def file_write():
                with open(result_path, "w") as f:
                    subprocess.run(self.anlys_cmd.split() + [result_type], stdout=f)

            if os.path.exists(result_path):
                with open(result_path, 'r') as file:
                    lines = file.readlines()
                    if len(lines):
                        for line in lines:
                            if 0 == len(line.strip()):
                                file_write()

                            break
                    else:
                        file_write()
            else:
                file_write()

    def result_analyse(self, heap_file):
        heap_file_rev   = heap_file[::-1]
        dump_count_rev  = heap_file_rev.split('.')[2]
        dump_count      = dump_count_rev[::-1]
        dot_path        = f"{self.anlys_result_dot_dir}/report_{dump_count}.dot"
        txt_path        = f"{self.anlys_result_txt_dir}/report_{dump_count}.txt"
        self.anlys_cmd  = self.plat_anlys_cmd(heap_file)

        self.heap_process(self.dot_enable, "--dot", dot_path)
        self.heap_process(self.txt_enable, "--text", txt_path)

    # 实时监视新生成的 .heap 文件
    def real_monitor_heap(self, q):
        inotify = INotify()
        watch_flags = flags.CREATE
        inotify.add_watch(self.heap_dir, watch_flags)

        while True:
            # 阻塞式监视
            for event in inotify.read():
                for flag in flags.from_mask(event.mask):
                    if flag is flags.CREATE:
                        new_file = os.path.join(self.heap_dir, event.name)
                        info.info_blue(f"New heap file: {os.path.basename(new_file)}")
                        q.put(new_file)

    def real_analyse_heap(self, q):
        while True:
            heap_file = q.get()
            if heap_file:
                self.result_analyse(f"{Path(heap_file)}")

    def real_analyse(self):
        # Queue 内默认加锁
        q = queue.Queue()

        monitor_thread = threading.Thread(target=self.real_monitor_heap, args=(q,))
        monitor_thread.daemon = True
        monitor_thread.start()

        for _ in range(self.real_anlys_thread_count):
            worker_thread = threading.Thread(target=self.real_analyse_heap, args=(q,))
            worker_thread.daemon = True
            worker_thread.start()

        monitor_thread.join()

    def run(self):
        pid = self.get_arbitrary_pid_of_heap()

        for heap_file in self.heap_dir.glob("*.heap"):
            pid_tmp = self.get_pid_of_heap(f"{heap_file}")
            if pid_tmp != pid:
                info.info_red("Heap files are generated by different processes")
                info.info_red(f"pid1: {pid}; pid2: {pid_tmp}")
                info.error_exit_print(JeAnalysisErrorCode.ERR_INVALID_HEAP_FILE)

            self.result_analyse(f"{heap_file}")

        if self.real_anlys_enable:
            self.real_analyse()
        else:
            if self.txt_enable:
                txt_merge_script = f"{g_project_dir}/script/python/heap_result_merge.py"
                args = [self.plat, self.anlys_result_txt_dir]
                subprocess.run(["python3", txt_merge_script] + args)

if __name__ == "__main__":
    if len(sys.argv) != 1:
        info.info_red("Usage: python3 je_analyse.py")
        info.error_exit_print(JeAnalysisErrorCode.ERR_INVALID_INPUT_PARAMMETER)

    config_file = f"{g_project_dir}/script/config/je_config.cfg"
    analyzer = HeapAnalyzer(config_file)
    analyzer.run()

    sys.exit(0)
