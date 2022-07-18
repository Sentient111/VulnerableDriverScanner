#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>

//no clue why dis is not in the normal header
#define IMAGE_FILE_MACHINE_RISCV64 0x5064

class PeFile
{
public:
	//ctors
	PeFile(std::string path);
	~PeFile();

	//functions
	bool HasImport(std::string& importName);
	
	PVOID ntHeaders = nullptr;
	PIMAGE_DOS_HEADER dosHeader = nullptr;

	//variables
	bool isOpen = false;
	bool is64Bit = false;


private:
	uintptr_t fileBuffer = 0;

	bool HasImport32(std::string& importName);
	bool HasImport64(std::string& importName);

	uintptr_t TranslateVa(uintptr_t rva)
	{
		//PIMAGE_NT_HEADERS32 
		PIMAGE_FILE_HEADER fileHeader = &((PIMAGE_NT_HEADERS32)this->ntHeaders)->FileHeader;

		PIMAGE_SECTION_HEADER currentSection = IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS32)this->ntHeaders);

		for (size_t i = 0; i < ((PIMAGE_NT_HEADERS32)this->ntHeaders)->FileHeader.NumberOfSections; ++i, ++currentSection)
		{
			if (rva >= currentSection->VirtualAddress && rva < currentSection->VirtualAddress + currentSection->Misc.VirtualSize)
			{
				return this->fileBuffer + currentSection->PointerToRawData + (rva - currentSection->VirtualAddress);
			}
		}
		return 0;
	}
};

bool PeFile::HasImport64(std::string& importName)
{
	PIMAGE_NT_HEADERS64 currentNtheaders = (PIMAGE_NT_HEADERS64)this->ntHeaders;

	if (!currentNtheaders)
	{
		printf("[PE] failed to get nt header\n");
		return false;
	}

	PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)TranslateVa(currentNtheaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	if (!importDescriptor)
	{
		printf("[PE] failed to get import descriptior\n");
		return false;
	}

	for (; importDescriptor->FirstThunk; ++importDescriptor)
	{
		for (PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)TranslateVa(importDescriptor->FirstThunk); thunk->u1.AddressOfData != 0; ++thunk)
		{
			PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)TranslateVa(thunk->u1.AddressOfData);
			if (!importByName)
				return false;

			if (!strcmp(importByName->Name, importName.c_str()))
			{
				return true;
			}

		}
	}
	return false;
}


bool PeFile::HasImport32(std::string& importName)
{
	PIMAGE_NT_HEADERS fileHeader = (PIMAGE_NT_HEADERS)this->ntHeaders;
	PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)TranslateVa(fileHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	if (!importDescriptor)
	{
		printf("[PE] failed to get import descriptior\n");
		return false;
	}

	for (; importDescriptor->FirstThunk; ++importDescriptor)
	{
		for (PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)TranslateVa(importDescriptor->FirstThunk); thunk->u1.AddressOfData; ++thunk)
		{
			PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)TranslateVa(thunk->u1.AddressOfData);
			if (!strcmp(importByName->Name, importName.c_str()))
			{
				return true;
			}
		}
	}
}


bool PeFile::HasImport(std::string& importName)
{
	if (this->is64Bit)
	{
		return HasImport64(importName);
	}
	else
	{
		return HasImport32(importName);
	}
}

PeFile::PeFile(std::string path)
{
	std::ifstream inputPeFile(path, std::ios::binary);
	if (!inputPeFile.is_open())
	{
		printf("[PE] failed to open %s\n", path.c_str());
		return;
	}

	inputPeFile.seekg(0, std::ios::end);
	int fileSize = inputPeFile.tellg();
	inputPeFile.seekg(0, std::ios::beg);

	if (!fileSize)
	{
		printf("[PE] following driver has a invalid file size\n");
		printf("%s\n", path.c_str());
		inputPeFile.close();
		return;
	}

	this->fileBuffer = (uintptr_t)VirtualAlloc(NULL, fileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!fileBuffer)
	{
		printf("[PE] failed to allocate memory for file buffer %x\n", GetLastError());
		inputPeFile.close();
		return;
	}

	inputPeFile.read((char*)this->fileBuffer, fileSize);
	inputPeFile.close();


	this->dosHeader = (PIMAGE_DOS_HEADER)(this->fileBuffer);
	PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)(this->fileBuffer + this->dosHeader->e_lfanew);

	WORD machineType = ntHeaders->FileHeader.Machine;

	if (machineType == IMAGE_FILE_MACHINE_AMD64 || machineType == IMAGE_FILE_MACHINE_ARM64 || machineType == IMAGE_FILE_MACHINE_RISCV64)
	{
		//image is 64 bit
		this->is64Bit = true;
	}
	
	else if (machineType == IMAGE_FILE_MACHINE_I386)
	{
		//image is 32 bit
		this->is64Bit = false;
	}

	else
	{
		printf("[PE] pe file does not have a valid machine type\n");
		return;
	}

	this->ntHeaders = ntHeaders;
	this->isOpen = true;
}

PeFile::~PeFile()
{
	if (this->isOpen)
	{
		if (!VirtualFree((PVOID)this->fileBuffer, NULL, MEM_RELEASE))
		{
			printf("[PE] failed to free pe file buffer %x\n", GetLastError());
		}
	}
}