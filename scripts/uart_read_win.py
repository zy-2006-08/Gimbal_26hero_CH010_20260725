#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
uart_read_win.py - Windows 串口抓取脚本 (pyserial)

在 Windows 上读板子的 INFO 打印。与 macOS 版 scripts/uart_read.py 刻意保持
相同的命令行接口和输出格式。用法见 串口调试指南_Windows.md。

    py scripts\\uart_read_win.py -l                 # 列出所有 COM 口
    py scripts\\uart_read_win.py -p COM3            # 抓 4 秒 (默认 115200 8N1)
    py scripts\\uart_read_win.py -p COM3 -s 10      # 抓 10 秒
    py scripts\\uart_read_win.py -p COM3 -b 9600    # 换波特率
    py scripts\\uart_read_win.py -p COM3 --raw      # 只要正文不要统计

依赖: pyserial (py -m pip install pyserial)
"""

import argparse
import sys
import time
from collections import Counter

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    sys.stderr.write(
        "缺少 pyserial。安装: py -m pip install pyserial\n"
        "注意包名是 pyserial, import 时写 serial;别装成 `pip install serial`。\n"
    )
    sys.exit(1)

DEFAULT_PORT = "COM3"      # 占位, 你的端口号几乎肯定不是它, 先用 -l 查
DEFAULT_BAUD = 115200
DEFAULT_SECONDS = 4.0

# 波特率错误判定阈值: 可打印字节占比低于此值就告警
PRINTABLE_WARN_RATIO = 0.5
# top frames 显示条数
TOP_N = 10


def is_printable(b):
    """ASCII 可打印 (0x20-0x7E) 或常见空白 (\\t \\n \\r)。"""
    return b in (9, 10, 13) or 0x20 <= b <= 0x7E


def list_ports():
    """返回 (device, description) 列表。"""
    return [(p.device, p.description) for p in serial.tools.list_ports.comports()]


def print_ports(ports, stream=sys.stdout):
    """按 macOS 版对齐格式打印端口清单。"""
    if not ports:
        stream.write("  (none)\n")
        return
    for device, desc in ports:
        stream.write("  {:<9}  {}\n".format(device, desc))


def summarize(data, elapsed):
    """打印字节数 / 可打印占比 / top frames, 并在疑似波特率错误时告警。"""
    n = len(data)
    printable = sum(1 for b in data if is_printable(b))
    print("=== {} bytes in {:.1f}s ===".format(n, elapsed))
    print("printable: {}/{}".format(printable, n))

    if n:
        # 分帧: 按 \n 切, 去掉行尾 \r。保留末尾空帧 (与 macOS 版一致)。
        frames = [f.rstrip(b"\r") for f in data.split(b"\n")]
        counts = Counter(frames)
        print("--- top frames ---")
        for frame, cnt in counts.most_common(TOP_N):
            print("{:6d} x {!r}".format(cnt, frame))

    if n and printable / n < PRINTABLE_WARN_RATIO:
        print(
            "WARNING: mostly non-printable -> wrong baud rate, "
            "see 串口调试指南_Windows.md"
        )


def capture(port, baud, seconds, raw):
    """打开端口抓取 seconds 秒。打不开则列出可用端口后退出。"""
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except serial.SerialException as exc:
        print("cannot open {}: {}".format(port, exc))
        print()
        print("available ports:")
        print_ports(list_ports())
        sys.exit(1)

    chunks = []
    start = time.monotonic()
    try:
        # 丢弃打开瞬间的残留字节, 只统计这次窗口内的数据
        ser.reset_input_buffer()
        while time.monotonic() - start < seconds:
            waiting = ser.in_waiting
            data = ser.read(waiting if waiting else 1)
            if not data:
                continue
            if raw:
                sys.stdout.write(data.decode("utf-8", errors="replace"))
                sys.stdout.flush()
            else:
                chunks.append(data)
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()  # 必须关, 否则端口一直被占 (见指南第4章)

    elapsed = time.monotonic() - start
    if raw:
        sys.stdout.write("\n")
    else:
        summarize(b"".join(chunks), elapsed)


def main():
    parser = argparse.ArgumentParser(
        description="Windows serial reader (pyserial). See 串口调试指南_Windows.md"
    )
    parser.add_argument("-p", "--port", default=DEFAULT_PORT,
                        help="COM port, e.g. COM3 (default: %(default)s, placeholder)")
    parser.add_argument("-b", "--baud", type=int, default=DEFAULT_BAUD,
                        help="baud rate (default: %(default)s)")
    parser.add_argument("-s", "--seconds", type=float, default=DEFAULT_SECONDS,
                        help="capture duration in seconds (default: %(default)s)")
    parser.add_argument("-l", "--list", action="store_true",
                        help="list available COM ports and exit")
    parser.add_argument("--raw", action="store_true",
                        help="print raw body only, no stats")
    args = parser.parse_args()

    if args.list:
        print_ports(list_ports())
        return

    capture(args.port, args.baud, args.seconds, args.raw)


if __name__ == "__main__":
    main()
