#include "global.hpp"

typedef struct _IMAGE_DEBUG_DIRECTORY_RAW
{
    char Format[4];
    GUID Guid;
    DWORD Age;
    char Name[255];
} IMAGE_DEBUG_DIRECTORY_RAW, * PIMAGE_DEBUG_DIRECTORY_RAW;

plugmod_t* idaapi init()
{
    msg("[unity-pdb] Plugin loaded\n");
    return PLUGIN_KEEP;
}

void idaapi term()
{
    /**/
}

bool idaapi run(size_t arg)
{
    char file_path[QMAXPATH];
    get_input_file_path(file_path, sizeof(file_path));

    msg("[unity-pdb] Reading file %s...\n", file_path);
    std::vector<uint8_t> file_buffer;
    bool status = utils::load_file_to_memory(file_path, &file_buffer);
    if (!status)
    {
        msg("[unity-pdb] Failed to read file\n");
        return false;
    }

    msg("[unity-pdb] Read %u bytes\n", file_buffer.size());

    msg("[unity-pdb] Parsing PE headers...\n");
    PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(file_buffer.data());
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
    {
        msg("[unity-pdb] Invalid DOS signature\n");
        return false;
    }

    PIMAGE_NT_HEADERS64 nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS64>(file_buffer.data() + dos_header->e_lfanew);
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
    {
        msg("[unity-pdb] Invalid NT signature\n");
        return false;
    }

    if (nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        msg("[unity-pdb] File is not 64-bit\n");
        return false;
    }

    IMAGE_DATA_DIRECTORY debug_entry = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    if (!debug_entry.Size)
    {
        msg("[unity-pdb] File does not have debug information\n");
        return false;
    }

    msg("[unity-pdb] IMAGE_DIRECTORY_ENTRY_DEBUG has %u bytes\n", debug_entry.Size);

    msg("[unity-pdb] Parsing sections...\n");
    PIMAGE_DEBUG_DIRECTORY debug_directory = nullptr;
    PIMAGE_SECTION_HEADER current_image_section = IMAGE_FIRST_SECTION(nt_headers);
    for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++)
    {
        if (current_image_section[i].VirtualAddress <= debug_entry.VirtualAddress
            && (current_image_section[i].VirtualAddress + current_image_section[i].SizeOfRawData) > debug_entry.VirtualAddress)
        {
            debug_directory = reinterpret_cast<PIMAGE_DEBUG_DIRECTORY>(file_buffer.data() + debug_entry.VirtualAddress - current_image_section[i].
                VirtualAddress + current_image_section[i].PointerToRawData);
        }
    }

    if (!debug_directory)
    {
        msg("[unity-pdb] File does not have debug directory\n");
        return false;
    }

    if (debug_directory->Type != 2)
    {
        msg("[unity-pdb] File has invalid debug directory type\n");
        return false;
    }

    msg("[unity-pdb] Parsing debug info...\n");
    PIMAGE_DEBUG_DIRECTORY_RAW raw_info = reinterpret_cast<PIMAGE_DEBUG_DIRECTORY_RAW>(file_buffer.data() + debug_directory->PointerToRawData);

    char* guid = static_cast<char*>(malloc(128));
    utils::guid_to_string(raw_info->Guid, guid);

    std::string path_name = std::string(raw_info->Name);
    path_name = path_name.substr(path_name.find_last_of("/\\") + 1);
    path_name = path_name.substr(0, path_name.find_last_of('.'));

    msg("[unity-pdb] PDB name: %s\n", path_name.c_str());
    msg("[unity-pdb] PDB guid: %s\n", guid);
    msg("[unity-pdb] PDB age: %u\n", raw_info->Age);

    char* target_url = static_cast<char*>(malloc(512));
    wsprintfA(target_url, "http://symbolserver.unity3d.com/%s.pdb/%s%x/%s.pd_", path_name.c_str(), guid, raw_info->Age, path_name.c_str());

    msg("[unity-pdb] Downloading from URL %s...\n", target_url);

    constexpr const char* pdb_path = "unity_pdb.cab";
    HRESULT result = URLDownloadToFileA(nullptr, target_url, pdb_path, 0, nullptr);
    if (result != S_OK)
    {
        msg("[unity-pdb] Failed to download PDB file\n");
        return false;
    }

    msg("[unity-pdb] CAB file downloaded\n");

    msg("[unity-pdb] Extracting CAB file...\n");
    status = utils::extract_cab_file(pdb_path, "unity_pdb");
    if (!status)
    {
        msg("[unity-pdb] Failed to extract CAB file\n");
        return false;
    }

    msg("[unity-pdb] Run File->Load file->PDB file and load %s\n", utils::last_pdb_path.c_str());

    return true;
}

__declspec(dllexport) plugin_t PLUGIN = 
{
    IDP_INTERFACE_VERSION,
    PLUGIN_PROC,
    init,
    term,
    run,
    "Download symbols (PDB) for Unity game engine files",
    "Check Edit/Plugins/UnityPdb",
    "UnityPdb",
    nullptr
};