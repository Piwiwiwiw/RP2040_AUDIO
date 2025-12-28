#include <SimpleFSTerminal.h>


AudioPlayer* pAudio;
String SimpleFSTerminal::currentDir = "/";


String getSDType(uint8_t type) {
    switch (type) {
        case 1: return "SD1";
        case 2: return "SD2";
        case 3: return "SDHC";
        default: return "Unknown";
    }
}


String getFatType(uint8_t fatType) {
    switch (fatType) {
        case 1: return "FAT12";
        case 2: return "FAT16";
        case 3: return "FAT32";
        default: return "Unknown";
    }
}


uint64_t getUsedSpace() {
    uint64_t totalSpace = SD.size64();
    uint64_t freeSpace = SD.totalBlocks() * SD.blockSize() - SD.totalClusters() * SD.clusterSize();
    return totalSpace - freeSpace;
}


String SimpleFSTerminal::getFullPath(const String& path) {
    if (path.startsWith("/")) {
        return path;
    } else {
        return currentDir + (currentDir.endsWith("/") ? "" : "/") + path;
    }
}


void SimpleFSTerminal::begin() {
    delay(100);
    if (!SD.begin(SD_CLK_PIN, SD_CMD_PIN, SD_DATA_PIN)) {
        delay(100);
        Serial.println("文件系统启动失败");
        // SD.format();
        // SD.begin();
    }
    Serial.println("文件系统就绪!");
    Serial.println("输入 'help' 查看命令");
    Serial.print(currentDir + "> ");
    
    static AudioPlayer AudioOutput(pio1, 5, 3, 4);
    pAudio = &AudioOutput;
}


void SimpleFSTerminal::processSerial() {
    static String input = "";
    
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (input.length() > 0) {
                Serial.println();
                
                // 解析命令
                int space = input.indexOf(' ');
                String cmd = input;
                String args = "";
                
                if (space != -1) {
                    cmd = input.substring(0, space);
                    args = input.substring(space + 1);
                }
                
                executeCommand(cmd, args);
                
                Serial.print(currentDir + "> ");
                input = "";
            }
        } else if (c == 8 || c == 127) { // Backspace/Delete
            if (input.length() > 0) {
                input.remove(input.length() - 1);
                Serial.print("\b \b");
            }
        } else if (c >= 32) {
            input += c;
            Serial.print(c);
        }
    }
}


void SimpleFSTerminal::executeCommand(const String& cmd, const String& args) {
    if (cmd == "ls") ls(args);
    else if (cmd == "cd") cd(args);
    else if (cmd == "mkdir") mkdir(args);
    else if (cmd == "rm") rm(args);
    else if (cmd == "cat") cat(args);
    else if (cmd == "play") play(args);
    else if (cmd == "write") {
        int space = args.indexOf(' ');
        if (space != -1) {
            String path = args.substring(0, space);
            String content = args.substring(space + 1);
            writeFile(path, content);
        } else {
            Serial.println("用法: write <文件> <内容>");
        }
    }
    else if (cmd == "help") showHelp();
    else if (cmd == "info") showInfo();

    else if (cmd == "clear" || cmd == "cls") Serial.print("\033[2J\033[H");
    else Serial.println("未知命令");
}


void SimpleFSTerminal::ls(const String& path) {
    String fullPath = getFullPath(path);
    File dir = SD.open(fullPath, "r");
    
    if (!dir || !dir.isDirectory()) {
        Serial.println("目录不存在");
        return;
    }
    
    File file = dir.openNextFile();
    while (file) {
        Serial.print(file.name());
        if (file.isDirectory()) {
            Serial.println("/");
        } else {
            Serial.print(" ");
            Serial.println(file.size());
        }
        file = dir.openNextFile();
    }
    dir.close();
}


void SimpleFSTerminal::cd(const String& path) {
    if (path == "..") {
        int lastSlash = currentDir.lastIndexOf('/');
        if (lastSlash > 0) {
            currentDir = currentDir.substring(0, lastSlash);
        }
    } else if (path == "/") {
        currentDir = "/";
    } else if (path.length() > 0) {
        String newPath = getFullPath(path);
        File dir = SD.open(newPath, "r");
        if (dir && dir.isDirectory()) {
            currentDir = newPath;
        } else {
            Serial.println("目录不存在");
        }
        if (dir) dir.close();
    }
    Serial.println("当前目录: " + currentDir);
}


void SimpleFSTerminal::mkdir(const String& path) {
    String fullPath = getFullPath(path);
    if (SD.mkdir(fullPath)) {
        Serial.println("目录创建成功");
    } else {
        Serial.println("目录创建失败");
    }
}


void SimpleFSTerminal::rm(const String& path) {
    String fullPath = getFullPath(path);
    if (SD.remove(fullPath)) {
        Serial.println("删除成功");
    } else {
        Serial.println("删除失败");
    }
}


void SimpleFSTerminal::cat(const String& path) {
    String fullPath = getFullPath(path);
    File file = SD.open(fullPath, "r");
    
    if (!file || file.isDirectory()) {
        Serial.println("文件不存在");
        return;
    }
    
    while (file.available()) {
        Serial.write(file.read());
    }
    Serial.println();
    file.close();
}



void SimpleFSTerminal::play(const String& path){

    Serial.printf("开始播放\n");
    String fullPath = getFullPath(path);
    int dotIndex = path.lastIndexOf('.');
    format fmt;
    if(dotIndex != -1) {
        String extension = path.substring(dotIndex + 1);

        // 根据后缀名设置枚举值
        if(extension.equalsIgnoreCase("mp3")) {
            fmt = mp3;
        } else if(extension.equalsIgnoreCase("wav")) {
            fmt = wav;
        } else if(extension.equalsIgnoreCase("flac")) {
            fmt = flac;
        } else {
            Serial.printf("未知的文件格式: %s\n", extension.c_str());
            return;
        }
    }
    File audioFile = SD.open(fullPath, "r");
    if(!audioFile)
      Serial.printf("无指定文件\n");
    else{
      pAudio->play(&audioFile, fmt);
    }
}


void SimpleFSTerminal::writeFile(const String& path, const String& content) {
    String fullPath = getFullPath(path);
    File file = SD.open(fullPath, "w");
    
    if (file) {
        file.print(content);
        file.close();
        Serial.println("写入成功");
    } else {
        Serial.println("写入失败");
    }
}


void SimpleFSTerminal::showHelp() {
    Serial.println("可用命令:");
    Serial.println("  ls [路径]    - 列出文件");
    Serial.println("  cd 路径      - 切换目录");
    Serial.println("  mkdir 路径   - 创建目录");
    Serial.println("  rm 文件      - 删除文件");
    Serial.println("  cat 文件     - 显示文件内容");
    Serial.println("  write 文件 内容 - 写入文件");
    Serial.println("  info        - 系统信息");
    Serial.println("  help        - 显示帮助");
    Serial.println("  clear/cls   - 清屏");
}


void SimpleFSTerminal::showInfo() {
    // 获取并打印 SD 卡的信息
        Serial.print("SD Type: ");
        Serial.println(getSDType(SD.type()));
    
        Serial.print("SD FAT Type: ");
        Serial.println(getFatType(SD.fatType()));
    
        Serial.print("Card Size (Bytes): ");
        Serial.println(SD.size64());
    
        Serial.print("Used Space (Bytes): ");
        Serial.println(getUsedSpace());
}



