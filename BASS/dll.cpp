//#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winnt.h>
#include <stddef.h>
#include <stdint.h>
#include <tchar.h>
#include <iostream>
#include <fstream>

//#ifdef DEBUG_OUTPUT
#include <stdio.h>
//#endif

#if _MSC_VER
// Disable warning about data -> function pointer conversion
#pragma warning(disable:4055)
#endif

#define IMAGE_SIZEOF_BASE_RELOCATION (sizeof(IMAGE_BASE_RELOCATION))

#include "dll.h"

#define GET_HEADER_DICTIONARY(module, idx)  &(module)->headers->OptionalHeader.DataDirectory[idx]
#define ALIGN_DOWN(address, alignment)      (LPVOID)((uintptr_t)(address) & ~((alignment) - 1))
#define ALIGN_VALUE_UP(value, alignment)    (((value) + (alignment) - 1) & ~((alignment) - 1))

BOOL
CWin32PE::CheckSize(size_t size, size_t expected) {
	if (size < expected) {
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	return TRUE;
}

BOOL CWin32PE::CopySections(const unsigned char *data, size_t size, PIMAGE_NT_HEADERS old_headers, PMEMORYMODULE module)
{
	int i, section_size;
	unsigned char *codeBase = module->codeBase;
	unsigned char *dest;
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
	for (i = 0; i < module->headers->FileHeader.NumberOfSections; i++, section++) {
		if (section->SizeOfRawData == 0) {
			// section doesn't contain data in the dll itself, but may define
			// uninitialized data
			section_size = old_headers->OptionalHeader.SectionAlignment;
			if (section_size > 0) {
				dest = (unsigned char *) VirtualAlloc(codeBase + section->VirtualAddress,
													  section_size,
													  MEM_COMMIT,
													  PAGE_READWRITE);
				if (dest == NULL) {
					return FALSE;
				}

				// Always use position from file to support alignments smaller
				// than page size.
				dest = codeBase + section->VirtualAddress;
				section->Misc.PhysicalAddress = (DWORD) (uintptr_t) dest;
				memset(dest, 0, section_size);
			}

			// section is empty
			continue;
		}

		if (!CheckSize(size, section->PointerToRawData + section->SizeOfRawData)) {
			return FALSE;
		}

		// commit memory block and copy data from dll
		dest = (unsigned char *) VirtualAlloc(codeBase + section->VirtualAddress,
											  section->SizeOfRawData,
											  MEM_COMMIT,
											  PAGE_READWRITE);
		if (dest == NULL) {
			return FALSE;
		}

		// Always use position from file to support alignments smaller
		// than page size.
		dest = codeBase + section->VirtualAddress;
		memcpy(dest, data + section->PointerToRawData, section->SizeOfRawData);
		section->Misc.PhysicalAddress = (DWORD) (uintptr_t) dest;
	}

	return TRUE;
}

// Protection flags for memory pages (Executable, Readable, Writeable)
static int ProtectionFlags[2][2][2] = {
	{
		// not executable
		{ PAGE_NOACCESS, PAGE_WRITECOPY },
{ PAGE_READONLY, PAGE_READWRITE },
	},{
		// executable
		{ PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY },
{ PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE },
	},
};

DWORD
CWin32PE::GetRealSectionSize(PMEMORYMODULE module, PIMAGE_SECTION_HEADER section) {
	DWORD size = section->SizeOfRawData;
	if (size == 0) {
		if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
			size = module->headers->OptionalHeader.SizeOfInitializedData;
		}
		else if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
			size = module->headers->OptionalHeader.SizeOfUninitializedData;
		}
	}
	return size;
}

BOOL
CWin32PE::FinalizeSection(PMEMORYMODULE module, PSECTIONFINALIZEDATA sectionData) {
	DWORD protect, oldProtect;
	BOOL executable;
	BOOL readable;
	BOOL writeable;

	if (sectionData->size == 0) {
		return TRUE;
	}

	if (sectionData->characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
		// section is not needed any more and can safely be freed
		if (sectionData->address == sectionData->alignedAddress &&
			(sectionData->last ||
			 module->headers->OptionalHeader.SectionAlignment == module->pageSize ||
			 (sectionData->size % module->pageSize) == 0)
			) {
			// Only allowed to decommit whole pages
			VirtualFree(sectionData->address, sectionData->size, MEM_DECOMMIT);
		}
		return TRUE;
	}

	// determine protection flags based on characteristics
	executable = (sectionData->characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
	readable = (sectionData->characteristics & IMAGE_SCN_MEM_READ) != 0;
	writeable = (sectionData->characteristics & IMAGE_SCN_MEM_WRITE) != 0;
	protect = ProtectionFlags[executable][readable][writeable];
	if (sectionData->characteristics & IMAGE_SCN_MEM_NOT_CACHED) {
		protect |= PAGE_NOCACHE;
	}

	// change memory access flags
	if (VirtualProtect(sectionData->address, sectionData->size, protect, &oldProtect) == 0) {
//#ifdef DEBUG_OUTPUT
		//OutputLastError("Error protecting memory page")
//#endif
			return FALSE;
	}

	return TRUE;
}

BOOL
CWin32PE::FinalizeSections(PMEMORYMODULE module)
{
	int i;
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
#ifdef _WIN64
	uintptr_t imageOffset = (module->headers->OptionalHeader.ImageBase & 0xffffffff00000000);
#else
#define imageOffset 0
#endif
	SECTIONFINALIZEDATA sectionData;
	sectionData.address = (LPVOID) ((uintptr_t) section->Misc.PhysicalAddress | imageOffset);
	sectionData.alignedAddress = ALIGN_DOWN(sectionData.address, module->pageSize);
	sectionData.size = GetRealSectionSize(module, section);
	sectionData.characteristics = section->Characteristics;
	sectionData.last = FALSE;
	section++;

	// loop through all sections and change access flags
	for (i = 1; i < module->headers->FileHeader.NumberOfSections; i++, section++) {
		LPVOID sectionAddress = (LPVOID) ((uintptr_t) section->Misc.PhysicalAddress | imageOffset);
		LPVOID alignedAddress = ALIGN_DOWN(sectionAddress, module->pageSize);
		DWORD sectionSize = GetRealSectionSize(module, section);
		// Combine access flags of all sections that share a page
		// TODO(fancycode): We currently share flags of a trailing large section
		//   with the page of a first small section. This should be optimized.
		if (sectionData.alignedAddress == alignedAddress || (uintptr_t) sectionData.address + sectionData.size > (uintptr_t) alignedAddress) {
			// Section shares page with previous
			if ((section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0 || (sectionData.characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0) {
				sectionData.characteristics = (sectionData.characteristics | section->Characteristics) & ~IMAGE_SCN_MEM_DISCARDABLE;
			}
			else {
				sectionData.characteristics |= section->Characteristics;
			}
			sectionData.size = (((uintptr_t) sectionAddress) + sectionSize) - (uintptr_t) sectionData.address;
			continue;
		}

		if (!FinalizeSection(module, &sectionData)) {
			return FALSE;
		}
		sectionData.address = sectionAddress;
		sectionData.alignedAddress = alignedAddress;
		sectionData.size = sectionSize;
		sectionData.characteristics = section->Characteristics;
	}
	sectionData.last = TRUE;
	if (!FinalizeSection(module, &sectionData)) {
		return FALSE;
	}
#ifndef _WIN64
#undef imageOffset
#endif
	return TRUE;
}

BOOL
CWin32PE::ExecuteTLS(PMEMORYMODULE module)
{
	unsigned char *codeBase = module->codeBase;
	PIMAGE_TLS_DIRECTORY tls;
	PIMAGE_TLS_CALLBACK* callback;

	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_TLS);
	if (directory->VirtualAddress == 0) {
		return TRUE;
	}

	tls = (PIMAGE_TLS_DIRECTORY) (codeBase + directory->VirtualAddress);
	callback = (PIMAGE_TLS_CALLBACK *) tls->AddressOfCallBacks;
	if (callback) {
		while (*callback) {
			(*callback)((LPVOID) codeBase, DLL_PROCESS_ATTACH, NULL);
			callback++;
		}
	}
	return TRUE;
}

BOOL
CWin32PE::PerformBaseRelocation(PMEMORYMODULE module, ptrdiff_t delta)
{
	unsigned char *codeBase = module->codeBase;
	PIMAGE_BASE_RELOCATION relocation;

	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_BASERELOC);
	if (directory->Size == 0) {
		return (delta == 0);
	}

	relocation = (PIMAGE_BASE_RELOCATION) (codeBase + directory->VirtualAddress);
	for (; relocation->VirtualAddress > 0;) {
		DWORD i;
		unsigned char *dest = codeBase + relocation->VirtualAddress;
		unsigned short *relInfo = (unsigned short *) ((unsigned char *) relocation + IMAGE_SIZEOF_BASE_RELOCATION);
		for (i = 0; i < ((relocation->SizeOfBlock - IMAGE_SIZEOF_BASE_RELOCATION) / 2); i++, relInfo++) {
			DWORD *patchAddrHL;
#ifdef _WIN64
			ULONGLONG *patchAddr64;
#endif
			int type, offset;

			// the upper 4 bits define the type of relocation
			type = *relInfo >> 12;
			// the lower 12 bits define the offset
			offset = *relInfo & 0xfff;

			switch (type)
			{
			case IMAGE_REL_BASED_ABSOLUTE:
				// skip relocation
				break;

			case IMAGE_REL_BASED_HIGHLOW:
				// change complete 32 bit address
				patchAddrHL = (DWORD *) (dest + offset);
				*patchAddrHL += (DWORD) delta;
				break;

#ifdef _WIN64
			case IMAGE_REL_BASED_DIR64:
				patchAddr64 = (ULONGLONG *) (dest + offset);
				*patchAddr64 += (ULONGLONG) delta;
				break;
#endif

			default:
				//printf("Unknown relocation: %d\n", type);
				break;
			}
		}

		// advance to next relocation block
		relocation = (PIMAGE_BASE_RELOCATION) (((char *) relocation) + relocation->SizeOfBlock);
	}
	return TRUE;
}

BOOL
CWin32PE::BuildImportTable(PMEMORYMODULE module)
{
	unsigned char *codeBase = module->codeBase;
	PIMAGE_IMPORT_DESCRIPTOR importDesc;
	BOOL result = TRUE;

	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_IMPORT);
	if (directory->Size == 0) {
		return TRUE;
	}

	importDesc = (PIMAGE_IMPORT_DESCRIPTOR) (codeBase + directory->VirtualAddress);
	for (; !IsBadReadPtr(importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR)) && importDesc->Name; importDesc++) {
		uintptr_t *thunkRef;
		FARPROC *funcRef;
		HCUSTOMMODULE *tmp;
		HCUSTOMMODULE handle = module->loadLibrary((LPCSTR) (codeBase + importDesc->Name), module->userdata);
		if (handle == NULL) {
			SetLastError(ERROR_MOD_NOT_FOUND);
			result = FALSE;
			break;
		}

		tmp = (HCUSTOMMODULE *) realloc(module->modules, (module->numModules + 1)*(sizeof(HCUSTOMMODULE)));
		if (tmp == NULL) {
			module->freeLibrary(handle, module->userdata);
			SetLastError(ERROR_OUTOFMEMORY);
			result = FALSE;
			break;
		}
		module->modules = tmp;

		module->modules[module->numModules++] = handle;
		if (importDesc->OriginalFirstThunk) {
			thunkRef = (uintptr_t *) (codeBase + importDesc->OriginalFirstThunk);
			funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
		}
		else {
			// no hint table
			thunkRef = (uintptr_t *) (codeBase + importDesc->FirstThunk);
			funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
		}
		for (; *thunkRef; thunkRef++, funcRef++) {
			if (IMAGE_SNAP_BY_ORDINAL(*thunkRef)) {
				*funcRef = module->getProcAddress(handle, (LPCSTR) IMAGE_ORDINAL(*thunkRef), module->userdata);
			}
			else {
				PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME) (codeBase + (*thunkRef));
				*funcRef = module->getProcAddress(handle, (LPCSTR) &thunkData->Name, module->userdata);
			}
			if (*funcRef == 0) {
				result = FALSE;
				break;
			}
		}

		if (!result) {
			module->freeLibrary(handle, module->userdata);
			SetLastError(ERROR_PROC_NOT_FOUND);
			break;
		}
	}

	return result;
}


