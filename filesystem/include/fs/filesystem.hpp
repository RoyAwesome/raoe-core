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
#pragma once

#include "core/core.hpp"

#include <filesystem>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

struct PHYSFS_File;

namespace raoe::fs
{
    class path
    {
      public:
        path() = default;
        explicit path(std::u8string path)
            : m_underlying(std::move(path))
        {
        }
        explicit path(const std::filesystem::path& path)
            : m_underlying(path.u8string())
        {
        }
        template<typename TChar>
        explicit path(const std::basic_string<TChar>& path)
            : m_underlying(std::u8string(path.begin(), path.end()))
        {
        }

        explicit operator std::u8string_view() const { return std::u8string_view(m_underlying); }

        [[nodiscard]] std::u8string u8string() const { return m_underlying; }
        [[nodiscard]] std::string string() const { return std::string(string_view()); }
        [[nodiscard]] std::filesystem::path filesystem_path() const { return m_underlying; }

        bool operator==(const path& other) const { return m_underlying == other.m_underlying; }
        bool operator!=(const path& other) const { return m_underlying != other.m_underlying; }

        path operator/(const path& other) const { return path(m_underlying + u8"/" + other.m_underlying); }

        path& operator/=(const path& other)
        {
            m_underlying += u8"/" + other.m_underlying;
            return *this;
        }

        path& operator=(const path& other) = default;

        path& operator=(const std::filesystem::path& other)
        {
            m_underlying = other.u8string();
            return *this;
        }

        path& operator=(const std::u8string& other)
        {
            m_underlying = other;
            return *this;
        }

        path operator+(const path& other) const { return path(m_underlying + other.m_underlying); }
        path& operator+=(const path& other)
        {
            m_underlying += other.m_underlying;
            return *this;
        }

        path operator+(const std::u8string& other) const { return path(m_underlying + other); }
        path& operator+=(const std::u8string& other)
        {
            m_underlying += other;
            return *this;
        }

        path operator+(const std::filesystem::path& other) const { return path(m_underlying + other.u8string()); }
        path& operator+=(const std::filesystem::path& other)
        {
            m_underlying += other.u8string();
            return *this;
        }

        std::u8string::value_type operator[](std::size_t pos) const { return m_underlying[pos]; }
        std::u8string::value_type& operator[](std::size_t pos) { return m_underlying[pos]; }

        [[nodiscard]] const char8_t* c_str() const { return m_underlying.c_str(); }
        [[nodiscard]] const char* data() const { return reinterpret_cast<const char*>(m_underlying.data()); }
        [[nodiscard]] std::string_view string_view() const
        {
            return {reinterpret_cast<const char*>(m_underlying.data()), m_underlying.size()};
        }

        class path_iterator
        {
          public:
            explicit path_iterator(const std::u8string& path)
            {
                m_path = path;
                m_current = m_path.find(u8'/');
                m_last = m_path.size();
            }

            static path_iterator make_last(const std::u8string& path)
            {
                path_iterator it(path);
                it.m_current = it.m_last;
                return it;
            }

            path_iterator& operator++()
            {
                if(m_current != std::u8string::npos && m_current < m_last)
                {
                    m_current = m_path.find(u8'/', m_current + 1);
                }
                return *this;
            }

            path_iterator& operator--()
            {
                if(m_current != std::u8string::npos && m_current > 0)
                {
                    m_current = m_path.rfind(u8'/', m_current - 1);
                }
                return *this;
            }

            bool operator==(const path_iterator& other) const { return m_current == other.m_current; }

            bool operator!=(const path_iterator& other) const { return m_current != other.m_current; }

            path operator*() const
            {
                if(m_current == std::u8string::npos)
                {
                    return path(m_path);
                }

                return path(m_path.substr(0, m_current));
            }

          private:
            std::u8string_view m_path;
            std::size_t m_current;
            std::size_t m_last;
        };

        [[nodiscard]] path_iterator begin() const { return path_iterator(m_underlying); }
        [[nodiscard]] path_iterator end() const { return path_iterator::make_last(m_underlying); }

