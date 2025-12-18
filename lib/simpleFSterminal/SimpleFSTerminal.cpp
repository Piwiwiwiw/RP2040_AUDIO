#include <SimpleFSTerminal.h>

I2S i2s(OUTPUT);
File audioFile;
uint8_t buffer[16384];

#include <PWMAudio.h>
PWMAudio audio(0);
String SimpleFSTerminal::currentDir = "/";



void SimpleFSTerminal::begin() {
    if (!LittleFS.begin()) {
        Serial.println("格式化文件系统...");
        LittleFS.format();
        LittleFS.begin();
    }
    Serial.println("文件系统就绪!");
    Serial.println("输入 'help' 查看命令");
    Serial.print(currentDir + "> ");
    i2s.setBCLK(19);
    i2s.setDOUT(18);
    i2s.setStereo(true);
    i2s.begin(44100);
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
    File dir = LittleFS.open(fullPath, "r");
    
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
        File dir = LittleFS.open(newPath, "r");
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
    if (LittleFS.mkdir(fullPath)) {
        Serial.println("目录创建成功");
    } else {
        Serial.println("目录创建失败");
    }
}

void SimpleFSTerminal::rm(const String& path) {
    String fullPath = getFullPath(path);
    if (LittleFS.remove(fullPath)) {
        Serial.println("删除成功");
    } else {
        Serial.println("删除失败");
    }
}

void SimpleFSTerminal::cat(const String& path) {
    String fullPath = getFullPath(path);
    File file = LittleFS.open(fullPath, "r");
    
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
    if(audioFile) audioFile.close();
    Serial.printf("开始播放\n");
    String fullPath = getFullPath(path);
    audioFile = LittleFS.open(fullPath, "r");
    if(!audioFile)
      Serial.printf("无指定文件\n");
    else{
      audioEncoder.setDevice(&audio);
      audioEncoder.begin();
      Serial.printf("%d to Write\n", audioEncoder.availableForWrite());
      while(audioFile){
        while(audioFile && audioEncoder.availableForWrite() > 4096){
            int len = audioFile.read(buffer, 4096);
            audioEncoder.write(buffer, len);
            if (len < 4096) audioFile.close();
         }
      }
    }
    audioEncoder.end();
    Serial.printf("结束\n");
}

void SimpleFSTerminal::writeFile(const String& path, const String& content) {
    String fullPath = getFullPath(path);
    File file = LittleFS.open(fullPath, "w");
    
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
    FSInfo info;
    if (LittleFS.info(info)) {
        Serial.print("总空间: ");
        Serial.println(info.totalBytes);
        Serial.print("已用空间: ");
        Serial.println(info.usedBytes);
        Serial.print("使用率: ");
        Serial.println((info.usedBytes * 100) / info.totalBytes);
    }
}

String SimpleFSTerminal::getFullPath(const String& path) {
    if (path.startsWith("/")) {
        return path;
    } else {
        return currentDir + (currentDir.endsWith("/") ? "" : "/") + path;
    }
}