HCUSTOMMODULE MemoryDefaultLoadLibrary(LPCSTR filename, void *userdata)
{
	HMODULE result;
	UNREFERENCED_PARAMETER(userdata);
	result = LoadLibraryA(filename);
	if (result == NULL) {
		return NULL;
	}

	return (HCUSTOMMODULE) result;
}

FARPROC MemoryDefaultGetProcAddress(HCUSTOMMODULE module, LPCSTR name, void *userdata)
{
	UNREFERENCED_PARAMETER(userdata);
	return (FARPROC) GetProcAddress((HMODULE) module, name);
}

void MemoryDefaultFreeLibrary(HCUSTOMMODULE module, void *userdata)
{
	UNREFERENCED_PARAMETER(userdata);
	FreeLibrary((HMODULE) module);
}


HANDLE CLoad::MemLoadLibraryEx(const void *data, size_t size,
							   MemLoadLibraryFn loadLibrary,
							   MemGetProcAddressFn getProcAddress,
							   MemFreeLibraryFn freeLibrary,
							   void *userdata)
{
	PMEMORYMODULE result = NULL;
	PIMAGE_DOS_HEADER dos_header;
	PIMAGE_NT_HEADERS old_header;
	unsigned char *code, *headers;
	ptrdiff_t locationDelta;
	SYSTEM_INFO sysInfo;
	PIMAGE_SECTION_HEADER section;
	DWORD i;
	size_t optionalSectionSize;
	size_t lastSectionEnd = 0;
	size_t alignedImageSize;

	if (!CheckSize(size, sizeof(IMAGE_DOS_HEADER))) {
		return NULL;
	}
	dos_header = (PIMAGE_DOS_HEADER) data;
	if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
		SetLastError(ERROR_BAD_EXE_FORMAT);
		return NULL;
	}

	if (!CheckSize(size, dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS))) {
		return NULL;
	}
	old_header = (PIMAGE_NT_HEADERS)&((const unsigned char *) (data))[dos_header->e_lfanew];
	if (old_header->Signature != IMAGE_NT_SIGNATURE) {
		SetLastError(ERROR_BAD_EXE_FORMAT);
		return NULL;
	}

#ifdef _WIN64
	if (old_header->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
#else
	if (old_header->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
#endif
		SetLastError(ERROR_BAD_EXE_FORMAT);
		return NULL;
	}

	if (old_header->OptionalHeader.SectionAlignment & 1) {
		// Only support section alignments that are a multiple of 2
		SetLastError(ERROR_BAD_EXE_FORMAT);
		return NULL;
	}

	section = IMAGE_FIRST_SECTION(old_header);
	optionalSectionSize = old_header->OptionalHeader.SectionAlignment;
	for (i = 0; i < old_header->FileHeader.NumberOfSections; i++, section++) {
		size_t endOfSection;
		if (section->SizeOfRawData == 0) {
			// Section without data in the DLL
			endOfSection = section->VirtualAddress + optionalSectionSize;
		}
		else {
			endOfSection = section->VirtualAddress + section->SizeOfRawData;
		}

		if (endOfSection > lastSectionEnd) {
			lastSectionEnd = endOfSection;
		}
	}

	GetNativeSystemInfo(&sysInfo);
	alignedImageSize = ALIGN_VALUE_UP(old_header->OptionalHeader.SizeOfImage, sysInfo.dwPageSize);
	if (alignedImageSize != ALIGN_VALUE_UP(lastSectionEnd, sysInfo.dwPageSize)) {
		SetLastError(ERROR_BAD_EXE_FORMAT);
		return NULL;
	}

	// reserve memory for image of library
	// XXX: is it correct to commit the complete memory region at once?
	//      calling DllEntry raises an exception if we don't...
	code = (unsigned char *) VirtualAlloc((LPVOID) (old_header->OptionalHeader.ImageBase),
										  alignedImageSize,
										  MEM_RESERVE | MEM_COMMIT,
										  PAGE_READWRITE);

	if (code == NULL) {
		// try to allocate memory at arbitrary position
		code = (unsigned char *) VirtualAlloc(NULL,
											  alignedImageSize,
											  MEM_RESERVE | MEM_COMMIT,
											  PAGE_READWRITE);
		if (code == NULL) {
			SetLastError(ERROR_OUTOFMEMORY);
			return NULL;
		}
	}

	result = (PMEMORYMODULE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MEMORYMODULE));
	if (result == NULL) {
		VirtualFree(code, 0, MEM_RELEASE);
		SetLastError(ERROR_OUTOFMEMORY);
		return NULL;
	}

	result->codeBase = code;
	result->isDLL = (old_header->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
	result->loadLibrary = loadLibrary;
	result->getProcAddress = getProcAddress;
	result->freeLibrary = freeLibrary;
	result->userdata = userdata;
	result->pageSize = sysInfo.dwPageSize;

	if (!CheckSize(size, old_header->OptionalHeader.SizeOfHeaders)) {
		goto error;
	}

	// commit memory for headers
	headers = (unsigned char *) VirtualAlloc(code,
											 old_header->OptionalHeader.SizeOfHeaders,
											 MEM_COMMIT,
											 PAGE_READWRITE);

	// copy PE header to code
	memcpy(headers, dos_header, old_header->OptionalHeader.SizeOfHeaders);
	result->headers = (PIMAGE_NT_HEADERS)&((const unsigned char *) (headers))[dos_header->e_lfanew];

	// update position
	result->headers->OptionalHeader.ImageBase = (uintptr_t) code;

	// copy sections from DLL file block to new memory location
	if (!CopySections((const unsigned char *) data, size, old_header, result)) {
		goto error;
	}

	// adjust base address of imported data
	locationDelta = (ptrdiff_t) (result->headers->OptionalHeader.ImageBase - old_header->OptionalHeader.ImageBase);
	if (locationDelta != 0) {
		result->isRelocated = PerformBaseRelocation(result, locationDelta);
	}
	else {
		result->isRelocated = TRUE;
	}

	// load required dlls and adjust function table of imports
	if (!BuildImportTable(result)) {
		goto error;
	}

	// mark memory pages depending on section headers and release
	// sections that are marked as "discardable"
	if (!FinalizeSections(result)) {
		goto error;
	}

	// TLS callbacks are executed BEFORE the main loading
	if (!ExecuteTLS(result)) {
		goto error;
	}

	// get entry point of loaded library
	if (result->headers->OptionalHeader.AddressOfEntryPoint != 0) {
		if (result->isDLL) {
			DllEntryProc DllEntry = (DllEntryProc) (LPVOID) (code + result->headers->OptionalHeader.AddressOfEntryPoint);
			// notify library about attaching to process
			BOOL successfull = (*DllEntry)((HINSTANCE) code, DLL_PROCESS_ATTACH, 0);
			if (!successfull) {
				SetLastError(ERROR_DLL_INIT_FAILED);
				goto error;
			}
			result->initialized = TRUE;
		}
		else {
			result->exeEntry = (ExeEntryProc) (LPVOID) (code + result->headers->OptionalHeader.AddressOfEntryPoint);
		}
	}
	else {
		result->exeEntry = NULL;
	}

	return (HANDLE) result;

error:
	// cleanup
	FreeLibraryFromMemory(result);
	return NULL;
}

HANDLE CLoad::LoadFromMemory(const void *data, size_t size)
{
	return MemLoadLibraryEx(data, size, MemoryDefaultLoadLibrary, MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, NULL);
}

