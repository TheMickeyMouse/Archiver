#include <filesystem>
#include <iostream>

#include "Archive.h"
#include "Utils/Str.h"

void TryHelp() {
    std::cout << "archive: use the -h or --help option to show usage\n";
}

int main(int argc, char* argv[]) {
    using namespace Quasi;
    switch (argc) {
        case 1:
            std::cout << "archive: no input files\n";
            TryHelp();
            return 1;
        case 2: {
            const Str option = argv[1];
            if (option == "-h" || option == "--help") {
                std::cout << "usage: archive [options] files...\n"
                             " Options:\n"
                             "  -o OUTPUT_FILE              Exports the archive object file to OUTPUT_FILE.\n"
                             "  -i BUILD_INCLUDE_FILE       Exports the archive header file to BUILD_INCLUDE_FILE. \n"
                             "                              By default prints to the console. \n"
                             "  -r RESOURCE_DIR             Tells archive where to search for the resource files. \n"
                             "  -h / --help                 Shows this screen.\n";
                return 0;
            } else {
                TryHelp();
                return 1;
            }
        }
        default:;
    }
    if (!Archive::CheckReq())
        return 1;

    Option<Str> outputFile, buildIncludeFile;
    Str resourceDir = ".";
    int i = 1;
    for (; i < argc; i++) {
        const Str s = argv[i];
        if (s == "-o") {
            outputFile = Str(argv[i + 1]);
            ++i;
            continue;
        }
        if (s == "-i") {
            buildIncludeFile = Str(argv[i + 1]);
            ++i;
            continue;
        }
        if (s == "-r") {
            resourceDir = Str(argv[i + 1]);
            ++i;
            continue;
        }
        break;
    }
    if (outputFile.IsNull()) {
        std::cout << "archive: no output file specified\n";
        TryHelp();
        return 1;
    }

    Vec<Str> args;
    for (; i < argc; i++) {
        args.Push(argv[i]);
    }

    struct File {
        FILE* f = nullptr;
        ~File() { if (f) std::fclose(f); }
    } file = { buildIncludeFile ? std::fopen(buildIncludeFile->Data(), "w") : nullptr };

    const wchar_t* wcurPath = std::filesystem::current_path().c_str();
    std::mbstate_t state = std::mbstate_t();
    size_t len = std::wcsrtombs(nullptr, &wcurPath, 0, &state);
    String curPath = String::WithCap(len);
    curPath.Resize(len);
    std::wcsrtombs(curPath.Data(), &wcurPath, len, &state);

    Debug::QInfo$("Running from {}, len = {}", curPath, len);

    Archive::ArchiveFiles(args, resourceDir, curPath, *outputFile,
        buildIncludeFile ? Text::StringWriter::WriteToFile(file.f) : Text::StringWriter::WriteToConsole());
}