# 串口调试指南（Windows 版）｜在 Windows 上读到板子的 INFO 打印

macOS 版见 `串口调试指南.md`。两份内容**不能互相照搬**——macOS 那份的核心坑（`stty -f` 丢设置、`cu.*` 沿用残留波特率）是 Unix 特有的，Windows 不存在；反过来 Windows 有自己的一套问题（端口号会变、独占访问、驱动）。

> ✅ **[部分已验证 @ Windows]** `uart_read_win.py` 已在本机 Windows (Python 3.14 + pyserial) 实跑：`--help`、`-l`（无设备时输出 `(none)`）、打不开端口时列出可用端口并退出码 1 —— 这三条**不需要接板子**的路径都已跑通，`py -m py_compile` 通过。
> 
> ⚠ **仍需真机确认**：抓取正文（`-p COMx` 读到 INFO）、波特率错误告警、`--raw`、以及第 3/4/6/7 章讲的端口变号 / 独占 / 驱动 / 乱码，都要**插上板子**才能验。跑通后把实测输出补进第 2 章。
>
> 尚未在 Windows 实机验证的还有 `scripts/flash.ps1` 和 `Mac迁移配置指南.md` 5.4 节的 Windows 部分。

---

## 第0章 三件事先确认

| 项 | Windows 上的形态 | 怎么查 |
| --- | --- | --- |
| 端口号 | `COM3` / `COM7` …（**会变**，见第 3 章） | `py -m serial.tools.list_ports -v` |
| 波特率 | **115200** 8N1 | 见下表，两个工程不一样 |
| 谁在打印 | 云台 `INFO()` 宏 | 代码里搜 `INFO(` |

**波特率和串口号取决于工程，别混**（这一点两个平台一致）：

| 工程 | INFO 走哪个 UART | 波特率 |
| --- | --- | --- |
| Gimbal（云台，英雄） | **huart1** | 115200 |
| Chassis（底盘） | **huart2** | 115200 |

---

## 第1章 装 pyserial

Windows 上没有 macOS 那套 `termios` / `stty` 机制，标准做法就是 pyserial。**它比 macOS 那边省事得多**，因为不用绕 `stty -f` 的坑——`serial.Serial(port, baudrate)` 打开时就把波特率设死了。

```powershell
py -m pip install pyserial
```

注意包名是 `pyserial`，但 `import` 时写 `serial`。装 `pip install serial` 会装到一个完全无关的包上，这是常见误装。

验证：

```powershell
py -m serial.tools.list_ports -v
```

会列出所有 COM 口及其 USB 描述信息。**没有输出说明驱动没装好**，见第 6 章。

---

## 第2章 最快一条命令

仓库里放了 Windows 版脚本：

```powershell
cd D:\Newcode\Gimbal_26hero_CH010_20260725
py scripts\uart_read_win.py -p COM3
```

> 底盘工程同理：`cd D:\Newcode\Chassisl_26_SHANGTAIJIE`，脚本已放在两个工程各自的 `scripts\` 下。

**注意必须带 `-p`**，脚本默认值是 `COM3` 只是个占位——你的端口号几乎肯定不是它，先用 `-l` 查：

```powershell
py scripts\uart_read_win.py -l
```

**预期输出**（格式如此，端口号和描述以你的机器为准）：

```
  COM3       USB-SERIAL CH340 (COM3)
  COM7       STMicroelectronics STLink Virtual COM Port (COM7)
```

抓取的预期输出，与 macOS 版格式一致：

```
=== 1656 bytes in 4.0s ===
printable: 1656/1656
--- top frames ---
   276 x b'okkk'
     1 x b''
```

常用参数（和 macOS 版完全相同，刻意保持一致）：

```powershell
py scripts\uart_read_win.py -p COM3 -s 10       # 抓 10 秒（默认 4）
py scripts\uart_read_win.py -p COM3 -b 9600     # 换波特率
py scripts\uart_read_win.py -p COM3 --raw       # 只要正文不要统计
py scripts\uart_read_win.py -l                  # 列出所有 COM 口
```

波特率设错时脚本会主动告警：

```
printable: 0/146
WARNING: mostly non-printable -> wrong baud rate, see 串口调试指南_Windows.md
```

打不开端口时会直接把可用端口列出来，不用你再去猜：

```
cannot open COM3: could not open port 'COM3': ...

