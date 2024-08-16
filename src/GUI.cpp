#include "GUI.h"

std::string getRandomString(unsigned int lenght) {

    std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    std::random_device rd; // Obtain a random number from hardware
    std::mt19937 gen(rd()); // Seed the generator

    // Define the distribution for integers
    std::uniform_int_distribution<> dis_int(0, chars.size() - 1); // Range

    std::string rs = "";

    for(unsigned int i = 0; i < lenght; i++) {
        rs += chars[dis_int(gen)];
    }

    return rs;

}

std::string exec(const std::string& command) {
    std::string result;
    HANDLE hPipeRead, hPipeWrite;
    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    // Create a pipe for the child process's output.
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0)) {
        throw std::runtime_error("CreatePipe failed");
    }

    // Ensure the read handle to the pipe is not inherited.
    if (!SetHandleInformation(hPipeRead, HANDLE_FLAG_INHERIT, 0)) {
        throw std::runtime_error("SetHandleInformation failed");
    }

    // Create the child process.
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hPipeWrite;
    si.hStdError = hPipeWrite;

    PROCESS_INFORMATION pi;
    if (!CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        throw std::runtime_error("CreateProcess failed");
    }

    // Close the write end of the pipe before reading from the read end of the pipe.
    CloseHandle(hPipeWrite);

    // Read the output from the pipe.
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hPipeRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    // Wait for the child process to exit.
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hPipeRead);

    return result;
}

// Function to split a string by a given delimiter
std::vector<std::string> splitLine(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void GUI::loadKernel() {
    system("gpg --yes --decrypt --output file kernel");

    std::ifstream file("file");
    if (!file.is_open()) {
        std::cerr << "Failed to open kernel file" << std::endl;
        return;
    }

    std::string line;
    while (getline(file, line)) {
        std::vector<std::string> tokens = splitLine(line, ';');
        kernelFile f;
        f.name = tokens[0];
        f.randomName = tokens[1];
        f.password = tokens[2];
        kernel.push_back(f);
    }

    file.close();

    system("rm file");
    //system("cipher /w:.");

}

void GUI::saveKernel() {
    std::ofstream file;
    file.open("file");

    // Check if the file was successfully opened
    if (!file) {
        std::cerr << "Failed to create the kernel file." << std::endl;
        return;
    }

    // Write data to the file
    for(unsigned int i = 0; i < kernel.size(); i++) {
        file << kernel[i].name + ";" + kernel[i].randomName + ";" + kernel[i].password + "\n";
    }

    // Close the file
    file.close();

    system("gpg --yes --symmetric --cipher-algo AES256 --output kernel file");
    system("rm file");
    //system("cipher /w:.");
}

void GUI::init() {
    // Check if kernel file exists
    std::string path = "kernel";
    if (std::filesystem::exists(path)) {
        std::cout << "Kernel file exists." << std::endl;
        loadKernel();
    } else {
        std::cout << "Kernel file does not exist." << std::endl;
    }

    // Check if files folder exists
    path = "files";
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
        std::cout << "files folder exists." << std::endl;
    } else {
        std::cout << "files folder does not exist." << std::endl;
        system("mkdir files");
    }

    deleteFile = false;
    sureToDelete = false;
    exportFile = false;
    importFile = false;
    addFile = false;
    kernelUnsaved = false;
    saveChanges = false;
    addFilePath = "";

}

void GUI::draw() {
    ImGui::Begin("Files");

    if(ImGui::Button("Add")) {
        addFilePath = openFileDialog();
        if(addFilePath != "") {
            addFile = true;
            std::vector<std::string> tokens = splitLine(addFilePath, '\\');
            addFileName = tokens[tokens.size() - 1];
        }
    }
    ImGui::SameLine();
    if(ImGui::Button("Delete")) {
        deleteFile = !deleteFile;
        exportFile = importFile = false;
    }
    ImGui::SameLine();
    if(ImGui::Button("Export")) {
        exportFile = !exportFile;
        deleteFile = importFile = false;
    }
    ImGui::SameLine();
    if(ImGui::Button("Import")) {
        importFile = !importFile;
        exportFile = deleteFile = false;
    }
    ImGui::Spacing();
    ImGui::Separator();

    for(unsigned int i = 0; i < kernel.size(); i++) {
        if(deleteFile || importFile || exportFile) {
            if(ImGui::Button(std::to_string(i).c_str())) {
                targetFile = i;
                action();
            }
            ImGui::SameLine();
        }
        ImGui::Text(kernel[i].name.c_str());
    }

    ImGui::End();

    if(addFile) {
        ImGui::Begin("Add file");
        ImGui::InputText("File Name", &addFileName);
        if(ImGui::Button("Add")) {
            kernelFile f;
            f.name = addFileName; addFileName = "";
            f.randomName = getRandomString(10);
            f.password = getRandomString(30);
            kernel.push_back(f);
            std::string command = "gpg --batch --yes --passphrase " + f.password + " --symmetric --cipher-algo AES256 --output files/" + f.randomName + " \"" + addFilePath + "\"";
            system(command.c_str());
            addFile = false;
            kernelUnsaved = true;
        }
        ImGui::End();
    }

    if(kernelUnsaved) {
        ImGui::Begin("Kernel modified");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unsaved changes");
        if(ImGui::Button("Save")) {
            saveKernel();
            kernelUnsaved =  false;
        }
        ImGui::End();
    }

    if(sureToDelete) {
        ImGui::Begin("Delete file");
        ImGui::Text("Are you sure that you want to delete this file?");
        if(ImGui::Button("Yes")) {
            std::string command;
            command = "rm files/" + kernel[targetFile].randomName;
            system(command.c_str());
            kernel.erase(kernel.begin() + targetFile);
            sureToDelete = false;
            kernelUnsaved =  true;
        }
        ImGui::SameLine();
        if(ImGui::Button("No")) {
            sureToDelete = false;
        }
        ImGui::End();
    }

    if(saveChanges) {
        ImGui::Begin("Save changes");
        ImGui::Text("Exit saving changes");
        if(ImGui::Button("Save")) {
            saveKernel();
            system("cipher /w:.");
            kernelUnsaved = false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Discard")) {
            kernelUnsaved = false;
        }
        ImGui::End();
    }
    
    ImGui::Begin("Clean directory");
    if(ImGui::Button("Clean")) {
        system("cipher /w:.");
    }
    ImGui::End();
}

void GUI::action() {
    if(deleteFile) {
        sureToDelete = true;
    }

    if(exportFile) {
        exportFilePath = pickFolderDialog();
        std::string command;
        command = "gpg --batch --yes --passphrase " + kernel[targetFile].password + " --decrypt --output \"" + exportFilePath + "\\" + kernel[targetFile].name + "\" files\\" + kernel[targetFile].randomName;
        system(command.c_str());
    }

    if(importFile) {
        std::string importFilePath = openFileDialog();
        std::string command;
        command = "gpg --batch --yes --passphrase " + kernel[targetFile].password + " --symmetric --cipher-algo AES256 --output files/" + kernel[targetFile].randomName + " \"" + importFilePath + "\"";
        system(command.c_str());
        command = "rm \"" + importFilePath + "\"";
        system(command.c_str());
    }
}