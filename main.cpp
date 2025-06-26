#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <windows.h>
#include <shellapi.h>
#include <setupapi.h>
#include <winioctl.h>
#include <cfgmgr32.h>
#include <thread>
#include <atomic>
#include <direct.h> // 添加目录操作支持
#include <conio.h>  // 修复_kbhit未定义
#include <errno.h>  // 修复EEXIST未定义
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")
bool isAdbAuthorized(); // 声明
// 声明/定义缺失的全局变量和函数
std::atomic<bool> isMirroringRunning(false); // 修复未定义

// 假设trim和isAdbEnabledOnDevice已实现，否则可简单声明如下：
std::string trim(const std::string& s); // 声明
bool isAdbEnabledOnDevice();            // 声明

// 新增：检查文件是否存在
bool fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

// 修改后的启动手表投屏功能
void startScreenMirroring() {
    if (!isAdbEnabledOnDevice()) {
        std::cout << "【错误】设备未开启ADB调试，无法投屏\n";
        return;
    }

    std::string scrcpyPath = ".\\scrcpy\\scrcpy.exe";
    if (!fileExists(scrcpyPath)) {
        std::cout << "【错误】scrcpy工具未找到，请先通过主菜单下载\n";
        return;
    }

    // 检查ADB设备连接（提前阻断scrcpy启动）
    if (system("adb devices | findstr \"\tdevice\" >nul") != 0) {
        std::cout << "【错误】未检测到ADB设备连接，请确保:\n";
        std::cout << "1. 手表已通过USB连接到电脑\n";
        std::cout << "2. 手表已开启USB调试模式\n";
        std::cout << "3. 电脑已安装ADB驱动程序\n";
        std::cout << "【提示】请检查上述步骤后重试！\n";
        return;
    }

    if (!isAdbAuthorized()) {
        std::cout << "【错误】设备未授权，请在手表上确认USB调试授权弹窗！\n";
        return;
    }

    std::cout << "【提示】正在启动手表投屏功能...\n";
    std::cout << "【提示】按任意键停止投屏并返回菜单\n";

    std::string scrcpyOptions = "--window-title \"小天才手表投屏\" --max-fps 30 --bit-rate 2M";
    std::string commandLine = scrcpyPath + " " + scrcpyOptions;

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    BOOL success = CreateProcessA(
        NULL,
        (LPSTR)commandLine.c_str(),
        NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi
    );
    if (!success) {
        std::cout << "【错误】启动scrcpy失败\n";
        return;
    }

    // 等待用户按键或scrcpy进程退出
    bool stopped = false;
    while (true) {
        DWORD waitResult = WaitForSingleObject(pi.hProcess, 100);
        if (waitResult == WAIT_OBJECT_0) {
            // scrcpy进程已退出
            break;
        }
        if (_kbhit()) {
            // 用户按键，杀掉scrcpy进程
            system("taskkill /im scrcpy.exe /f >nul 2>&1");
            stopped = true;
            break;
        }
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (stopped) {
        std::cout << "\n【提示】已停止投屏\n";
    } else {
        std::cout << "\n【提示】投屏已结束\n";
    }
}

// 封装彩色输出函数
void printColorText(const std::string& text, WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes = 0;
    if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo)) {
        saved_attributes = consoleInfo.wAttributes;
        SetConsoleTextAttribute(hConsole, color);
    }
    std::cout << text << std::flush;
    if (saved_attributes)
        SetConsoleTextAttribute(hConsole, saved_attributes);
}

