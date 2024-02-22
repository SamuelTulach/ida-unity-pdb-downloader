#pragma once

namespace utils
{
    extern std::string last_pdb_path;

    bool load_file_to_memory(const std::string& file_path, std::vector<uint8_t>* out_buffer);
    void guid_to_string(GUID guid, char* output);
    bool extract_cab_file(const std::string& input, const std::string& output);
    bool create_folder(const std::string& path);
}
