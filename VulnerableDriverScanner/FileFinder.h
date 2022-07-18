#pragma once
#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>


void FindFilesOfType(std::string dir, std::string& extention, std::vector<std::string>* foundFiles)
{
    std::filesystem::directory_iterator directoryIterator (dir);
	for (const auto& dirEntry : directoryIterator)
	{
		if (dirEntry.is_directory())
		{
			FindFilesOfType(dirEntry.path().string(), extention, foundFiles);
		}

		std::string currentExtention = dirEntry.path().extension().string();
		if (!strcmp(currentExtention.c_str(), extention.c_str()))
		{
			foundFiles->push_back(dirEntry.path().string());
		}
	}

}

bool CreateCopy(std::string& path, std::string& newPath)
{
	//open input and output steam
	std::ofstream newFile(newPath, std::ios::binary);
	std::ifstream baseFile(path, std::ios::binary);

	//error checks
	if (!newFile.is_open())
	{
		std::cout << "Could not open newFile stream";
		return false;
	}

	if (!baseFile.is_open())
	{
		std::cout << "Could not open basefile stream";
		return false;
	}

	//get filesize
	baseFile.seekg(0, std::ios::end);
	int fileSize = baseFile.tellg();
	baseFile.seekg(0, std::ios::beg);

	//create buffer for filesize and copy into it
	char* content = new char[fileSize];
	baseFile.read(content, fileSize);
	//copy buffer to new file
	newFile.write(content, fileSize);

	//clean up
	baseFile.close();
	newFile.close();
	delete[] content;
	return true;
}