// 修改后的控制台菜单
void showConsoleMenu() {
    system("cls");
    // 使用彩色输出函数输出艺术字
    printColorText(
        R"(  ____                   ______                 ____              __ 
  / __ \____  ___  ____  / ____/___ ________  __/ __ \____  ____  / /_
 / / / / __ \/ _ \/ __ \/ __/ / __ `/ ___/ / / / /_/ / __ \/ __ \/ __/
 / /_/ / /_/ /  __/ / / / /___/ /_/ (__  ) /_/ / _, _/ /_/ / /_/ / /_  
\____/ .___/\___/_/ /_/_____/\__,_/____/\__, /_/ |_|\____/\____/\__/  
    /_/                                /____/                         
)" "\n", FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    printColorText("============== 菜单 ==============\n", FOREGROUND_INTENSITY | FOREGROUND_BLUE);
    std::cout << "         ";
    printColorText("ADB调试检测工具 (菜单模式)\n", FOREGROUND_INTENSITY | FOREGROUND_GREEN);
    printColorText("==================================\n", FOREGROUND_INTENSITY | FOREGROUND_BLUE);

    std::cout << "  1. 检测设备ADB状态\n";
    std::cout << "  2. 输入APK路径进行安装\n";
    std::cout << "  3. 手表投屏\n";
    std::cout << "  4. 下载投屏工具\n";
    std::cout << "  5. 关于我们\n";
    std::cout << "  6. 退出程序\n";
    printColorText("==================================\n", FOREGROUND_INTENSITY | FOREGROUND_BLUE);

    std::cout << "当前设备状态: ";
    if (isAdbEnabledOnDevice()) {
        printColorText("已开启ADB调试", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printColorText(" [success]\n", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    } else {
        printColorText("未开启ADB调试", FOREGROUND_RED | FOREGROUND_INTENSITY);
        printColorText(" [failed]\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
    }

    std::cout << "投屏工具: ";
    if (fileExists(".\\scrcpy\\scrcpy.exe")) {
        printColorText("已安装\n", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    } else {
        printColorText("未安装 (请选择4下载)\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
    }
    printColorText("==================================\n", FOREGROUND_INTENSITY | FOREGROUND_BLUE);
}

// 新增：下载并安装scrcpy
void downloadScrcpy() {
    // 创建scrcpy目录
    if (_mkdir("scrcpy") != 0 && errno != EEXIST) {
        std::cout << "【错误】无法创建scrcpy目录\n";
        return;
    }

    std::cout << "【提示】正在下载投屏工具 (scrcpy)...\n";
    std::cout << "【提示】这可能需要几分钟时间，请耐心等待...\n";

    // 下载scrcpy
    if (system("powershell -Command \"Invoke-WebRequest -Uri 'https://github.com/Genymobile/scrcpy/releases/download/v2.0/scrcpy-win64-v2.0.zip' -OutFile 'scrcpy.zip'\"") != 0) {
        std::cout << "【错误】下载失败，请检查网络连接\n";
        return;
    }
// 包含运行环境问题肯能会导致powershell指令执行异常 //
    std::cout << "【提示】正在解压文件...\n";

    // 修正：先解压到临时目录，再移动文件到scrcpy目录
    if (system("powershell -Command \"Expand-Archive -Path 'scrcpy.zip' -DestinationPath '.' -Force\"") != 0) {
        std::cout << "【错误】解压失败\n";
        return;
    }

    // 移动解压出来的文件到scrcpy目录
    system("move /Y scrcpy-win64-v2.0\\* scrcpy\\ >nul 2>&1");
    system("rmdir /S /Q scrcpy-win64-v2.0 >nul 2>&1");

    // 清理临时文件
    system("del /f scrcpy.zip >nul 2>&1");

    // 检查是否安装成功
    if (fileExists(".\\scrcpy\\scrcpy.exe")) {
        std::cout << "【成功】投屏工具已安装到 scrcpy 目录\n";
    } else {
        std::cout << "【错误】安装失败，请手动下载:\n";
        std::cout << "1. 访问 https://github.com/Genymobile/scrcpy/releases\n";
        std::cout << "2. 下载最新版 scrcpy-win64.zip\n";
        std::cout << "3. 解压到程序目录下的 scrcpy 文件夹中\n";
    }
}

// 修改后的菜单处理
bool processConsoleInput() {
    showConsoleMenu();
    std::cout << "请选择操作 (1-6): ";
    std::string choice;
    std::getline(std::cin, choice);
    choice = trim(choice);

    if (choice == "1") {
        // ... 保持不变 ...
        return true;
    }
    else if (choice == "2") {
        // ... 保持不变 ...
        return true;
    }
    else if (choice == "3") {
        startScreenMirroring();
        std::cout << "按任意键返回菜单...\n";
        _getch(); // 只在投屏结束后暂停一次
        return true;
    }
    else if (choice == "4") { // 新增：下载投屏工具
        downloadScrcpy();
        system("pause");
        return true;
    }
    else if (choice == "5") { // 关于我们
        std::cout << "【关于我们】Qmoo社区,主群:1050033206;CK:623908523;CEO:22518482081\n";
        system("pause");
        return true;
    }
    else if (choice == "6") { // 退出程序
        return false;
    }
    else {
        std::cout << "【错误】无效选择，请输入1-6\n";
        system("pause");
        return true;
    }
}

// 实现trim函数
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}


bool isAdbEnabledOnDevice() {
    int result = system("adb devices | findstr \"\tdevice\" >nul");
    return result == 0;
}

bool isAdbAuthorized() {
    // 检查adb devices输出是否有 unauthorized
    int result = system("adb devices | findstr \"unauthorized\" >nul");
    return result != 0;
}

int main() {
    // 先切换编码（如有异常可注释掉此行）
    system("chcp 65001 >nul");

    // 不再关闭同步，保持C++流和C流同步
    // std::ios::sync_with_stdio(false);
    // std::cin.tie(nullptr);

    // 不再在main中输出艺术字，交由showConsoleMenu统一处理

    while (processConsoleInput()) {

    }
    return 0;
}
