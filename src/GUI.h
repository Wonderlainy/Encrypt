#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <random>
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "FileDialog.h"

std::string getRandomString(unsigned int lenght);
std::string exec(const std::string& command);
std::vector<std::string> splitLine(const std::string &s, char delimiter);

struct kernelFile {
    std::string name;
    std::string randomName;
    std::string password;
};

struct GUI {

    std::vector<kernelFile> kernel;
    void loadKernel();
    void saveKernel();
    bool kernelUnsaved;
    bool saveChanges;
    unsigned int targetFile;
    void action();

    bool deleteFile;
    bool sureToDelete;
    bool exportFile;
    bool importFile;

    std::string exportFilePath;

    std::string addFilePath;
    std::string addFileName;
    bool addFile;

    void init();
    void draw();
};