        [[nodiscard]] path stem() const
        {
            // return the filename without the extension
            auto fname = filename();
            auto last = fname.m_underlying.rfind(u8'.');
            if(last == std::u8string::npos)
            {
                return fname;
            }
            return path(fname.m_underlying.substr(0, last));
        }
        [[nodiscard]] path filename() const
        {
            if(m_underlying.empty())
            {
                return {};
            }
            auto last = m_underlying.rfind(u8'/');
            if(last == std::u8string::npos)
            {
                return *this;
            }
            return path(m_underlying.substr(last, m_underlying.size()));
        }
        [[nodiscard]] path parent_path() const
        {
            if(m_underlying.empty())
            {
                return {};
            }
            return path(m_underlying.substr(0, m_underlying.rfind(u8'/')));
        }
        [[nodiscard]] path extension() const
        {
            // return just the extension of the filename
            const auto file_name = filename();
            const auto last = file_name.m_underlying.rfind(u8'.');
            if(last == std::u8string::npos)
            {
                return {};
            }
            return path(file_name.m_underlying.substr(last, file_name.m_underlying.size()));
        }

        [[nodiscard]] std::filesystem::path real_path() const;

      private:
        std::u8string m_underlying;
    };

    enum class fstream_flags : uint8
    {
        good = 0,
        eof = 1 << 0,
        fail = 1 << 1,
        bad = 1 << 2,
    };

    enum class fstream_dir : uint8
    {
        cur,
        end,
        beg,
    };

    class base_physfs_stream
    {
      protected:
        PHYSFS_File* m_file = nullptr;

      public:
        explicit base_physfs_stream(PHYSFS_File* in_file);
        virtual ~base_physfs_stream();
        [[nodiscard]] std::size_t length() const;
    };

    class ifstream final : public base_physfs_stream, public std::istream
    {
      public:
        explicit ifstream(const path& in_path);
        explicit ifstream(const std::string& in_path);
        virtual ~ifstream() override;

        [[nodiscard]] const raoe::fs::path& path() const { return m_in_path; }

      private:
        fs::path m_in_path;
    };

    enum class write_mode : uint8
    {
        write,
        append,
    };

    class ofstream final : public base_physfs_stream, public std::ostream
    {
      public:
        explicit ofstream(const path& in_path, write_mode mode = write_mode::write);
        explicit ofstream(const std::string& in_path, write_mode mode = write_mode::write);
        virtual ~ofstream() override;
    };

    void init_fs(const std::string& arg0, const std::filesystem::path& base_path, const std::string& app_name,
                 const std::string& org_name);
    void mount(const std::filesystem::path& path, const std::filesystem::path& mount_point = "",
               bool append_to_search_path = true);
    void unmount(const std::filesystem::path& path);

    void permit_symlinks(bool allow);

    void mkdir(const path& path);
    void mkdir(const std::string& path);

    void delete_path(const path& path);
    void delete_path(const std::string& path);

    bool exists(const path& path);
    bool exists(const std::string& path);

    const std::vector<std::string>& mountable_file_extensions();

    enum class file_type
    {
        regular,
        directory,
        symlink,
        other,
    };

    struct path_stats
    {
        int64 size = 0;
        int64 mod_time = 0;
        int64 create_time = 0;
        int64 access_time = 0;
        file_type type = file_type::other;
        bool read_only = false;
    };

    path_stats stat(const path& path);
    [[nodiscard]] inline bool is_directory(const path& path)
    {
        return stat(path).type == file_type::directory;
    }
    [[nodiscard]] inline bool is_regular_file(const path& path)
    {
        return stat(path).type == file_type::regular;
    }
    [[nodiscard]] inline bool is_symlink(const path& path)
    {
        return stat(path).type == file_type::symlink;
    }

    path_stats stat(const std::string& path);
    [[nodiscard]] inline bool is_directory(const std::string& path)
    {
        return stat(path).type == file_type::directory;
    }
    [[nodiscard]] inline bool is_regular_file(const std::string& path)
    {
        return stat(path).type == file_type::regular;
    }
    [[nodiscard]] inline bool is_symlink(const std::string& path)
    {
        return stat(path).type == file_type::symlink;
    }
}

template<>
struct std::hash<raoe::fs::path>
{
    std::size_t operator()(const raoe::fs::path& path) const noexcept
    {
        return std::hash<std::u8string> {}(path.u8string());
    }
};

RAOE_CORE_DECLARE_FORMATTER(raoe::fs::path,
                            return format_to(ctx.out(), "{}", reinterpret_cast<const char*>(value.c_str()));)