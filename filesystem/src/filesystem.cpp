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
#include "fs/filesystem.hpp"

#include <streambuf>

namespace raoe::fs
{
    static inline auto maybe_error(auto value)
    {
        if(!(!!value))
        {
            PHYSFS_ErrorCode ec = PHYSFS_getLastErrorCode();
            const char* error = PHYSFS_getErrorByCode(ec);
            raoe::ensure(false, "Filesystem Error {}: {}", static_cast<int32>(ec), error);
        }
        return value;
    }

    void init_fs(std::string arg0, std::filesystem::path base_path, std::string app_name, std::string org_name)
    {
        // initialize physfs
        maybe_error(PHYSFS_init(arg0.c_str()));

        std::string prefdir = std::string(maybe_error(PHYSFS_getPrefDir(org_name.c_str(), app_name.c_str())));

        if(!prefdir.empty())
        {
            if(maybe_error(PHYSFS_setWriteDir(prefdir.c_str())))
            {
                mount(prefdir, "", false);
            }
        }

        if(base_path != "")
        {
            mount(base_path, "", true);
        }
    }

    void mount(std::filesystem::path path, std::filesystem::path mount_point, bool append_to_search_path)
    {
        maybe_error(PHYSFS_mount(path.c_str(), mount_point.c_str(), append_to_search_path));
    }

    void permit_symlinks(bool allow)
    {
        PHYSFS_permitSymbolicLinks(allow);
    }

    void mkdir(const path& path)
    {
        maybe_error(PHYSFS_mkdir(reinterpret_cast<const char*>(path.c_str())));
    }

    void delete_path(const path& path)
    {
        maybe_error(PHYSFS_delete(reinterpret_cast<const char*>(path.c_str())));
    }

    bool exists(const path& path)
    {
        return !!PHYSFS_exists(reinterpret_cast<const char*>(path.c_str()));
    }

    path_stats stat(const path& path)
    {
        PHYSFS_Stat stats;
        if(!PHYSFS_stat(reinterpret_cast<const char*>(path.c_str()), &stats))
        {
            return path_stats();
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
            return std::filesystem::path();
        }
        return std::filesystem::path(real_path);
    }

    base_physfs_stream::base_physfs_stream(PHYSFS_File* in_file)
        : m_file(in_file)
    {
        raoe::check_if(m_file != nullptr, "Input file is nullptr");
    }
    base_physfs_stream::~base_physfs_stream()
    {
        if(m_file != nullptr)
        {
            PHYSFS_close(m_file);
        }
    }
    std::size_t base_physfs_stream::length()
    {
        if(m_file == nullptr)
        {
            return {};
        }
        return PHYSFS_fileLength(m_file);
    }

    template <std::size_t TBufferSize = 2048>
    class physfs_streambuf : public std::streambuf
    {
      private:
        physfs_streambuf(const physfs_streambuf&) = delete;
        physfs_streambuf& operator=(const physfs_streambuf&) = delete;

        int_type underflow() override
        {
            raoe::check_if(m_file != nullptr, "File is nullptr");

            if(PHYSFS_eof(m_file))
            {
                return traits_type::eof();
            }

            char buff[1];
            std::size_t bytes_read = PHYSFS_readBytes(m_file, m_buffer.data(), TBufferSize);
            if(bytes_read < 1)
            {
                return traits_type::eof();
            }
            setg(m_buffer.data(), m_buffer.data(), m_buffer.data() + bytes_read);
            return (unsigned char)(*gptr());
        }

        pos_type seekoff(off_type pos, std::ios_base::seekdir dir, std::ios_base::openmode mode) override
        {
            raoe::check_if(m_file != nullptr, "File is nullptr");

            switch(dir)
            {
                case std::ios_base::beg: PHYSFS_seek(m_file, pos); break;
                case std::ios_base::cur: PHYSFS_seek(m_file, (PHYSFS_tell(m_file) + pos) - (egptr() - gptr())); break;
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

        pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override
        {
            raoe::check_if(m_file != nullptr, "File is nullptr");

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

        int_type overflow(int_type c = traits_type::eof()) override
        {
            raoe::check_if(m_file != nullptr, "File is nullptr");

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

        int sync() override { return overflow(); }

        PHYSFS_File* m_file = nullptr;
        std::array<char, TBufferSize> m_buffer;

      public:
        physfs_streambuf(PHYSFS_File* in_file)
            : m_file(in_file)
        {
            setg(m_buffer.end(), m_buffer.end(), m_buffer.end());
            setp(m_buffer.data(), m_buffer.end());
        }

        ~physfs_streambuf() { sync(); }
    };

    ifstream::ifstream(path in_path)
        : base_physfs_stream(PHYSFS_openRead(reinterpret_cast<const char*>(in_path.c_str())))
        , std::istream(new physfs_streambuf<>(m_file))
    {
    }

    ifstream::~ifstream()
    {
        delete rdbuf();
    }
    ofstream::ofstream(path in_path, write_mode mode)
        : base_physfs_stream(mode == write_mode::write
                                 ? PHYSFS_openWrite(reinterpret_cast<const char*>(in_path.c_str()))
                                 : PHYSFS_openAppend(reinterpret_cast<const char*>(in_path.c_str())))
        , std::ostream(new physfs_streambuf<>(m_file))
    {
    }
    ofstream::~ofstream()
    {
        delete rdbuf();
    }
}
