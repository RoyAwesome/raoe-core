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
        const char* mount_path = nullptr;
        if(!mount_point.empty())
        {
            mount_path = mount_point.c_str();
        }
        maybe_error(PHYSFS_mount(path.c_str(), mount_path, append_to_search_path));
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

    ifstream::ifstream(path p, bool buffer)
        : base_fstream()
    {
        open(p, buffer);
    }

    ifstream& ifstream::open(path p, bool buffer)
    {
        if(m_file)
        {
            close();
        }

        m_flags = fstream_flags::good;

        m_file = PHYSFS_openRead(reinterpret_cast<const char*>(p.c_str()));

        if(!m_file)
        {
            set_error_bit(fstream_flags::fail);
            return *this;
        }

        if(buffer)
        {
            PHYSFS_Stat stats;
            if(!PHYSFS_stat(reinterpret_cast<const char*>(p.c_str()), &stats))
            {
                set_error_bit(fstream_flags::fail);
                return *this;
            }
            if(!PHYSFS_setBuffer(m_file, stats.filesize))
            {
                set_error_bit(fstream_flags::fail);
                return *this;
            }
        }

        return *this;
    }
    base_fstream::~base_fstream()
    {
        close();
    }
    void base_fstream::close()
    {
        if(m_file)
        {
            if(!PHYSFS_close(m_file))
            {
                set_error_bit(fstream_flags::bad, false);
            }
            m_file = nullptr;
        }
    }
    ifstream& ifstream::read(std::byte* buffer, std::size_t size)
    {
        if(m_file)
        {
            m_last_rw_len = PHYSFS_readBytes(m_file, buffer, size);
            if(m_last_rw_len == -1)
            {
                set_error_bit(fstream_flags::fail);
                return *this;
            }
            m_current_position = PHYSFS_tell(m_file);
            if(m_current_position == -1)
            {
                set_error_bit(fstream_flags::fail);
                return *this;
            }
            PHYSFS_eof(m_file) ? m_flags |= fstream_flags::eof : m_flags &= ~fstream_flags::eof;
        }
        return *this;
    }

    void base_fstream::seekg_internal(int64 pos, fstream_dir dir)
    {
        if(m_file)
        {
            int64 new_pos = 0;
            if(dir == fstream_dir::cur)
            {
                new_pos = m_current_position + pos;
            }
            else if(dir == fstream_dir::end)
            {

                new_pos = PHYSFS_fileLength(m_file) - pos;
            }
            else
            {
                new_pos = pos;
            }

            if(!PHYSFS_seek(m_file, new_pos))
            {
                set_error_bit(fstream_flags::fail);
                return;
            }

            m_current_position = PHYSFS_tell(m_file);
            if(m_current_position == -1)
            {
                set_error_bit(fstream_flags::fail);
                return;
            }
        }
    }

    ifstream& ifstream::seekg(int64 pos, fstream_dir dir)
    {
        seekg_internal(pos, dir);
        return *this;
    }
    bool base_fstream::sync()
    {
        if(m_file)
        {
            if(!PHYSFS_flush(m_file))
            {
                set_error_bit(fstream_flags::fail);
                return false;
            }
        }
        return true;
    }
    void base_fstream::set_error_bit(fstream_flags flag, bool should_close)
    {
        m_flags |= flag;
        if(should_close)
        {
            close();
        }

        PHYSFS_ErrorCode ec = PHYSFS_getLastErrorCode();
        const char* error = PHYSFS_getErrorByCode(ec);

        raoe::ensure(false, "Filesystem Error {}: {}", static_cast<int32>(ec), error);
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
    ofstream::ofstream(path p, bool append, bool buffer)
    {
        open(p, append, buffer);
    }
    ofstream& ofstream::open(path p, bool append, bool buffer)
    {
        if(m_file)
        {
            close();
        }

        m_flags = fstream_flags::good;

        if(append)
        {
            m_file = PHYSFS_openAppend(reinterpret_cast<const char*>(p.c_str()));
        }
        else
        {
            m_file = PHYSFS_openWrite(reinterpret_cast<const char*>(p.c_str()));
        }

        if(!m_file)
        {
            set_error_bit(fstream_flags::fail);
            return *this;
        }

        if(buffer)
        {
            PHYSFS_Stat stats;
            if(!PHYSFS_stat(reinterpret_cast<const char*>(p.c_str()), &stats))
            {
                set_error_bit(fstream_flags::fail);
                return *this;
            }
            if(!PHYSFS_setBuffer(m_file, stats.filesize))
            {
                set_error_bit(fstream_flags::fail);
                return *this;
            }
        }

        return *this;
    }
    ofstream& ofstream::write(const std::byte* buffer, std::size_t size)
    {
        if(m_file)
        {
            m_last_rw_len = PHYSFS_writeBytes(m_file, buffer, size);
            if(m_last_rw_len == -1)
            {
                set_error_bit(fstream_flags::fail);
                return *this;
            }
            m_current_position = PHYSFS_tell(m_file);
            if(m_current_position == -1)
            {
                set_error_bit(fstream_flags::fail);
                return *this;
            }
        }
        return *this;
    }
    ofstream& ofstream::seekp(int64 pos, fstream_dir dir)
    {
        seekg_internal(pos, dir);
        return *this;
    }
}