available ports:
  COM7       USB-SERIAL CH340 (COM7)
```

---

## 第3章 Windows 头号坑：COM 端口号会变

macOS 的设备名带序列号（`cu.usbmodemATK_202109141`），基本固定。**Windows 的 `COMn` 是注册表按插入顺序分配的**，换 USB 口、换转接板、甚至重启后都可能变号。

所以：

- **不要把 COM 号写死进脚本或文档**。每次先 `-l` 查。
- 脚本里 `DEFAULT_PORT = "COM3"` 只是占位，别当成你的端口。
- 换了 USB 口发现读不到数据，第一反应是**重新查端口号**，不是怀疑板子。

想固定的话，设备管理器 → 端口 → 右键属性 → 端口设置 → 高级 → 手动指定 COM 号。这是一次性配置，不在本文范围内。

另一个坑：**`COM10` 及以上在某些老 API 里要写成 `\\.\COM10`**。pyserial 内部已经处理了这一点，直接写 `COM10` 即可；但如果你用别的工具遇到"COM10 打不开而 COM9 正常"，就是这个原因。

---

## 第4章 端口是独占的

**同一时刻只能一个程序打开一个 COM 口**（这点比 macOS 更严格，Windows 直接返回 `Access is denied`）。

典型报错：

```
could not open port 'COM3': PermissionError(13, '拒绝访问。', None, 5)
```

**这几乎总是"另一个程序占着"，不是权限不够，别去用管理员权限重试。** 常见占用者：

- 开着的 PuTTY / 串口助手 / Arduino IDE 串口监视器
- VSCode 的 Serial Monitor 插件
- 正点原子 XCOM、mcuisp、STM32CubeProgrammer
- 上次没正常退出的残留进程

查是谁占着（需要 [Handle](https://learn.microsoft.com/sysinternals/downloads/handle) 或 Process Explorer，Windows 没有 `lsof`）：

```powershell
handle.exe COM3
```

没装 Sysinternals 的话，最快的办法就是把上面那几个程序全关掉再试。

---

## 第5章 其他查看方式

### PuTTY（交互式实时看，推荐）

Windows 上没有 `screen`。PuTTY 是最省事的图形方案：

1. Connection type 选 **Serial**
2. Serial line 填 `COM3`（你的实际端口）
3. Speed 填 `115200`
4. Open

要存日志：Session → Logging → 选 "All session output"，指定文件路径。

**用完记得关窗口**，否则端口一直被占着（见第 4 章）。

### PowerShell 原生（不装任何东西）

PowerShell 能直接用 .NET 的 `SerialPort` 类，适合手边什么都没有的应急情况：

```powershell
$port = New-Object System.IO.Ports.SerialPort COM3,115200,None,8,One
$port.ReadTimeout = 500
$port.Open()
Start-Sleep -Milliseconds 300
$port.DiscardInBuffer()          # 丢弃残留字节
$sw = [Diagnostics.Stopwatch]::StartNew()
while ($sw.Elapsed.TotalSeconds -lt 4) {
    try { $port.ReadExisting() } catch [TimeoutException] { }
}
$port.Close()                    # 必须关,否则端口一直被占
```

`$port.Close()` 漏了的话，这个 PowerShell 会话不结束，端口就一直占着。用 `try/finally` 包起来更稳。

**不要用 `mode COM3 BAUD=115200`**。`mode` 命令能设参数，但和 macOS 的 `stty -f` 一个毛病——它开完端口就关，设置未必保留到下一个程序打开时。而且 `mode` 没法读数据，还得配合别的工具。直接用 pyserial 或 PuTTY 一步到位。

### 不要用 `type COM3`

`type \\.\COM3` 这类写法在现代 Windows 上不可靠：没设波特率（沿用驱动默认值，通常是 9600）、没有终止条件、编码会被 cmd 二次处理。等价于 macOS 那边的裸 `cat`，同样别用。

---

## 第6章 驱动问题

macOS 上 USB-TTL 基本免驱，**Windows 往往要装驱动**。这是两个平台最实际的差异之一。

`py -m serial.tools.list_ports -v` 没有任何输出，或设备管理器里出现带黄色感叹号的"未知设备"，就是驱动问题：

| 芯片 | 常见于 | 驱动 |
| --- | --- | --- |
| CH340 / CH341 | 正点原子 USB-TTL、多数国产板 | WCH 官网 CH341SER |
| CP2102 | 部分开发板 | Silicon Labs CP210x VCP |
| FT232 | FTDI 转接板 | FTDI VCP |

设备管理器 → 端口(COM 和 LPT) 下能看到带 COM 号的条目，才算装好了。

---

## 第7章 抓到数据但内容不对

先分清是"读取链路问题"还是"固件问题"。**能整齐分帧就说明链路没问题**，剩下的看固件。这一章两个平台完全一致。

| 现象 | 说明 |
| --- | --- |
| 整齐分帧但内容不是你以为的那样 | **板子里的固件不是你以为的那版**。重新编译烧写 |
| 完全无数据（0 字节） | 板子没上电 / 没跑到 `INFO` 那行 / TX 线没接 / 端口被占用 |
| 满屏 `FC` `F8` `FF` 之类 | 波特率不对（确认工程用 huart1 还是 huart2） |
| 有数据但帧率异常低 | `INFO` 所在循环里有阻塞或 `HAL_Delay` 太长 |

macOS 侧实测过一个例子：抓到 `okkk` 而不是预期的 `ok`，原因是磁盘上的 ELF 已被改过并烧进去了。**串口抓到什么就是什么，它比你对代码的记忆可靠。**

---

## 附录A：一次完整的排查流程

```powershell
cd D:\Newcode\Gimbal_26hero_CH010_20260725

