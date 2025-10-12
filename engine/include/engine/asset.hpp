/*
    Copyright 2022-2025 Roy Awesome's Open Engine (RAOE)

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

#include "engine/engine.hpp"
#include "fs/filesystem.hpp"

#include <expected>
#include <queue>
#include <toml++/toml.hpp>

namespace raoe::engine
{
    template<typename T>
    struct asset_loader
    {
        static_assert(false, "You must specialize asset_loader for your asset type");
    };

    struct asset_load_params
    {
        std::istream& file_stream;
        const raoe::fs::path& file_path;
        const toml::table* metadata = nullptr;
        flecs::world& world;
    };

    struct asset_load_error
    {
        static asset_load_error append(asset_load_error other_error, std::string file, std::string message,
                                       const uint32 line = 0, const uint32 column = 0)
        {
            asset_load_error result(std::move(file), std::move(message), line, column);
            while(!other_error.error_traces.empty())
            {
                result.error_traces.push(other_error.error_traces.front());
                other_error.error_traces.pop();
            }
            return result;
        }

        asset_load_error() = default;

        explicit asset_load_error(std::string file, std::string message, const uint32 line = 0, const uint32 column = 0)
        {
            add_trace(std::move(file), std::move(message), line, column);
        }
        // The path of the asset that failed to load (might be a dependant asset)
        struct error_trace
        {
            uint32 line = 0;
            uint32 column = 0;
            std::string file;
            // A message describing the error
            std::string message;
        };

        // Stack of errors.  Most recent error is on top (should be the asset you've directly loaded).
        std::queue<error_trace> error_traces;
        void add_trace(std::string file, std::string message, const uint32 line = 0, const uint32 column = 0)
        {
            error_traces.push(error_trace {
                .line = line,
                .column = column,
                .file = std::move(file),
                .message = std::move(message),
            });
        }
    };

    template<typename T>
    using asset_load_result = std::expected<T, asset_load_error>;

    template<typename T>
    concept is_asset_type = requires {
        {
            asset_loader<T>::load_asset(std::declval<const asset_load_params&>())
        } -> std::convertible_to<std::expected<T, asset_load_error>>;
    };

    namespace _internal
    {
        struct asset_handle_ref
        {
            int hard_ref_count = 0;
            int weak_ref_count = 0;
        };
    } // namespace _internal

    enum class asset_handle_type
    {
        strong,
        weak,
    };
    template<is_asset_type T, asset_handle_type THandleType = asset_handle_type::strong>
    class asset_handle
    {
        using asset_handle_ref = _internal::asset_handle_ref;

      public:
        template<is_asset_type U, asset_handle_type TOtherHandle>
        friend class asset_handle;

        asset_handle() = default;
        explicit asset_handle(flecs::ref<T> asset)
            requires(THandleType == asset_handle_type::strong)
            : ref(new asset_handle_ref {1, 0})
            , asset(new flecs::ref<T>(std::move(asset)))
        {
        }

        ~asset_handle()
        {
            if(ref)
            {
                if constexpr(THandleType == asset_handle_type::strong)
                {
                    if(--ref->hard_ref_count <= 0)
                    {
                        asset->entity().destruct();
                        delete asset;
                    }
                }
                if constexpr(THandleType == asset_handle_type::weak)
                {
                    --ref->weak_ref_count;
                }

                if(ref->weak_ref_count <= 0 && ref->hard_ref_count <= 0)
                {
                    delete ref;
                }
            }
        }

        asset_handle(const asset_handle<T, asset_handle_type::strong>& other) noexcept
            requires(THandleType == asset_handle_type::strong)
            : ref(other.ref)
            , asset(other.asset)
        {
            if(ref != nullptr)
            {
                ++ref->hard_ref_count;
            }
        }
        asset_handle& operator=(const asset_handle& other) noexcept
            requires(THandleType == asset_handle_type::strong)
        {
            if(this != &other)
            {
                ref = other.ref;
                asset = other.asset;
                if(ref)
                {
                    ++ref->hard_ref_count;
                }
            }
            return *this;
        }

        // ReSharper disable once CppNonExplicitConvertingConstructor
        template<asset_handle_type TOtherHandleType>
        asset_handle(const asset_handle<T, TOtherHandleType>& other) noexcept
            requires(THandleType == asset_handle_type::weak)
            : ref(other.ref)
            , asset(other.asset)
        {
            if(ref)
            {
                ++ref->weak_ref_count;
            }
        }
        template<asset_handle_type TOtherHandleType>
        asset_handle& operator=(const asset_handle<T, TOtherHandleType>& other) noexcept
            requires(THandleType == asset_handle_type::weak)

        {
            ref = const_cast<asset_handle<T, TOtherHandleType>&>(other).ref;
            asset = other.asset;
            if(ref)
            {
                ++ref->weak_ref_count;
            }
            return *this;
        }

        asset_handle(asset_handle&& other) noexcept
            : ref(std::exchange(other.ref, nullptr))
            , asset(std::exchange(other.asset, nullptr))
        {
        }
        asset_handle& operator=(asset_handle&& other) noexcept
        {
            if(this != &other)
            {
                ref = std::exchange(other.ref, nullptr);
                asset = std::exchange(other.asset, nullptr);
            }
            return *this;
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator bool() const noexcept
        {
            return ref != nullptr && ref->hard_ref_count > 0 && asset != nullptr && asset->entity().is_alive();
        }

        [[nodiscard]] bool valid() const noexcept
        {
            return ref != nullptr && ref->hard_ref_count > 0 && asset != nullptr && asset->entity().is_alive();
        }

        [[nodiscard]] T* get() const noexcept
        {
            if(ref && ref->hard_ref_count > 0 && asset)
            {
                return asset->try_get();
            }
            return nullptr;
        }
        T& operator*() const noexcept
        {
            if(auto* val = get())
            {
                return *val;
            }
            panic("asset_handle has nullasset");
        }
        T* operator->() const noexcept { return get(); }

        // ReSharper disable CppRedundantTemplateArguments
        asset_handle<T, asset_handle_type::strong> to_strong() const
            requires(THandleType == asset_handle_type::weak)
        {
            if(ref && ref->hard_ref_count > 0 && asset && asset->entity().is_alive())
            {
                return asset_handle<T, asset_handle_type::strong>(*this);
            }
            return {};
        }
        // ReSharper restore CppRedundantTemplateArguments

      private:
        // Allow conversion from strong to weak.  This is private because to_strong is the public API
        explicit asset_handle(const asset_handle<T, asset_handle_type::weak>& other)
            requires(THandleType == asset_handle_type::strong)
            : ref(other.ref)
            , asset(other.asset)
        {
            if(ref)
            {
                ++ref->hard_ref_count;
            }
        }

        asset_handle_ref* ref = nullptr;
        flecs::ref<T>* asset = nullptr;
    };

    template<typename T>
    using weak_asset_handle = asset_handle<T, asset_handle_type::weak>;

    template<>
    struct asset_loader<std::string>
    {
        static std::string load_asset(const asset_load_params& params)
        {
            std::string content((std::istreambuf_iterator(params.file_stream)), std::istreambuf_iterator<char>());
            return content;
        }
    };

    template<>
    struct asset_loader<toml::table>
    {
        static asset_load_result<toml::table> load_asset(const asset_load_params& params)
        {
            auto result = toml::parse(params.file_stream, params.file_path.string_view());
            if(result)
            {
                return std::expected<toml::table, asset_load_error> {std::move(result.table())};
            }
            return std::unexpected(
                asset_load_error(params.file_path.string(), std::string(result.error().description()),
                                 result.error().source().begin.line, result.error().source().begin.column));
        }
    };

    struct asset_meta
    {
        std::string m_name;
        std::string m_path;

        enum class load_state
        {
            not_loaded,
            loading,
            loaded,
            failed,
        } m_load_state = load_state::not_loaded;
        toml::table m_meta_table;
    };

    template<is_asset_type T>
    std::expected<asset_handle<T>, asset_load_error> load_asset(flecs::world& world, fs::path path)
    {
        world.component<weak_asset_handle<T>>();

        flecs::entity into_entity = world.entity(path.data());
        // Do we already have this entity? If so, return the existing handle
        if(into_entity.has<weak_asset_handle<T>>())
        {
            return into_entity.get<weak_asset_handle<T>>().to_strong();
        }

        if(!fs::exists(path))
        {
            return std::unexpected(asset_load_error(path.string(), "Asset path does not exist"));
        }
        if(!fs::is_regular_file(path))
        {
            return std::unexpected(asset_load_error(path.string(), "Asset path is not a file"));
        }

        fs::ifstream file(path);

        asset_meta meta = {.m_name = std::string(path.stem().string_view()),
                           .m_path = std::string(path.string()),
                           .m_load_state = asset_meta::load_state::loaded};
        if(const fs::path meta_path = path + u8".meta"; fs::exists(meta_path) && fs::is_regular_file(meta_path))
        {
            fs::ifstream meta_file(meta_path);
            if(const auto result = asset_loader<toml::table>::load_asset({
                   .file_stream = meta_file,
                   .file_path = meta_path,
                   .metadata = nullptr,
                   .world = world,
               });
               result.has_value())
            {
                meta.m_meta_table = result.value();
            }
        }
        if(auto load_result = asset_loader<T>::load_asset({
               .file_stream = file,
               .file_path = path,
               .metadata = &meta.m_meta_table,
               .world = world,
           });
           load_result.has_value())
        {
            world.component<T>();
            into_entity.set<T>(std::move(load_result.value()));
            into_entity.set<asset_meta>(std::move(meta));
            auto handle = asset_handle<T>({into_entity});
            into_entity.set<weak_asset_handle<T>>({handle});
            return handle;
        }
        else
        {
            return std::unexpected(load_result.error());
        }
    }
    template<is_asset_type T>
    asset_handle<T> emplace_asset(flecs::world& world, T&& asset, asset_meta meta = {})
    {
        meta.m_load_state = asset_meta::load_state::loaded;
        flecs::entity into_entity = world.entity().set<T>(std::forward<T>(asset)).template set<asset_meta>(meta);
        return asset_handle<T>({into_entity});
    }

    template<>
    struct asset_loader<render::shader::shader>
    {
        static asset_load_result<render::shader::shader> load_asset(const asset_load_params& params);
    };

    template<>
    struct asset_loader<render::shader::material>
    {
        static asset_load_result<render::shader::material> load_asset(const asset_load_params& params);
    };

}

template<>
struct raoe::engine::asset_loader<raoe::render::texture_2d>
{
    static asset_load_result<render::texture_2d> load_asset(const asset_load_params&);
};

template<typename T>
struct std::hash<raoe::engine::asset_handle<T>>
{
    std::size_t operator()(const raoe::engine::asset_handle<T>& asset) const noexcept
    {
        return std::hash<T*>()(asset.get());
    }
};

namespace raoe::render
{
    // Deduction guide to convert asset_handle to generic_handle
    template<typename T, engine::asset_handle_type type>
    generic_handle(engine::asset_handle<T, type>) -> generic_handle<T>;
}

RAOE_CORE_DECLARE_FORMATTER(
    raoe::engine::asset_load_error, switch(value.error_traces.size()) {
        case 0: return std::format_to(ctx.out(), "No error information available");
        case 1: {
            const auto& [line, column, file, message] = value.error_traces.front();
            return std::format_to(ctx.out(), "{} ({}:{}): {}", file, line, column, message);
        }
        default: {
            auto temp_stack = value.error_traces;
            int i = 0;
            while(!temp_stack.empty())
            {
                const auto& [line, column, file, message] = temp_stack.front();
                std::format_to(ctx.out(), "{:3}:{} {} ({}:{}): {}\n", i, i == 0 ? "" : "\t", file, line, column,
                               message);
                temp_stack.pop();
                i++;
            }
            return ctx.out();
        }
    })