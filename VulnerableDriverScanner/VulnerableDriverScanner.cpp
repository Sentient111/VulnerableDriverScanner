#include "PeFile.h"
#include "FileFinder.h"

std::vector<std::string> wantedImports = {"IoCreateSymbolicLink", "IoCreateDevice"};

std::vector<std::string> foundDrivers;
std::string extention(".sys");



int main()
{
    std::cout << "Enter scan path:";
    std::filesystem::path scanPath;
    std::cin >> scanPath;

    std::cout << "Enter output path:";
    std::filesystem::path outputPath;
    std::cin >> outputPath;

    //fills foundDrivers with all the paths of .sys files found in the search dir
    FindFilesOfType(scanPath.string(), extention, &foundDrivers);

    for (std::string currDriverPath : foundDrivers)
    {
        PeFile currentPeFile(currDriverPath);
        if (!currentPeFile.isOpen || !currentPeFile.is64Bit)
            continue;
        
        for (std::string wantedImport : wantedImports)
        {
            if (currentPeFile.HasImport(wantedImport))
            { 
                std::filesystem::path driverPath(currDriverPath);
                std::string newPath(outputPath.string() + driverPath.filename().string());
                if (!CreateCopy(currDriverPath, newPath))
                {
                    printf("failed to create copy\n");
                }
                break;
            }
        }
    }

}

