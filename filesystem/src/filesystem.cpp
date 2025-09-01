/*
Copyright 2022-2024 Roy Awesome's Open Engine (RAOE)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "fs/filesystem.hpp"
#include "physfs.h"

#include <streambuf>

namespace raoe::fs
{
    static auto maybe_error(auto value)
    {
        if(!!!value)
        {
            const PHYSFS_ErrorCode ec = PHYSFS_getLastErrorCode();
            const char* error = PHYSFS_getErrorByCode(ec);
            ensure(false, "Filesystem Error {}: {}", static_cast<int32>(ec), error);
        }
        return value;
    }

    void init_fs(const std::string& arg0, const std::filesystem::path& base_path, const std::string& app_name,
                 const std::string& org_name)
    {
        // initialize physfs
        maybe_error(PHYSFS_init(arg0.c_str()));

        const auto preference_dir = std::string(maybe_error(PHYSFS_getPrefDir(org_name.c_str(), app_name.c_str())));

        if(!preference_dir.empty())
        {
            if(maybe_error(PHYSFS_setWriteDir(preference_dir.c_str())))
            {
                mount(preference_dir, "", false);
            }
        }

        if(base_path != "")
        {
            mount(base_path, "", true);
        }
    }

    void mount(const std::filesystem::path& path, const std::filesystem::path& mount_point,
               const bool append_to_search_path)
    {
        maybe_error(PHYSFS_mount(path.c_str(), mount_point.c_str(), append_to_search_path));
    }
    void unmount(const std::filesystem::path& path)
    {
        maybe_error(PHYSFS_unmount(path.c_str()));
    }

    void permit_symlinks(const bool allow)
    {
        PHYSFS_permitSymbolicLinks(allow);
    }

    void mkdir(const path& path)
    {
        maybe_error(PHYSFS_mkdir(reinterpret_cast<const char*>(path.c_str())));
    }
    void mkdir(const std::string& path)
    {
        maybe_error(PHYSFS_mkdir(path.c_str()));
    }

    void delete_path(const path& path)
    {
        maybe_error(PHYSFS_delete(reinterpret_cast<const char*>(path.c_str())));
    }
    void delete_path(const std::string& path)
    {
        maybe_error(PHYSFS_delete(path.c_str()));
    }

    bool exists(const path& path)
    {
        return !!PHYSFS_exists(reinterpret_cast<const char*>(path.c_str()));
    }
    bool exists(const std::string& path)
    {
        return !!PHYSFS_exists(path.c_str());
    }
    const std::vector<std::string>& mountable_file_extensions()
    {
        static std::optional<std::vector<std::string>> extensions;
        if(!extensions.has_value())
        {
            extensions.emplace();
            for(const PHYSFS_ArchiveInfo** i = PHYSFS_supportedArchiveTypes(); *i != nullptr; i++)
            {
                extensions->emplace_back((*i)->extension);
            }
        }
        return *extensions;
    }

    path_stats stat(const path& path)
    {
        PHYSFS_Stat stats;
        if(!PHYSFS_stat(reinterpret_cast<const char*>(path.c_str()), &stats))
        {
            return {};
        }
        return path_stats {
            stats.filesize,  stats.modtime, stats.createtime, stats.accesstime, static_cast<file_type>(stats.filetype),
            !!stats.readonly};
    }
    path_stats stat(const std::string& path)
    {
        PHYSFS_Stat stats;
        if(!PHYSFS_stat(path.c_str(), &stats))
        {
            return {};
        }
        return path_stats {
            stats.filesize,  stats.modtime, stats.createtime, stats.accesstime, static_cast<file_type>(stats.filetype),
            !!stats.readonly};
    }

    std::filesystem::path path::real_path() const
    {
        const char* real_path = PHYSFS_getRealDir(reinterpret_cast<const char*>(m_underlying.c_str()));
        if(real_path == nullptr)
        {
            return {};
        }
        return std::filesystem::path(real_path);
    }

    base_physfs_stream::base_physfs_stream(PHYSFS_File* in_file)
        : m_file(in_file)
    {
        check_if(m_file != nullptr, "Input file is nullptr: Error {}",
                 PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }
    base_physfs_stream::~base_physfs_stream()
    {
        if(m_file != nullptr)
        {
            PHYSFS_close(m_file);
        }
    }
    std::size_t base_physfs_stream::length() const
    {
        if(m_file == nullptr)
        {
            return {};
        }
        return PHYSFS_fileLength(m_file);
    }

    template<std::size_t TBufferSize = 2048>
    class physfs_streambuf final : public std::streambuf
    {

        virtual int_type underflow() override
        {
            check_if(m_file != nullptr, "File is nullptr");

            if(PHYSFS_eof(m_file))
            {
                return traits_type::eof();
            }

            std::size_t bytes_read = PHYSFS_readBytes(m_file, m_buffer.data(), TBufferSize);
            if(bytes_read < 1)
            {
                return traits_type::eof();
            }
            setg(m_buffer.data(), m_buffer.data(), m_buffer.data() + bytes_read);
            return static_cast<unsigned char>(*gptr());
        }

        virtual pos_type seekoff(const off_type pos, const std::ios_base::seekdir dir,
                                 const std::ios_base::openmode mode) override
        {
            check_if(m_file != nullptr, "File is nullptr");

            switch(dir)
            {
                case std::ios_base::beg: PHYSFS_seek(m_file, pos); break;
                case std::ios_base::cur: PHYSFS_seek(m_file, PHYSFS_tell(m_file) + pos - (egptr() - gptr())); break;
                case std::ios_base::end: PHYSFS_seek(m_file, PHYSFS_fileLength(m_file) + pos); break;
                default: break;
            }
            if(mode & std::ios_base::in)
            {
                setg(egptr(), egptr(), egptr());
            }
            if(mode & std::ios_base::out)
            {
                setp(m_buffer.data(), m_buffer.data());
            }
            return PHYSFS_tell(m_file);
        }

        virtual pos_type seekpos(const pos_type pos, const std::ios_base::openmode mode) override
        {
            check_if(m_file != nullptr, "File is nullptr");

            PHYSFS_seek(m_file, pos);
            if(mode & std::ios_base::in)
            {
                setg(egptr(), egptr(), egptr());
            }
            if(mode & std::ios_base::out)
            {
                setp(m_buffer.data(), m_buffer.data());
            }
            return PHYSFS_tell(m_file);
        }

        virtual int_type overflow(const int_type c) override
        {
            check_if(m_file != nullptr, "File is nullptr");

            if(pptr() == pbase() && c == traits_type::eof())
            {
                return {};
            }
            if(c != traits_type::eof())
            {
                if(PHYSFS_writeBytes(m_file, &c, 1) < 1)
                {
                    return traits_type::eof();
                }
            }
            return {};
        }

        virtual int sync() override { return overflow(traits_type::eof()); }

        PHYSFS_File* m_file = nullptr;
        std::array<char, TBufferSize> m_buffer;

      public:
        explicit physfs_streambuf(PHYSFS_File* in_file)
            : m_file(in_file)
        {
            setg(m_buffer.end(), m_buffer.end(), m_buffer.end());
            setp(m_buffer.data(), m_buffer.end());
        }

        virtual ~physfs_streambuf() override { physfs_streambuf::sync(); }
        physfs_streambuf(const physfs_streambuf&) = delete;
        physfs_streambuf& operator=(const physfs_streambuf&) = delete;
    };

    ifstream::ifstream(const fs::path& in_path)
        : base_physfs_stream(PHYSFS_openRead(reinterpret_cast<const char*>(in_path.c_str())))
        , std::istream(new physfs_streambuf(m_file))
        , m_in_path(in_path)
    {
    }
    ifstream::ifstream(const std::string& in_path)
        : base_physfs_stream(PHYSFS_openRead(in_path.c_str()))
        , std::istream(new physfs_streambuf(m_file))
        , m_in_path(in_path)
    {
    }

    ifstream::~ifstream()
    {
        delete rdbuf();
    }
    ofstream::ofstream(const path& in_path, const write_mode mode)
        : base_physfs_stream(mode == write_mode::write
                                 ? PHYSFS_openWrite(reinterpret_cast<const char*>(in_path.c_str()))
                                 : PHYSFS_openAppend(reinterpret_cast<const char*>(in_path.c_str())))
        , std::ostream(new physfs_streambuf(m_file))
    {
    }
    ofstream::ofstream(const std::string& in_path, const write_mode mode)
        : base_physfs_stream(mode == write_mode::write ? PHYSFS_openWrite(in_path.c_str())
                                                       : PHYSFS_openAppend(in_path.c_str()))
        , std::ostream(new physfs_streambuf(m_file))
    {
    }
    ofstream::~ofstream()
    {
        delete rdbuf();
    }
}