FARPROC CLoad::GetProcAddressFromMemory(HANDLE module, LPCSTR name)
{
	if (!module || !name)
		return 0x0;

	unsigned char *codeBase = ((PMEMORYMODULE) module)->codeBase;
	DWORD idx = 0;
	PIMAGE_EXPORT_DIRECTORY exports;
	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY((PMEMORYMODULE) module, IMAGE_DIRECTORY_ENTRY_EXPORT);
	if (directory->Size == 0) {
		// no export table found
		SetLastError(ERROR_PROC_NOT_FOUND);
		return NULL;
	}

	exports = (PIMAGE_EXPORT_DIRECTORY) (codeBase + directory->VirtualAddress);
	if (exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0) {
		// DLL doesn't export anything
		SetLastError(ERROR_PROC_NOT_FOUND);
		return NULL;
	}

	if (HIWORD(name) == 0) {
		// load function by ordinal value
		if (LOWORD(name) < exports->Base) {
			SetLastError(ERROR_PROC_NOT_FOUND);
			return NULL;
		}

		idx = LOWORD(name) - exports->Base;
	}
	else {
		// search function name in list of exported names
		DWORD i;
		DWORD *nameRef = (DWORD *) (codeBase + exports->AddressOfNames);
		WORD *ordinal = (WORD *) (codeBase + exports->AddressOfNameOrdinals);
		BOOL found = FALSE;
		for (i = 0; i < exports->NumberOfNames; i++, nameRef++, ordinal++) {
			if (_stricmp(name, (const char *) (codeBase + (*nameRef))) == 0) {
				idx = *ordinal;
				found = TRUE;
				break;
			}
		}

		if (!found) {
			// exported symbol not found
			SetLastError(ERROR_PROC_NOT_FOUND);
			return NULL;
		}
	}

	if (idx > exports->NumberOfFunctions) {
		// name <-> ordinal number don't match
		SetLastError(ERROR_PROC_NOT_FOUND);
		return NULL;
	}

	// AddressOfFunctions contains the RVAs to the "real" functions
	return (FARPROC) (LPVOID) (codeBase + (*(DWORD *) (codeBase + exports->AddressOfFunctions + (idx * 4))));
}

void CLoad::FreeLibraryFromMemory(HANDLE mod)
{
	PMEMORYMODULE module = (PMEMORYMODULE) mod;

	if (module == NULL) {
		return;
	}
	if (module->initialized) {
		// notify library about detaching from process
		DllEntryProc DllEntry = (DllEntryProc) (LPVOID) (module->codeBase + module->headers->OptionalHeader.AddressOfEntryPoint);
		(*DllEntry)((HINSTANCE) module->codeBase, DLL_PROCESS_DETACH, 0);
	}

	if (module->modules != NULL) {
		// free previously opened libraries
		int i;
		for (i = 0; i < module->numModules; i++) {
			if (module->modules[i] != NULL) {
				module->freeLibrary(module->modules[i], module->userdata);
			}
		}

		free(module->modules);
	}

	if (module->codeBase != NULL) {
		// release memory of library
		VirtualFree(module->codeBase, 0, MEM_RELEASE);
	}

	HeapFree(GetProcessHeap(), 0, module);
}

int CLoad::CallEntryPointFromMemory(HANDLE mod)
{
	PMEMORYMODULE module = (PMEMORYMODULE) mod;

	if (module == NULL || module->isDLL || module->exeEntry == NULL || !module->isRelocated) {
		return -1;
	}

	return module->exeEntry();
}

HANDLE CLoad::LoadFromFile(LPCSTR filename)
{
	HANDLE Module;
	size_t size;
	char * memblock;
	std::fstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open())
	{
		size = size_t(file.tellg());
		memblock = new char[size];
		file.seekg(0, std::ios::beg);
		file.read(memblock, size);
		file.close();
		Module = LoadFromMemory(memblock, size);
		delete[] memblock;
		return Module;
	}
	else {
		return 0;
	}
}

HANDLE CLoad::LoadFromResources(int IDD_RESOUCE)
{
	/*
	HGLOBAL hResData;
	HRSRC   hResInfo;
	void    *pvRes;
	DWORD dwSize;
	void* lpMemory;
	HMODULE hModule = GetModuleHandle( NULL );

	if ( ( ( hResInfo = FindResource( hModule, MAKEINTRESOURCE( IDD_RESOUCE ), "DLL" ) ) != NULL ) && ( ( hResData = LoadResource( hModule, hResInfo ) ) != NULL ) && ( ( pvRes = LockResource( hResData ) ) != NULL ) )
	{
		dwSize = SizeofResource( hModule, hResInfo );
		lpMemory = (char*)malloc( dwSize );
		memset( lpMemory, 0, dwSize );
		memcpy( lpMemory, pvRes, dwSize );
		return lpMemory;
	}
	*/

	return nullptr;
}

namespace BASS
{
	stream_sounds_s stream_sounds;
	HANDLE bass_lib_handle;
	CLoad bass_lib;
	DWORD stream_handle;
	DWORD request_number;
	BOOL bass_init;
	char bass_metadata[MAX_PATH];
	char bass_channelinfo[MAX_PATH];
}
















