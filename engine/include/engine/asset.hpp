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

namespace raoe::engine
{
    enum class asset_handle_type
    {
        strong,
        weak,
    };
    template<typename T, asset_handle_type THandleType = asset_handle_type::strong>
    class asset_handle
    {
      public:
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
                    ++ref->ref_count;
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
            ref = other.ref;
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
            if(ref && ref->hard_ref_count > 0 && asset && asset->is_alive())
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

        struct asset_handle_ref
        {
            int32 hard_ref_count = 0;
            int32 weak_ref_count = 0;
        };
        asset_handle_ref* ref = nullptr;
        flecs::ref<T>* asset = nullptr;
    };

    template<typename T>
    using weak_asset_handle = asset_handle<T, asset_handle_type::weak>;
}