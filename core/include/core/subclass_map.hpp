/*
Copyright 2022 Roy Awesome's Open Engine (RAOE)

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

#include <concepts>
#include <memory>
#include <typeinfo>
#include <unordered_map>

namespace raoe
{
    template <typename BaseClass>
    class subclass_map
    {
        using TypeInfoRef = std::reference_wrapper<const std::type_info>;

        struct Hasher
        {
            std::size_t operator()(TypeInfoRef code) const { return code.get().hash_code(); }
        };

        struct EqualTo
        {
            bool operator()(TypeInfoRef lhs, TypeInfoRef rhs) const { return lhs.get() == rhs.get(); }
        };

      public:
        subclass_map() = default;

        subclass_map(subclass_map&& other)
            : storage(std::move(std::exchange(other.storage, {})))
        {
        }

        subclass_map& operator=(subclass_map&& other)
        {
            storage = std::move(std::exchange(other.storage, {}));
            return *this;
        }

        /***
         * Insert
         * Insert an type of type T that is derived from the BaseClass, with the forwarded arguments
         * Returns nullptr if T is already in this map, or a non-owning pointer if the object was created
         */
        template <std::derived_from<BaseClass> T, typename... Args>
        std::weak_ptr<T> insert(Args&&... args)
        {
            if(contains<T>())
            {
                return std::weak_ptr<T>();
            }

            auto [itr, success] = storage.emplace(typeid(T), std::make_shared<T>(std::forward<Args>(args)...));
            return success ? std::dynamic_pointer_cast<T>((*itr).second) : std::weak_ptr<T>();
        }

        /***
         * Insert
         * Insert an type of type T that is derived from the BaseClass
         * Returns nullptr if T is already in this map, or a non-owning pointer if the object was created
         */
        template <std::derived_from<BaseClass> T>
        std::weak_ptr<T> insert()
        {
            if(contains<T>())
            {
                return std::weak_ptr<T>();
            }
            auto [itr, success] = storage.emplace(typeid(T), std::make_shared<T>());
            return success ? std::dynamic_pointer_cast<T>((*itr).second) : nullptr;
        }

        /***
         *  Find
         *  Returns a pointer to the stored type, or nullptr if it doesn't exist
         *
         */
        template <std::derived_from<BaseClass> T>
        std::weak_ptr<T> find() const
        {
            auto itr = storage.find(typeid(T));
            return itr != storage.end() ? std::dynamic_pointer_cast<T>((*itr).second) : nullptr;
        }

        std::weak_ptr<BaseClass> find(const std::type_info& type_info)
        {
            auto itr = storage.find(type_info);
            return itr != storage.end() ? (*itr).second : std::weak_ptr<BaseClass>();
        }

        template <std::derived_from<BaseClass> T>
        bool contains() const
        {
            return storage.contains(typeid(T));
        }

        template <std::derived_from<BaseClass> T>
        bool erase()
        {
            return storage.erase(typeid(T)) > 0;
        }

        size_t size() const noexcept { return storage.size(); }

        bool empty() const noexcept { return storage.empty(); }

        void clear() noexcept { storage.clear(); }

      private:
        /* Note on the usage of Unordered map
         * I am well aware that unordered_map is not the most efficient storage structure.  I plan on replacing it
         * The main thing needed from an underlying storage is O(1) lookup (see Find) and the ability to use a hashed
         * type (such as typeid(T)) All the other behavior is useless to me, and replacement should be trivial
         *
         */
        std::unordered_map<TypeInfoRef, std::shared_ptr<BaseClass>, Hasher, EqualTo> storage;

      public:
        auto begin() const { return storage.begin(); }
        auto end() const { return storage.end(); }

        auto cbegin() const { return storage.cbegin(); }
        auto cend() const { return storage.cend(); }
    };
}