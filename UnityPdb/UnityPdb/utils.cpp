#include "global.hpp"

bool utils::load_file_to_memory(const std::string& file_path, std::vector<uint8_t>* out_buffer)
{
	std::ifstream file_ifstream(file_path, std::ios::binary);

	if (!file_ifstream)
		return false;

	out_buffer->assign((std::istreambuf_iterator<char>(file_ifstream)), std::istreambuf_iterator<char>());
	file_ifstream.close();

	return true;
}

void utils::guid_to_string(const GUID guid, char* output)
{
	wsprintfA(output, "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	for (char* p = output; *p; ++p) 
		*p = static_cast<char>(toupper(*p));
}

std::string utils::last_pdb_path;
UINT CALLBACK extract_callback(const PVOID context, const UINT notification, const UINT_PTR param1, UINT_PTR param2)
{
    const char* destination_path = static_cast<const char*>(context);

    if (notification == SPFILENOTIFY_FILEINCABINET) 
	{
        FILE_IN_CABINET_INFO_A* info = reinterpret_cast<FILE_IN_CABINET_INFO_A*>(param1);
        strcpy_s(info->FullTargetName, MAX_PATH, destination_path);
        strcat_s(info->FullTargetName, MAX_PATH, "\\");
        strcat_s(info->FullTargetName, MAX_PATH, info->NameInCabinet);

		msg("[unity-pdb] Extracted %s\n", info->FullTargetName);
		utils::last_pdb_path = info->FullTargetName;

        return FILEOP_DOIT;
    }

    return NO_ERROR;
}

bool utils::extract_cab_file(const std::string& input, const std::string& output)
{
	create_folder(output.c_str());
    return SetupIterateCabinetA(input.c_str(), 0, extract_callback, PVOID(output.c_str()));
}

bool utils::create_folder(const std::string& path)
{
    if (CreateDirectoryA(path.c_str(), nullptr) || ERROR_ALREADY_EXISTS == GetLastError())
        return true;

    return false;
}