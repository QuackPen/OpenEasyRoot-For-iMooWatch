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
    
    // 检查scrcpy是否已安装
    std::string scrcpyPath = ".\\scrcpy\\scrcpy.exe";
    if (!fileExists(scrcpyPath)) {
        std::cout << "【错误】scrcpy工具未找到，请先通过主菜单下载\n";
        return;
    }
    
    // 检查ADB设备连接
    if (system("adb devices | findstr \"\tdevice\" >nul") != 0) {
        std::cout << "【错误】未检测到ADB设备连接，请确保:\n";
        std::cout << "1. 手表已通过USB连接到电脑\n";
        std::cout << "2. 手表已开启USB调试模式\n";
        std::cout << "3. 电脑已安装ADB驱动程序\n";
        return;
    }
    
    std::cout << "【提示】正在启动手表投屏功能...\n";
    std::cout << "【提示】按任意键停止投屏并返回菜单\n";
    
    // 设置投屏参数
    std::string scrcpyOptions = "--window-title \"小天才手表投屏\" --max-fps 30 --bit-rate 2M";
    
    // 在后台线程中运行投屏
    isMirroringRunning = true;
    std::thread mirrorThread([scrcpyPath, scrcpyOptions]() {
        std::string command = "\"" + scrcpyPath + "\" " + scrcpyOptions;
        system(command.c_str());
        isMirroringRunning = false;
    });
    mirrorThread.detach();
    
    // 等待用户按键停止
    while (isMirroringRunning) {
        if (_kbhit()) {
            // 停止scrcpy进程
            system("taskkill /im scrcpy.exe /f >nul 2>&1");
            std::cout << "\n【提示】已停止投屏\n";
            break;
        }
        Sleep(100);
    }
    isMirroringRunning = false;
}

// 修改后的控制台菜单
void showConsoleMenu() {
    system("cls");
    std::cout << "================================\n";
    std::cout << "        ADB调试检测工具 (菜单模式)\n";
    std::cout << "================================\n";
    std::cout << "1. 检测设备ADB状态\n";
    std::cout << "2. 输入APK路径进行安装\n";
    std::cout << "3. 手表投屏\n";
    std::cout << "4. 下载投屏工具\n";  // 新增下载选项
    std::cout << "5. 关于我们\n";
    std::cout << "6. 退出程序\n";
    std::cout << "================================\n";
    std::cout << "当前设备状态: ";
    
    if (isAdbEnabledOnDevice()) {
        std::cout << "已开启ADB调试\n";
    } else {
        std::cout << "未开启ADB调试\n";
    }
    
    // 显示scrcpy安装状态
    if (fileExists(".\\scrcpy\\scrcpy.exe")) {
        std::cout << "投屏工具: 已安装\n";
    } else {
        std::cout << "投屏工具: 未安装 (请选择4下载)\n";
    }
    std::cout << "================================\n";
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
    
    std::cout << "【提示】正在解压文件...\n";
    
    // 解压到scrcpy目录
    if (system("powershell -Command \"Expand-Archive -Path 'scrcpy.zip' -DestinationPath 'scrcpy' -Force\"") != 0) {
        std::cout << "【错误】解压失败\n";
        return;
    }
    
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
        system("pause");
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

// 实现isAdbEnabledOnDevice函数
bool isAdbEnabledOnDevice() {
    // 检查adb devices输出是否有device
    int result = system("adb devices | findstr \"\tdevice\" >nul");
    return result == 0;
}

int main() {
    // 设置控制台为UTF-8编码，防止中文乱码
    system("chcp 65001 >nul");
    // 不需要在main函数中检查scrcpy，改为在菜单中显示状态
    // ... 窗口创建代码保持不变 ...
    while (processConsoleInput()) {
        // 循环直到用户选择退出
    }
    return 0;
}
