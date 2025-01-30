#pragma once

#include <fstream>

class FileLogger {
public:
    FileLogger(std::string_view file_path)
        : s_file(file_path.data(), std::ios::out | std::ios::app)
    {
    }

    void flush()
    {
        s_file.flush();
    }

    template<typename... Args>
    void log(std::string_view fmt_str, Args&&... args)
    {
        if (!s_file.is_open())
            return;

        const auto msg = std::vformat(fmt_str, std::make_format_args(args...));
        s_file << msg << std::endl;
    }

    // @brief operator overload to allow using streams instead
    // @note does not flush - has to be done manually
    template<typename T>
    FileLogger& operator<<(const T& value)
    {
        if (s_file.is_open())
            s_file << value;
        return *this;
    }

private:
    std::ofstream s_file;
};