# 1) 端口在不在(每次都查,COM 号会变)
py scripts\uart_read_win.py -l
#    无输出 → 驱动问题,见第6章,后面都不用看了

# 2) 正式抓取(端口号换成上一步查到的)
py scripts\uart_read_win.py -p COM3 -s 5
#    printable 接近 100% + 分帧整齐 → 链路 OK,内容对不上就是固件版本问题(第7章)
#    WARNING 非可打印      → 波特率不对,确认 huart1/huart2(第0章)
#    拒绝访问 / Access denied → 端口被占用(第4章)
#    0 字节                → 板子没上电 / 没跑到 INFO / TX 未接
```

---

## 附录B：与 macOS 版的差异对照

| 项 | macOS | Windows |
| --- | --- | --- |
| 脚本 | `scripts/uart_read.py`（标准库 `termios`，**免依赖**） | `scripts/uart_read_win.py`（**需 pyserial**） |
| 端口名 | `/dev/cu.usbmodemATK_202109141`（含序列号，稳定） | `COM3`（**按插入顺序分配，会变**） |
| 设波特率 | 麻烦：`stty -f` 会丢设置，必须在已打开的 fd 上设 termios | 简单：`serial.Serial(port, baudrate)` 打开即生效 |
| 残留波特率 | **会沿用上次的**，不显式设就乱码 | 不存在此问题 |
| 交互式查看 | `screen`（自带，但版本古老、有坑） | PuTTY（**需自行安装**） |
| 端口占用 | `lsof /dev/cu.*` | `handle.exe COM3`（需 Sysinternals，**无 lsof**） |
| 驱动 | 基本免驱 | **常需装 CH340 / CP210x 驱动** |
| 别用 | `timeout 3 cat /dev/cu.*`（无 `timeout` 命令） | `type \\.\COM3`、`mode COM3 BAUD=...` |

**一句话总结**：Windows 侧真正的坑是**端口号会变**和**端口独占**；macOS 侧真正的坑是**波特率不会自动设对**。两边不是一回事。

---

## 相关文档

- macOS 版：`串口调试指南.md`
- 环境搭建、烧录脚本：`Mac迁移配置指南.md`
- 抓取脚本本体：`scripts/uart_read_win.py`（Windows）、`scripts/uart_read.py`（macOS）