// Junk Code By Troll Face & Thaisen's Gen
void vGDMWPHhLRzWaRoPrgxreWjzYnaOlUhETquwelJc44999373() {     int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf99968097 = -523833526;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf16355121 = 25969342;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf12987860 = -405230336;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf22754984 = -584202387;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf88305559 = 94936484;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf77993263 = -36653213;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf55163830 = -197274478;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf39955428 = -598471522;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf47973659 = 64538520;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf97218506 = -462729218;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf63446681 = -331217426;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf41298445 = -673349719;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf61981099 = -934519926;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf62835659 = -214372260;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf18009610 = -258397174;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf10421358 = -722335796;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf66569142 = 91353759;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf85701030 = -918593925;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf93246285 = -333647604;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf59309848 = 91015017;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf15416781 = -402688614;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf93861242 = -489723592;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf99984491 = -761712075;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf79990124 = -52233467;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf10459010 = -331968607;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20593792 = -824257620;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf14132866 = -874937017;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf87467094 = -218242677;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf64397328 = -920428959;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf53897207 = -64721263;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf32769713 = -762170035;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf31979022 = -847035;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf15002568 = -584039419;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf34950577 = -938757102;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf41390960 = 73461502;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf36825056 = -922742730;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf13721257 = -69969496;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf33747877 = -662487284;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf91171975 = -245203472;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf74936476 = -494858213;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20990622 = 15196188;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf34676987 = -552695617;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf48552803 = -85560280;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf13309344 = -378222955;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf52945124 = -180657417;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf69583738 = -1651582;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20300464 = -177521339;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf75514668 = -449656810;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf18984337 = -383540541;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf96239781 = -55827343;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20398246 = -405121009;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf67504103 = -431098311;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf54812398 = 32887468;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf35095455 = -453548967;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf29582592 = -261059275;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf6106855 = 65890065;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf16370629 = -212318584;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf32997735 = -252996870;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf12295975 = -152233781;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf67711767 = -80805896;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf63860397 = -161716196;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf67696735 = -979031802;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf75558099 = -678042564;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf94076452 = -870740217;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf64448793 = -700559183;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf31467659 = -230370392;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf26295877 = 10689700;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf27030523 = -995762825;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf21444700 = -187833762;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf81184554 = -335654445;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf96700101 = -552366301;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf32821265 = -246158958;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf94529055 = -573390453;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf18309810 = -838789391;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf38319226 = -924181172;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf80739793 = -849992997;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf45308439 = -304163312;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf86675148 = -283489120;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf27045001 = -871576050;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf40875271 = -230317026;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf293329 = -546736282;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf38618197 = -325280207;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf68482757 = -834702136;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf68157546 = -764601616;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf33498962 = -659600254;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf65265610 = -231071725;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf77166624 = 66265497;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf79907112 = -30490452;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf5367985 = -577697828;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf35284105 = -992428564;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20454428 = -610424147;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf80723521 = -816972626;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf21451903 = -410253504;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf23460208 = -64397577;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf11076079 = -233142018;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf53293886 = -5772011;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf59118888 = -874653054;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf54476351 = -214820064;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf48860550 = -677663772;    int mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf21477465 = -523833526;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf99968097 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf16355121;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf16355121 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf12987860;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf12987860 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf22754984;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf22754984 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf88305559;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf88305559 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf77993263;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf77993263 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf55163830;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf55163830 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf39955428;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf39955428 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf47973659;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf47973659 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf97218506;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf97218506 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf63446681;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf63446681 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf41298445;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf41298445 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf61981099;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf61981099 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf62835659;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf62835659 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf18009610;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf18009610 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf10421358;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf10421358 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf66569142;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf66569142 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf85701030;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf85701030 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf93246285;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf93246285 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf59309848;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf59309848 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf15416781;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf15416781 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf93861242;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf93861242 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf99984491;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf99984491 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf79990124;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf79990124 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf10459010;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf10459010 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20593792;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20593792 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf14132866;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf14132866 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf87467094;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf87467094 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf64397328;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf64397328 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf53897207;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf53897207 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf32769713;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf32769713 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf31979022;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf31979022 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf15002568;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf15002568 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf34950577;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf34950577 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf41390960;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf41390960 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf36825056;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf36825056 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf13721257;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf13721257 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf33747877;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf33747877 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf91171975;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf91171975 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf74936476;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf74936476 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20990622;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20990622 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf34676987;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf34676987 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf48552803;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf48552803 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf13309344;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf13309344 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf52945124;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf52945124 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf69583738;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf69583738 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20300464;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20300464 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf75514668;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf75514668 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf18984337;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf18984337 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf96239781;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf96239781 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20398246;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20398246 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf67504103;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf67504103 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf54812398;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf54812398 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf35095455;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf35095455 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf29582592;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf29582592 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf6106855;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf6106855 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf16370629;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf16370629 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf32997735;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf32997735 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf12295975;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf12295975 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf67711767;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf67711767 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf63860397;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf63860397 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf67696735;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf67696735 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf75558099;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf75558099 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf94076452;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf94076452 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf64448793;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf64448793 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf31467659;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf31467659 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf26295877;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf26295877 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf27030523;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf27030523 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf21444700;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf21444700 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf81184554;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf81184554 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf96700101;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf96700101 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf32821265;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf32821265 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf94529055;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf94529055 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf18309810;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf18309810 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf38319226;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf38319226 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf80739793;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf80739793 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf45308439;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf45308439 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf86675148;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf86675148 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf27045001;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf27045001 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf40875271;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf40875271 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf293329;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf293329 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf38618197;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf38618197 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf68482757;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf68482757 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf68157546;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf68157546 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf33498962;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf33498962 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf65265610;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf65265610 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf77166624;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf77166624 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf79907112;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf79907112 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf5367985;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf5367985 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf35284105;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf35284105 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20454428;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf20454428 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf80723521;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf80723521 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf21451903;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf21451903 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf23460208;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf23460208 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf11076079;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf11076079 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf53293886;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf53293886 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf59118888;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf59118888 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf54476351;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf54476351 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf48860550;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf48860550 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf21477465;     mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf21477465 = mtFUXJhoScbCnQQYWDniotvqkvCkWslGWVElRObOGmvItfQCZVZaxantxOkcGxRarVfSMf99968097;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void eYMIWMMrlDuoYWcZzOmdghvnzsMnvESQveAbXpLIlKesmnNboyU31500425() {     float fzBNxdZJezZwhkdtuWACJCesWCDB79071508 = 73458911;    float fzBNxdZJezZwhkdtuWACJCesWCDB52562807 = -619779970;    float fzBNxdZJezZwhkdtuWACJCesWCDB41828877 = -165332475;    float fzBNxdZJezZwhkdtuWACJCesWCDB57728630 = -814921810;    float fzBNxdZJezZwhkdtuWACJCesWCDB64859490 = -865164204;    float fzBNxdZJezZwhkdtuWACJCesWCDB62252767 = -322087424;    float fzBNxdZJezZwhkdtuWACJCesWCDB23862405 = -168636460;    float fzBNxdZJezZwhkdtuWACJCesWCDB75812246 = -259067358;    float fzBNxdZJezZwhkdtuWACJCesWCDB42538544 = -395383723;    float fzBNxdZJezZwhkdtuWACJCesWCDB34687670 = -967853011;    float fzBNxdZJezZwhkdtuWACJCesWCDB43460953 = -344740561;    float fzBNxdZJezZwhkdtuWACJCesWCDB74230360 = -743833703;    float fzBNxdZJezZwhkdtuWACJCesWCDB66193630 = -362600009;    float fzBNxdZJezZwhkdtuWACJCesWCDB49909457 = -790889264;    float fzBNxdZJezZwhkdtuWACJCesWCDB66719717 = -615506027;    float fzBNxdZJezZwhkdtuWACJCesWCDB89904110 = -142880485;    float fzBNxdZJezZwhkdtuWACJCesWCDB9303197 = -107270230;    float fzBNxdZJezZwhkdtuWACJCesWCDB10511828 = -835463829;    float fzBNxdZJezZwhkdtuWACJCesWCDB3071063 = -796482339;    float fzBNxdZJezZwhkdtuWACJCesWCDB15366352 = -218036161;    float fzBNxdZJezZwhkdtuWACJCesWCDB42812214 = -624120708;    float fzBNxdZJezZwhkdtuWACJCesWCDB26086724 = -717862215;    float fzBNxdZJezZwhkdtuWACJCesWCDB89452680 = -611398969;    float fzBNxdZJezZwhkdtuWACJCesWCDB48338314 = -283255802;    float fzBNxdZJezZwhkdtuWACJCesWCDB66338294 = -177747750;    float fzBNxdZJezZwhkdtuWACJCesWCDB10443672 = -67205483;    float fzBNxdZJezZwhkdtuWACJCesWCDB20626648 = -834829247;    float fzBNxdZJezZwhkdtuWACJCesWCDB99355170 = -116911317;    float fzBNxdZJezZwhkdtuWACJCesWCDB6859369 = -828415597;    float fzBNxdZJezZwhkdtuWACJCesWCDB51064773 = -851656822;    float fzBNxdZJezZwhkdtuWACJCesWCDB53683475 = 74393983;    float fzBNxdZJezZwhkdtuWACJCesWCDB7584692 = -425596111;    float fzBNxdZJezZwhkdtuWACJCesWCDB77859508 = -216843162;    float fzBNxdZJezZwhkdtuWACJCesWCDB9777238 = -355090170;    float fzBNxdZJezZwhkdtuWACJCesWCDB68867839 = -177275954;    float fzBNxdZJezZwhkdtuWACJCesWCDB27539820 = -66139995;    float fzBNxdZJezZwhkdtuWACJCesWCDB49297586 = -33899729;    float fzBNxdZJezZwhkdtuWACJCesWCDB92839004 = -708228380;    float fzBNxdZJezZwhkdtuWACJCesWCDB42323429 = -540890310;    float fzBNxdZJezZwhkdtuWACJCesWCDB64549300 = -875218761;    float fzBNxdZJezZwhkdtuWACJCesWCDB63193921 = -622281242;    float fzBNxdZJezZwhkdtuWACJCesWCDB57046456 = -622173063;    float fzBNxdZJezZwhkdtuWACJCesWCDB32670980 = -826674696;    float fzBNxdZJezZwhkdtuWACJCesWCDB49630399 = -800167704;    float fzBNxdZJezZwhkdtuWACJCesWCDB12793205 = -380545837;    float fzBNxdZJezZwhkdtuWACJCesWCDB93958025 = -547439674;    float fzBNxdZJezZwhkdtuWACJCesWCDB90981849 = 76477261;    float fzBNxdZJezZwhkdtuWACJCesWCDB56920484 = -93175381;    float fzBNxdZJezZwhkdtuWACJCesWCDB56005188 = -417510267;    float fzBNxdZJezZwhkdtuWACJCesWCDB52864324 = -504964528;    float fzBNxdZJezZwhkdtuWACJCesWCDB78092608 = -367176218;    float fzBNxdZJezZwhkdtuWACJCesWCDB43766903 = -195506819;    float fzBNxdZJezZwhkdtuWACJCesWCDB5879276 = -750348365;    float fzBNxdZJezZwhkdtuWACJCesWCDB7137547 = 72924414;    float fzBNxdZJezZwhkdtuWACJCesWCDB88688211 = -529792919;    float fzBNxdZJezZwhkdtuWACJCesWCDB52984784 = -208678875;    float fzBNxdZJezZwhkdtuWACJCesWCDB63110126 = 91618999;    float fzBNxdZJezZwhkdtuWACJCesWCDB93490563 = -882076674;    float fzBNxdZJezZwhkdtuWACJCesWCDB91390335 = -537174060;    float fzBNxdZJezZwhkdtuWACJCesWCDB54415819 = -697958722;    float fzBNxdZJezZwhkdtuWACJCesWCDB41626119 = -487258178;    float fzBNxdZJezZwhkdtuWACJCesWCDB24507235 = 48274857;    float fzBNxdZJezZwhkdtuWACJCesWCDB68952878 = -430651762;    float fzBNxdZJezZwhkdtuWACJCesWCDB91473770 = -543726901;    float fzBNxdZJezZwhkdtuWACJCesWCDB81004194 = -942246994;    float fzBNxdZJezZwhkdtuWACJCesWCDB35876261 = -919144451;    float fzBNxdZJezZwhkdtuWACJCesWCDB96370851 = -426990541;    float fzBNxdZJezZwhkdtuWACJCesWCDB56416392 = 92490161;    float fzBNxdZJezZwhkdtuWACJCesWCDB81041618 = -513613311;    float fzBNxdZJezZwhkdtuWACJCesWCDB39179898 = -449366032;    float fzBNxdZJezZwhkdtuWACJCesWCDB40606525 = -8980757;    float fzBNxdZJezZwhkdtuWACJCesWCDB16464192 = -399041850;    float fzBNxdZJezZwhkdtuWACJCesWCDB68188399 = -194573519;    float fzBNxdZJezZwhkdtuWACJCesWCDB38521763 = -921263578;    float fzBNxdZJezZwhkdtuWACJCesWCDB52172430 = -595754920;    float fzBNxdZJezZwhkdtuWACJCesWCDB85765758 = 98052355;    float fzBNxdZJezZwhkdtuWACJCesWCDB93415744 = -891187519;    float fzBNxdZJezZwhkdtuWACJCesWCDB39822281 = -811231266;    float fzBNxdZJezZwhkdtuWACJCesWCDB35545110 = -902709965;    float fzBNxdZJezZwhkdtuWACJCesWCDB72380269 = -630308076;    float fzBNxdZJezZwhkdtuWACJCesWCDB19461822 = -43682744;    float fzBNxdZJezZwhkdtuWACJCesWCDB63706163 = -641653867;    float fzBNxdZJezZwhkdtuWACJCesWCDB43349983 = -699401051;    float fzBNxdZJezZwhkdtuWACJCesWCDB53995044 = -223451069;    float fzBNxdZJezZwhkdtuWACJCesWCDB72972165 = -384480605;    float fzBNxdZJezZwhkdtuWACJCesWCDB9916573 = -730099199;    float fzBNxdZJezZwhkdtuWACJCesWCDB1705417 = -675247746;    float fzBNxdZJezZwhkdtuWACJCesWCDB70721962 = -189767576;    float fzBNxdZJezZwhkdtuWACJCesWCDB21089026 = -825297252;    float fzBNxdZJezZwhkdtuWACJCesWCDB15883055 = -968597079;    float fzBNxdZJezZwhkdtuWACJCesWCDB64429693 = -57758994;    float fzBNxdZJezZwhkdtuWACJCesWCDB55807023 = -151823056;    float fzBNxdZJezZwhkdtuWACJCesWCDB1448669 = -71054321;    float fzBNxdZJezZwhkdtuWACJCesWCDB87907609 = -842931589;    float fzBNxdZJezZwhkdtuWACJCesWCDB22923181 = -287960584;    float fzBNxdZJezZwhkdtuWACJCesWCDB38686687 = -570556099;    float fzBNxdZJezZwhkdtuWACJCesWCDB88093577 = -91521301;    float fzBNxdZJezZwhkdtuWACJCesWCDB41197209 = -182947795;    float fzBNxdZJezZwhkdtuWACJCesWCDB68626205 = -857920710;    float fzBNxdZJezZwhkdtuWACJCesWCDB76916943 = 73458911;     fzBNxdZJezZwhkdtuWACJCesWCDB79071508 = fzBNxdZJezZwhkdtuWACJCesWCDB52562807;     fzBNxdZJezZwhkdtuWACJCesWCDB52562807 = fzBNxdZJezZwhkdtuWACJCesWCDB41828877;     fzBNxdZJezZwhkdtuWACJCesWCDB41828877 = fzBNxdZJezZwhkdtuWACJCesWCDB57728630;     fzBNxdZJezZwhkdtuWACJCesWCDB57728630 = fzBNxdZJezZwhkdtuWACJCesWCDB64859490;     fzBNxdZJezZwhkdtuWACJCesWCDB64859490 = fzBNxdZJezZwhkdtuWACJCesWCDB62252767;     fzBNxdZJezZwhkdtuWACJCesWCDB62252767 = fzBNxdZJezZwhkdtuWACJCesWCDB23862405;     fzBNxdZJezZwhkdtuWACJCesWCDB23862405 = fzBNxdZJezZwhkdtuWACJCesWCDB75812246;     fzBNxdZJezZwhkdtuWACJCesWCDB75812246 = fzBNxdZJezZwhkdtuWACJCesWCDB42538544;     fzBNxdZJezZwhkdtuWACJCesWCDB42538544 = fzBNxdZJezZwhkdtuWACJCesWCDB34687670;     fzBNxdZJezZwhkdtuWACJCesWCDB34687670 = fzBNxdZJezZwhkdtuWACJCesWCDB43460953;     fzBNxdZJezZwhkdtuWACJCesWCDB43460953 = fzBNxdZJezZwhkdtuWACJCesWCDB74230360;     fzBNxdZJezZwhkdtuWACJCesWCDB74230360 = fzBNxdZJezZwhkdtuWACJCesWCDB66193630;     fzBNxdZJezZwhkdtuWACJCesWCDB66193630 = fzBNxdZJezZwhkdtuWACJCesWCDB49909457;     fzBNxdZJezZwhkdtuWACJCesWCDB49909457 = fzBNxdZJezZwhkdtuWACJCesWCDB66719717;     fzBNxdZJezZwhkdtuWACJCesWCDB66719717 = fzBNxdZJezZwhkdtuWACJCesWCDB89904110;     fzBNxdZJezZwhkdtuWACJCesWCDB89904110 = fzBNxdZJezZwhkdtuWACJCesWCDB9303197;     fzBNxdZJezZwhkdtuWACJCesWCDB9303197 = fzBNxdZJezZwhkdtuWACJCesWCDB10511828;     fzBNxdZJezZwhkdtuWACJCesWCDB10511828 = fzBNxdZJezZwhkdtuWACJCesWCDB3071063;     fzBNxdZJezZwhkdtuWACJCesWCDB3071063 = fzBNxdZJezZwhkdtuWACJCesWCDB15366352;     fzBNxdZJezZwhkdtuWACJCesWCDB15366352 = fzBNxdZJezZwhkdtuWACJCesWCDB42812214;     fzBNxdZJezZwhkdtuWACJCesWCDB42812214 = fzBNxdZJezZwhkdtuWACJCesWCDB26086724;     fzBNxdZJezZwhkdtuWACJCesWCDB26086724 = fzBNxdZJezZwhkdtuWACJCesWCDB89452680;     fzBNxdZJezZwhkdtuWACJCesWCDB89452680 = fzBNxdZJezZwhkdtuWACJCesWCDB48338314;     fzBNxdZJezZwhkdtuWACJCesWCDB48338314 = fzBNxdZJezZwhkdtuWACJCesWCDB66338294;     fzBNxdZJezZwhkdtuWACJCesWCDB66338294 = fzBNxdZJezZwhkdtuWACJCesWCDB10443672;     fzBNxdZJezZwhkdtuWACJCesWCDB10443672 = fzBNxdZJezZwhkdtuWACJCesWCDB20626648;     fzBNxdZJezZwhkdtuWACJCesWCDB20626648 = fzBNxdZJezZwhkdtuWACJCesWCDB99355170;     fzBNxdZJezZwhkdtuWACJCesWCDB99355170 = fzBNxdZJezZwhkdtuWACJCesWCDB6859369;     fzBNxdZJezZwhkdtuWACJCesWCDB6859369 = fzBNxdZJezZwhkdtuWACJCesWCDB51064773;     fzBNxdZJezZwhkdtuWACJCesWCDB51064773 = fzBNxdZJezZwhkdtuWACJCesWCDB53683475;     fzBNxdZJezZwhkdtuWACJCesWCDB53683475 = fzBNxdZJezZwhkdtuWACJCesWCDB7584692;     fzBNxdZJezZwhkdtuWACJCesWCDB7584692 = fzBNxdZJezZwhkdtuWACJCesWCDB77859508;     fzBNxdZJezZwhkdtuWACJCesWCDB77859508 = fzBNxdZJezZwhkdtuWACJCesWCDB9777238;     fzBNxdZJezZwhkdtuWACJCesWCDB9777238 = fzBNxdZJezZwhkdtuWACJCesWCDB68867839;     fzBNxdZJezZwhkdtuWACJCesWCDB68867839 = fzBNxdZJezZwhkdtuWACJCesWCDB27539820;     fzBNxdZJezZwhkdtuWACJCesWCDB27539820 = fzBNxdZJezZwhkdtuWACJCesWCDB49297586;     fzBNxdZJezZwhkdtuWACJCesWCDB49297586 = fzBNxdZJezZwhkdtuWACJCesWCDB92839004;     fzBNxdZJezZwhkdtuWACJCesWCDB92839004 = fzBNxdZJezZwhkdtuWACJCesWCDB42323429;     fzBNxdZJezZwhkdtuWACJCesWCDB42323429 = fzBNxdZJezZwhkdtuWACJCesWCDB64549300;     fzBNxdZJezZwhkdtuWACJCesWCDB64549300 = fzBNxdZJezZwhkdtuWACJCesWCDB63193921;     fzBNxdZJezZwhkdtuWACJCesWCDB63193921 = fzBNxdZJezZwhkdtuWACJCesWCDB57046456;     fzBNxdZJezZwhkdtuWACJCesWCDB57046456 = fzBNxdZJezZwhkdtuWACJCesWCDB32670980;     fzBNxdZJezZwhkdtuWACJCesWCDB32670980 = fzBNxdZJezZwhkdtuWACJCesWCDB49630399;     fzBNxdZJezZwhkdtuWACJCesWCDB49630399 = fzBNxdZJezZwhkdtuWACJCesWCDB12793205;     fzBNxdZJezZwhkdtuWACJCesWCDB12793205 = fzBNxdZJezZwhkdtuWACJCesWCDB93958025;     fzBNxdZJezZwhkdtuWACJCesWCDB93958025 = fzBNxdZJezZwhkdtuWACJCesWCDB90981849;     fzBNxdZJezZwhkdtuWACJCesWCDB90981849 = fzBNxdZJezZwhkdtuWACJCesWCDB56920484;     fzBNxdZJezZwhkdtuWACJCesWCDB56920484 = fzBNxdZJezZwhkdtuWACJCesWCDB56005188;     fzBNxdZJezZwhkdtuWACJCesWCDB56005188 = fzBNxdZJezZwhkdtuWACJCesWCDB52864324;     fzBNxdZJezZwhkdtuWACJCesWCDB52864324 = fzBNxdZJezZwhkdtuWACJCesWCDB78092608;     fzBNxdZJezZwhkdtuWACJCesWCDB78092608 = fzBNxdZJezZwhkdtuWACJCesWCDB43766903;     fzBNxdZJezZwhkdtuWACJCesWCDB43766903 = fzBNxdZJezZwhkdtuWACJCesWCDB5879276;     fzBNxdZJezZwhkdtuWACJCesWCDB5879276 = fzBNxdZJezZwhkdtuWACJCesWCDB7137547;     fzBNxdZJezZwhkdtuWACJCesWCDB7137547 = fzBNxdZJezZwhkdtuWACJCesWCDB88688211;     fzBNxdZJezZwhkdtuWACJCesWCDB88688211 = fzBNxdZJezZwhkdtuWACJCesWCDB52984784;     fzBNxdZJezZwhkdtuWACJCesWCDB52984784 = fzBNxdZJezZwhkdtuWACJCesWCDB63110126;     fzBNxdZJezZwhkdtuWACJCesWCDB63110126 = fzBNxdZJezZwhkdtuWACJCesWCDB93490563;     fzBNxdZJezZwhkdtuWACJCesWCDB93490563 = fzBNxdZJezZwhkdtuWACJCesWCDB91390335;     fzBNxdZJezZwhkdtuWACJCesWCDB91390335 = fzBNxdZJezZwhkdtuWACJCesWCDB54415819;     fzBNxdZJezZwhkdtuWACJCesWCDB54415819 = fzBNxdZJezZwhkdtuWACJCesWCDB41626119;     fzBNxdZJezZwhkdtuWACJCesWCDB41626119 = fzBNxdZJezZwhkdtuWACJCesWCDB24507235;     fzBNxdZJezZwhkdtuWACJCesWCDB24507235 = fzBNxdZJezZwhkdtuWACJCesWCDB68952878;     fzBNxdZJezZwhkdtuWACJCesWCDB68952878 = fzBNxdZJezZwhkdtuWACJCesWCDB91473770;     fzBNxdZJezZwhkdtuWACJCesWCDB91473770 = fzBNxdZJezZwhkdtuWACJCesWCDB81004194;     fzBNxdZJezZwhkdtuWACJCesWCDB81004194 = fzBNxdZJezZwhkdtuWACJCesWCDB35876261;     fzBNxdZJezZwhkdtuWACJCesWCDB35876261 = fzBNxdZJezZwhkdtuWACJCesWCDB96370851;     fzBNxdZJezZwhkdtuWACJCesWCDB96370851 = fzBNxdZJezZwhkdtuWACJCesWCDB56416392;     fzBNxdZJezZwhkdtuWACJCesWCDB56416392 = fzBNxdZJezZwhkdtuWACJCesWCDB81041618;     fzBNxdZJezZwhkdtuWACJCesWCDB81041618 = fzBNxdZJezZwhkdtuWACJCesWCDB39179898;     fzBNxdZJezZwhkdtuWACJCesWCDB39179898 = fzBNxdZJezZwhkdtuWACJCesWCDB40606525;     fzBNxdZJezZwhkdtuWACJCesWCDB40606525 = fzBNxdZJezZwhkdtuWACJCesWCDB16464192;     fzBNxdZJezZwhkdtuWACJCesWCDB16464192 = fzBNxdZJezZwhkdtuWACJCesWCDB68188399;     fzBNxdZJezZwhkdtuWACJCesWCDB68188399 = fzBNxdZJezZwhkdtuWACJCesWCDB38521763;     fzBNxdZJezZwhkdtuWACJCesWCDB38521763 = fzBNxdZJezZwhkdtuWACJCesWCDB52172430;     fzBNxdZJezZwhkdtuWACJCesWCDB52172430 = fzBNxdZJezZwhkdtuWACJCesWCDB85765758;     fzBNxdZJezZwhkdtuWACJCesWCDB85765758 = fzBNxdZJezZwhkdtuWACJCesWCDB93415744;     fzBNxdZJezZwhkdtuWACJCesWCDB93415744 = fzBNxdZJezZwhkdtuWACJCesWCDB39822281;     fzBNxdZJezZwhkdtuWACJCesWCDB39822281 = fzBNxdZJezZwhkdtuWACJCesWCDB35545110;     fzBNxdZJezZwhkdtuWACJCesWCDB35545110 = fzBNxdZJezZwhkdtuWACJCesWCDB72380269;     fzBNxdZJezZwhkdtuWACJCesWCDB72380269 = fzBNxdZJezZwhkdtuWACJCesWCDB19461822;     fzBNxdZJezZwhkdtuWACJCesWCDB19461822 = fzBNxdZJezZwhkdtuWACJCesWCDB63706163;     fzBNxdZJezZwhkdtuWACJCesWCDB63706163 = fzBNxdZJezZwhkdtuWACJCesWCDB43349983;     fzBNxdZJezZwhkdtuWACJCesWCDB43349983 = fzBNxdZJezZwhkdtuWACJCesWCDB53995044;     fzBNxdZJezZwhkdtuWACJCesWCDB53995044 = fzBNxdZJezZwhkdtuWACJCesWCDB72972165;     fzBNxdZJezZwhkdtuWACJCesWCDB72972165 = fzBNxdZJezZwhkdtuWACJCesWCDB9916573;     fzBNxdZJezZwhkdtuWACJCesWCDB9916573 = fzBNxdZJezZwhkdtuWACJCesWCDB1705417;     fzBNxdZJezZwhkdtuWACJCesWCDB1705417 = fzBNxdZJezZwhkdtuWACJCesWCDB70721962;     fzBNxdZJezZwhkdtuWACJCesWCDB70721962 = fzBNxdZJezZwhkdtuWACJCesWCDB21089026;     fzBNxdZJezZwhkdtuWACJCesWCDB21089026 = fzBNxdZJezZwhkdtuWACJCesWCDB15883055;     fzBNxdZJezZwhkdtuWACJCesWCDB15883055 = fzBNxdZJezZwhkdtuWACJCesWCDB64429693;     fzBNxdZJezZwhkdtuWACJCesWCDB64429693 = fzBNxdZJezZwhkdtuWACJCesWCDB55807023;     fzBNxdZJezZwhkdtuWACJCesWCDB55807023 = fzBNxdZJezZwhkdtuWACJCesWCDB1448669;     fzBNxdZJezZwhkdtuWACJCesWCDB1448669 = fzBNxdZJezZwhkdtuWACJCesWCDB87907609;     fzBNxdZJezZwhkdtuWACJCesWCDB87907609 = fzBNxdZJezZwhkdtuWACJCesWCDB22923181;     fzBNxdZJezZwhkdtuWACJCesWCDB22923181 = fzBNxdZJezZwhkdtuWACJCesWCDB38686687;     fzBNxdZJezZwhkdtuWACJCesWCDB38686687 = fzBNxdZJezZwhkdtuWACJCesWCDB88093577;     fzBNxdZJezZwhkdtuWACJCesWCDB88093577 = fzBNxdZJezZwhkdtuWACJCesWCDB41197209;     fzBNxdZJezZwhkdtuWACJCesWCDB41197209 = fzBNxdZJezZwhkdtuWACJCesWCDB68626205;     fzBNxdZJezZwhkdtuWACJCesWCDB68626205 = fzBNxdZJezZwhkdtuWACJCesWCDB76916943;     fzBNxdZJezZwhkdtuWACJCesWCDB76916943 = fzBNxdZJezZwhkdtuWACJCesWCDB79071508;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void OlRpxjmkbuOKudVBsVJxcfXEF87804784() {     long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR7884381 = -461809513;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR50546041 = -435801088;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR5790406 = -50627225;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR32236332 = -645505593;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR83443587 = -403806005;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR94586659 = -857280040;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR21677573 = -716334435;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR53801014 = -418676943;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR40208888 = -823163242;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR52716046 = -257723925;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR65563186 = -87170436;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR5166338 = -959883797;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43212684 = -52411420;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR41464388 = -860031603;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1156220 = 24311953;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR39449955 = -834769237;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43666054 = -134684881;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR67347735 = -158662373;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR95108973 = -900811673;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR62811738 = -861423204;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR97930015 = -903931466;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR81475917 = 8516883;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR78107905 = -449756548;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR76638055 = -288481794;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1966152 = -976563134;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR46647577 = -237056408;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR2870998 = -505329893;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70331840 = -814785582;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR85671642 = -380334528;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR86674625 = -910840762;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR69800316 = -835114339;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR94770251 = -352031126;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR69672374 = -533707369;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR91532660 = -118121792;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1594690 = -245186905;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR35731866 = 16245255;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR99621897 = -5731258;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR14483113 = -555006468;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR93653035 = -588108542;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR15245706 = -653031403;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR53026043 = -510169341;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR75534890 = -898669305;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR38051959 = -694920045;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR9316698 = -624039267;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR33630123 = 14218915;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR84420371 = -343885173;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR2970835 = -72423552;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR26176315 = -213626146;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR97651118 = -298884680;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR94027510 = -906400285;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR42157484 = -746831710;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR99482477 = -659876912;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR32710819 = -169493663;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR25861764 = -848166144;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR89874768 = -404859477;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR26408464 = -370326397;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR72438135 = -986044541;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR29152350 = -762145432;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR30270180 = -668942459;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR36796011 = -66749597;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR91715661 = -251950147;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR51345733 = -901548853;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR68129371 = 61657584;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR53534262 = -912322481;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR82915729 = -422609586;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70792935 = -735139311;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR35493963 = -326176428;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR51680024 = -934289628;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR39869699 = -514844699;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR65424354 = -991933302;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR39828057 = -729037979;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR29182942 = -579678413;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR73694699 = -570553832;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR79863268 = -147780271;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR9785696 = -251253863;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR22395126 = 94737839;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43423958 = -296563073;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR68791207 = -825717282;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43007933 = -202700710;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR17545781 = -532677962;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43676743 = -64632857;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR76694683 = -191703748;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR72680722 = -415900902;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR91644132 = -473934244;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR44517142 = -64009052;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70317838 = -75237428;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR62059432 = -82537463;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43810610 = -685541225;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1657892 = -713262316;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR75186225 = -874860508;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR63293730 = 2289795;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70469548 = -243585827;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR84212932 = -886064010;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR56857025 = -421358945;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR23530044 = -301081256;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1680311 = -608620488;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR7405520 = -860326889;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR84517696 = -782597565;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR26400969 = -101429681;    long wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR62837187 = -461809513;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR7884381 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR50546041;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR50546041 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR5790406;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR5790406 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR32236332;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR32236332 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR83443587;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR83443587 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR94586659;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR94586659 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR21677573;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR21677573 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR53801014;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR53801014 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR40208888;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR40208888 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR52716046;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR52716046 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR65563186;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR65563186 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR5166338;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR5166338 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43212684;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43212684 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR41464388;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR41464388 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1156220;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1156220 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR39449955;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR39449955 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43666054;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43666054 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR67347735;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR67347735 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR95108973;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR95108973 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR62811738;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR62811738 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR97930015;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR97930015 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR81475917;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR81475917 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR78107905;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR78107905 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR76638055;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR76638055 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1966152;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1966152 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR46647577;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR46647577 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR2870998;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR2870998 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70331840;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70331840 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR85671642;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR85671642 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR86674625;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR86674625 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR69800316;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR69800316 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR94770251;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR94770251 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR69672374;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR69672374 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR91532660;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR91532660 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1594690;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1594690 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR35731866;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR35731866 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR99621897;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR99621897 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR14483113;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR14483113 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR93653035;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR93653035 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR15245706;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR15245706 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR53026043;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR53026043 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR75534890;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR75534890 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR38051959;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR38051959 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR9316698;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR9316698 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR33630123;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR33630123 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR84420371;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR84420371 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR2970835;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR2970835 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR26176315;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR26176315 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR97651118;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR97651118 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR94027510;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR94027510 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR42157484;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR42157484 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR99482477;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR99482477 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR32710819;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR32710819 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR25861764;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR25861764 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR89874768;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR89874768 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR26408464;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR26408464 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR72438135;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR72438135 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR29152350;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR29152350 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR30270180;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR30270180 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR36796011;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR36796011 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR91715661;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR91715661 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR51345733;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR51345733 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR68129371;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR68129371 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR53534262;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR53534262 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR82915729;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR82915729 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70792935;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70792935 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR35493963;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR35493963 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR51680024;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR51680024 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR39869699;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR39869699 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR65424354;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR65424354 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR39828057;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR39828057 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR29182942;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR29182942 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR73694699;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR73694699 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR79863268;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR79863268 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR9785696;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR9785696 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR22395126;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR22395126 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43423958;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43423958 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR68791207;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR68791207 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43007933;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43007933 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR17545781;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR17545781 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43676743;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43676743 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR76694683;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR76694683 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR72680722;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR72680722 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR91644132;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR91644132 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR44517142;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR44517142 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70317838;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70317838 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR62059432;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR62059432 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43810610;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR43810610 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1657892;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1657892 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR75186225;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR75186225 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR63293730;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR63293730 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70469548;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR70469548 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR84212932;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR84212932 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR56857025;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR56857025 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR23530044;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR23530044 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1680311;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR1680311 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR7405520;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR7405520 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR84517696;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR84517696 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR26400969;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR26400969 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR62837187;     wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR62837187 = wpxhIpTUHcEOZdykWKlGVhQYYUDJBeiOMjAtpCTEupmAR7884381;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void LIMfbPvpFUFYgKHNhWuLdHJgPtQVeLlaWMlui79126617() {     double gGZHOUBAslmdjXWTMgDzf15193389 = -859038966;    double gGZHOUBAslmdjXWTMgDzf49465450 = 36791052;    double gGZHOUBAslmdjXWTMgDzf91204911 = 2180492;    double gGZHOUBAslmdjXWTMgDzf30175328 = -723921575;    double gGZHOUBAslmdjXWTMgDzf80147555 = 10401170;    double gGZHOUBAslmdjXWTMgDzf76727663 = -41868284;    double gGZHOUBAslmdjXWTMgDzf82821616 = -580655276;    double gGZHOUBAslmdjXWTMgDzf93701094 = -415978732;    double gGZHOUBAslmdjXWTMgDzf53110063 = -820229054;    double gGZHOUBAslmdjXWTMgDzf99537057 = 9679762;    double gGZHOUBAslmdjXWTMgDzf10718326 = -203002390;    double gGZHOUBAslmdjXWTMgDzf41692904 = -990433339;    double gGZHOUBAslmdjXWTMgDzf63265180 = -552904332;    double gGZHOUBAslmdjXWTMgDzf1083023 = -770562580;    double gGZHOUBAslmdjXWTMgDzf74535562 = -707201012;    double gGZHOUBAslmdjXWTMgDzf42962789 = -372319637;    double gGZHOUBAslmdjXWTMgDzf91504215 = -280442676;    double gGZHOUBAslmdjXWTMgDzf30963878 = -920465778;    double gGZHOUBAslmdjXWTMgDzf90793695 = -668490063;    double gGZHOUBAslmdjXWTMgDzf9415777 = -129378184;    double gGZHOUBAslmdjXWTMgDzf42070899 = -118799955;    double gGZHOUBAslmdjXWTMgDzf10071685 = -40289904;    double gGZHOUBAslmdjXWTMgDzf51745241 = -509409508;    double gGZHOUBAslmdjXWTMgDzf19992594 = -381156285;    double gGZHOUBAslmdjXWTMgDzf27387268 = -378500991;    double gGZHOUBAslmdjXWTMgDzf41593748 = -64714291;    double gGZHOUBAslmdjXWTMgDzf93441524 = -210078877;    double gGZHOUBAslmdjXWTMgDzf4581493 = -478251990;    double gGZHOUBAslmdjXWTMgDzf3356793 = 68518824;    double gGZHOUBAslmdjXWTMgDzf70169631 = -302080653;    double gGZHOUBAslmdjXWTMgDzf64216361 = -573126295;    double gGZHOUBAslmdjXWTMgDzf18012445 = -579761514;    double gGZHOUBAslmdjXWTMgDzf9050389 = -149958711;    double gGZHOUBAslmdjXWTMgDzf27860881 = -627674196;    double gGZHOUBAslmdjXWTMgDzf24973104 = -239729789;    double gGZHOUBAslmdjXWTMgDzf7877931 = -805498162;    double gGZHOUBAslmdjXWTMgDzf14574566 = -613713331;    double gGZHOUBAslmdjXWTMgDzf61240632 = -34994387;    double gGZHOUBAslmdjXWTMgDzf61043024 = 3851927;    double gGZHOUBAslmdjXWTMgDzf38470879 = -277296904;    double gGZHOUBAslmdjXWTMgDzf62104343 = -801438873;    double gGZHOUBAslmdjXWTMgDzf96804250 = -345456619;    double gGZHOUBAslmdjXWTMgDzf77404309 = -680413380;    double gGZHOUBAslmdjXWTMgDzf54894729 = 89177029;    double gGZHOUBAslmdjXWTMgDzf89425337 = -478632216;    double gGZHOUBAslmdjXWTMgDzf45457154 = -897831721;    double gGZHOUBAslmdjXWTMgDzf29658031 = -970110932;    double gGZHOUBAslmdjXWTMgDzf67217236 = -586459129;    double gGZHOUBAslmdjXWTMgDzf91755670 = 54582393;    double gGZHOUBAslmdjXWTMgDzf20242875 = 79351784;    double gGZHOUBAslmdjXWTMgDzf17366467 = -797050910;    double gGZHOUBAslmdjXWTMgDzf8043102 = 91525208;    double gGZHOUBAslmdjXWTMgDzf5415659 = -292407290;    double gGZHOUBAslmdjXWTMgDzf63654246 = -555307471;    double gGZHOUBAslmdjXWTMgDzf37055718 = -817286030;    double gGZHOUBAslmdjXWTMgDzf5121705 = -718749063;    double gGZHOUBAslmdjXWTMgDzf97720208 = -453799440;    double gGZHOUBAslmdjXWTMgDzf71212317 = -616663223;    double gGZHOUBAslmdjXWTMgDzf2788061 = -245420584;    double gGZHOUBAslmdjXWTMgDzf38553807 = -924884540;    double gGZHOUBAslmdjXWTMgDzf83286138 = -831789408;    double gGZHOUBAslmdjXWTMgDzf78240124 = -2403286;    double gGZHOUBAslmdjXWTMgDzf90344301 = -384497556;    double gGZHOUBAslmdjXWTMgDzf82940432 = -418148402;    double gGZHOUBAslmdjXWTMgDzf35320697 = -417193943;    double gGZHOUBAslmdjXWTMgDzf92705881 = -623240876;    double gGZHOUBAslmdjXWTMgDzf32642515 = -740474628;    double gGZHOUBAslmdjXWTMgDzf35404299 = -925230136;    double gGZHOUBAslmdjXWTMgDzf76109919 = -430832791;    double gGZHOUBAslmdjXWTMgDzf66657632 = -901702850;    double gGZHOUBAslmdjXWTMgDzf28388223 = -758606307;    double gGZHOUBAslmdjXWTMgDzf30263583 = -145448290;    double gGZHOUBAslmdjXWTMgDzf69920854 = -824317706;    double gGZHOUBAslmdjXWTMgDzf52322816 = -291193159;    double gGZHOUBAslmdjXWTMgDzf47311433 = -327939311;    double gGZHOUBAslmdjXWTMgDzf45266649 = -773343337;    double gGZHOUBAslmdjXWTMgDzf32667376 = -359876524;    double gGZHOUBAslmdjXWTMgDzf96850512 = -498586537;    double gGZHOUBAslmdjXWTMgDzf30567257 = -902524069;    double gGZHOUBAslmdjXWTMgDzf81930113 = -480669271;    double gGZHOUBAslmdjXWTMgDzf11935718 = -94603360;    double gGZHOUBAslmdjXWTMgDzf26224288 = -623619749;    double gGZHOUBAslmdjXWTMgDzf12825822 = -432834383;    double gGZHOUBAslmdjXWTMgDzf83113918 = 89167039;    double gGZHOUBAslmdjXWTMgDzf52803165 = -505029744;    double gGZHOUBAslmdjXWTMgDzf56173259 = -564651504;    double gGZHOUBAslmdjXWTMgDzf12596787 = -187354225;    double gGZHOUBAslmdjXWTMgDzf45396143 = -594651241;    double gGZHOUBAslmdjXWTMgDzf90805163 = -810388167;    double gGZHOUBAslmdjXWTMgDzf19851400 = -520980727;    double gGZHOUBAslmdjXWTMgDzf10157722 = -251698722;    double gGZHOUBAslmdjXWTMgDzf43362249 = -997050108;    double gGZHOUBAslmdjXWTMgDzf58452572 = -789573803;    double gGZHOUBAslmdjXWTMgDzf22489217 = -71263534;    double gGZHOUBAslmdjXWTMgDzf55184741 = -445507497;    double gGZHOUBAslmdjXWTMgDzf83864219 = -699035587;    double gGZHOUBAslmdjXWTMgDzf6459949 = -960959063;    double gGZHOUBAslmdjXWTMgDzf94463877 = -162264979;    double gGZHOUBAslmdjXWTMgDzf19574032 = -493629028;    double gGZHOUBAslmdjXWTMgDzf96719456 = -859038966;     gGZHOUBAslmdjXWTMgDzf15193389 = gGZHOUBAslmdjXWTMgDzf49465450;     gGZHOUBAslmdjXWTMgDzf49465450 = gGZHOUBAslmdjXWTMgDzf91204911;     gGZHOUBAslmdjXWTMgDzf91204911 = gGZHOUBAslmdjXWTMgDzf30175328;     gGZHOUBAslmdjXWTMgDzf30175328 = gGZHOUBAslmdjXWTMgDzf80147555;     gGZHOUBAslmdjXWTMgDzf80147555 = gGZHOUBAslmdjXWTMgDzf76727663;     gGZHOUBAslmdjXWTMgDzf76727663 = gGZHOUBAslmdjXWTMgDzf82821616;     gGZHOUBAslmdjXWTMgDzf82821616 = gGZHOUBAslmdjXWTMgDzf93701094;     gGZHOUBAslmdjXWTMgDzf93701094 = gGZHOUBAslmdjXWTMgDzf53110063;     gGZHOUBAslmdjXWTMgDzf53110063 = gGZHOUBAslmdjXWTMgDzf99537057;     gGZHOUBAslmdjXWTMgDzf99537057 = gGZHOUBAslmdjXWTMgDzf10718326;     gGZHOUBAslmdjXWTMgDzf10718326 = gGZHOUBAslmdjXWTMgDzf41692904;     gGZHOUBAslmdjXWTMgDzf41692904 = gGZHOUBAslmdjXWTMgDzf63265180;     gGZHOUBAslmdjXWTMgDzf63265180 = gGZHOUBAslmdjXWTMgDzf1083023;     gGZHOUBAslmdjXWTMgDzf1083023 = gGZHOUBAslmdjXWTMgDzf74535562;     gGZHOUBAslmdjXWTMgDzf74535562 = gGZHOUBAslmdjXWTMgDzf42962789;     gGZHOUBAslmdjXWTMgDzf42962789 = gGZHOUBAslmdjXWTMgDzf91504215;     gGZHOUBAslmdjXWTMgDzf91504215 = gGZHOUBAslmdjXWTMgDzf30963878;     gGZHOUBAslmdjXWTMgDzf30963878 = gGZHOUBAslmdjXWTMgDzf90793695;     gGZHOUBAslmdjXWTMgDzf90793695 = gGZHOUBAslmdjXWTMgDzf9415777;     gGZHOUBAslmdjXWTMgDzf9415777 = gGZHOUBAslmdjXWTMgDzf42070899;     gGZHOUBAslmdjXWTMgDzf42070899 = gGZHOUBAslmdjXWTMgDzf10071685;     gGZHOUBAslmdjXWTMgDzf10071685 = gGZHOUBAslmdjXWTMgDzf51745241;     gGZHOUBAslmdjXWTMgDzf51745241 = gGZHOUBAslmdjXWTMgDzf19992594;     gGZHOUBAslmdjXWTMgDzf19992594 = gGZHOUBAslmdjXWTMgDzf27387268;     gGZHOUBAslmdjXWTMgDzf27387268 = gGZHOUBAslmdjXWTMgDzf41593748;     gGZHOUBAslmdjXWTMgDzf41593748 = gGZHOUBAslmdjXWTMgDzf93441524;     gGZHOUBAslmdjXWTMgDzf93441524 = gGZHOUBAslmdjXWTMgDzf4581493;     gGZHOUBAslmdjXWTMgDzf4581493 = gGZHOUBAslmdjXWTMgDzf3356793;     gGZHOUBAslmdjXWTMgDzf3356793 = gGZHOUBAslmdjXWTMgDzf70169631;     gGZHOUBAslmdjXWTMgDzf70169631 = gGZHOUBAslmdjXWTMgDzf64216361;     gGZHOUBAslmdjXWTMgDzf64216361 = gGZHOUBAslmdjXWTMgDzf18012445;     gGZHOUBAslmdjXWTMgDzf18012445 = gGZHOUBAslmdjXWTMgDzf9050389;     gGZHOUBAslmdjXWTMgDzf9050389 = gGZHOUBAslmdjXWTMgDzf27860881;     gGZHOUBAslmdjXWTMgDzf27860881 = gGZHOUBAslmdjXWTMgDzf24973104;     gGZHOUBAslmdjXWTMgDzf24973104 = gGZHOUBAslmdjXWTMgDzf7877931;     gGZHOUBAslmdjXWTMgDzf7877931 = gGZHOUBAslmdjXWTMgDzf14574566;     gGZHOUBAslmdjXWTMgDzf14574566 = gGZHOUBAslmdjXWTMgDzf61240632;     gGZHOUBAslmdjXWTMgDzf61240632 = gGZHOUBAslmdjXWTMgDzf61043024;     gGZHOUBAslmdjXWTMgDzf61043024 = gGZHOUBAslmdjXWTMgDzf38470879;     gGZHOUBAslmdjXWTMgDzf38470879 = gGZHOUBAslmdjXWTMgDzf62104343;     gGZHOUBAslmdjXWTMgDzf62104343 = gGZHOUBAslmdjXWTMgDzf96804250;     gGZHOUBAslmdjXWTMgDzf96804250 = gGZHOUBAslmdjXWTMgDzf77404309;     gGZHOUBAslmdjXWTMgDzf77404309 = gGZHOUBAslmdjXWTMgDzf54894729;     gGZHOUBAslmdjXWTMgDzf54894729 = gGZHOUBAslmdjXWTMgDzf89425337;     gGZHOUBAslmdjXWTMgDzf89425337 = gGZHOUBAslmdjXWTMgDzf45457154;     gGZHOUBAslmdjXWTMgDzf45457154 = gGZHOUBAslmdjXWTMgDzf29658031;     gGZHOUBAslmdjXWTMgDzf29658031 = gGZHOUBAslmdjXWTMgDzf67217236;     gGZHOUBAslmdjXWTMgDzf67217236 = gGZHOUBAslmdjXWTMgDzf91755670;     gGZHOUBAslmdjXWTMgDzf91755670 = gGZHOUBAslmdjXWTMgDzf20242875;     gGZHOUBAslmdjXWTMgDzf20242875 = gGZHOUBAslmdjXWTMgDzf17366467;     gGZHOUBAslmdjXWTMgDzf17366467 = gGZHOUBAslmdjXWTMgDzf8043102;     gGZHOUBAslmdjXWTMgDzf8043102 = gGZHOUBAslmdjXWTMgDzf5415659;     gGZHOUBAslmdjXWTMgDzf5415659 = gGZHOUBAslmdjXWTMgDzf63654246;     gGZHOUBAslmdjXWTMgDzf63654246 = gGZHOUBAslmdjXWTMgDzf37055718;     gGZHOUBAslmdjXWTMgDzf37055718 = gGZHOUBAslmdjXWTMgDzf5121705;     gGZHOUBAslmdjXWTMgDzf5121705 = gGZHOUBAslmdjXWTMgDzf97720208;     gGZHOUBAslmdjXWTMgDzf97720208 = gGZHOUBAslmdjXWTMgDzf71212317;     gGZHOUBAslmdjXWTMgDzf71212317 = gGZHOUBAslmdjXWTMgDzf2788061;     gGZHOUBAslmdjXWTMgDzf2788061 = gGZHOUBAslmdjXWTMgDzf38553807;     gGZHOUBAslmdjXWTMgDzf38553807 = gGZHOUBAslmdjXWTMgDzf83286138;     gGZHOUBAslmdjXWTMgDzf83286138 = gGZHOUBAslmdjXWTMgDzf78240124;     gGZHOUBAslmdjXWTMgDzf78240124 = gGZHOUBAslmdjXWTMgDzf90344301;     gGZHOUBAslmdjXWTMgDzf90344301 = gGZHOUBAslmdjXWTMgDzf82940432;     gGZHOUBAslmdjXWTMgDzf82940432 = gGZHOUBAslmdjXWTMgDzf35320697;     gGZHOUBAslmdjXWTMgDzf35320697 = gGZHOUBAslmdjXWTMgDzf92705881;     gGZHOUBAslmdjXWTMgDzf92705881 = gGZHOUBAslmdjXWTMgDzf32642515;     gGZHOUBAslmdjXWTMgDzf32642515 = gGZHOUBAslmdjXWTMgDzf35404299;     gGZHOUBAslmdjXWTMgDzf35404299 = gGZHOUBAslmdjXWTMgDzf76109919;     gGZHOUBAslmdjXWTMgDzf76109919 = gGZHOUBAslmdjXWTMgDzf66657632;     gGZHOUBAslmdjXWTMgDzf66657632 = gGZHOUBAslmdjXWTMgDzf28388223;     gGZHOUBAslmdjXWTMgDzf28388223 = gGZHOUBAslmdjXWTMgDzf30263583;     gGZHOUBAslmdjXWTMgDzf30263583 = gGZHOUBAslmdjXWTMgDzf69920854;     gGZHOUBAslmdjXWTMgDzf69920854 = gGZHOUBAslmdjXWTMgDzf52322816;     gGZHOUBAslmdjXWTMgDzf52322816 = gGZHOUBAslmdjXWTMgDzf47311433;     gGZHOUBAslmdjXWTMgDzf47311433 = gGZHOUBAslmdjXWTMgDzf45266649;     gGZHOUBAslmdjXWTMgDzf45266649 = gGZHOUBAslmdjXWTMgDzf32667376;     gGZHOUBAslmdjXWTMgDzf32667376 = gGZHOUBAslmdjXWTMgDzf96850512;     gGZHOUBAslmdjXWTMgDzf96850512 = gGZHOUBAslmdjXWTMgDzf30567257;     gGZHOUBAslmdjXWTMgDzf30567257 = gGZHOUBAslmdjXWTMgDzf81930113;     gGZHOUBAslmdjXWTMgDzf81930113 = gGZHOUBAslmdjXWTMgDzf11935718;     gGZHOUBAslmdjXWTMgDzf11935718 = gGZHOUBAslmdjXWTMgDzf26224288;     gGZHOUBAslmdjXWTMgDzf26224288 = gGZHOUBAslmdjXWTMgDzf12825822;     gGZHOUBAslmdjXWTMgDzf12825822 = gGZHOUBAslmdjXWTMgDzf83113918;     gGZHOUBAslmdjXWTMgDzf83113918 = gGZHOUBAslmdjXWTMgDzf52803165;     gGZHOUBAslmdjXWTMgDzf52803165 = gGZHOUBAslmdjXWTMgDzf56173259;     gGZHOUBAslmdjXWTMgDzf56173259 = gGZHOUBAslmdjXWTMgDzf12596787;     gGZHOUBAslmdjXWTMgDzf12596787 = gGZHOUBAslmdjXWTMgDzf45396143;     gGZHOUBAslmdjXWTMgDzf45396143 = gGZHOUBAslmdjXWTMgDzf90805163;     gGZHOUBAslmdjXWTMgDzf90805163 = gGZHOUBAslmdjXWTMgDzf19851400;     gGZHOUBAslmdjXWTMgDzf19851400 = gGZHOUBAslmdjXWTMgDzf10157722;     gGZHOUBAslmdjXWTMgDzf10157722 = gGZHOUBAslmdjXWTMgDzf43362249;     gGZHOUBAslmdjXWTMgDzf43362249 = gGZHOUBAslmdjXWTMgDzf58452572;     gGZHOUBAslmdjXWTMgDzf58452572 = gGZHOUBAslmdjXWTMgDzf22489217;     gGZHOUBAslmdjXWTMgDzf22489217 = gGZHOUBAslmdjXWTMgDzf55184741;     gGZHOUBAslmdjXWTMgDzf55184741 = gGZHOUBAslmdjXWTMgDzf83864219;     gGZHOUBAslmdjXWTMgDzf83864219 = gGZHOUBAslmdjXWTMgDzf6459949;     gGZHOUBAslmdjXWTMgDzf6459949 = gGZHOUBAslmdjXWTMgDzf94463877;     gGZHOUBAslmdjXWTMgDzf94463877 = gGZHOUBAslmdjXWTMgDzf19574032;     gGZHOUBAslmdjXWTMgDzf19574032 = gGZHOUBAslmdjXWTMgDzf96719456;     gGZHOUBAslmdjXWTMgDzf96719456 = gGZHOUBAslmdjXWTMgDzf15193389;}
// Junk Finished
