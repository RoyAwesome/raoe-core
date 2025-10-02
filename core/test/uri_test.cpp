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

#include "core/uri.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Full URI Parse", "[URI]")
{
    raoe::uri test1("http://user:pass@host:8080/path?query#fragment");
    REQUIRE(test1.scheme() == "http");
    REQUIRE(test1.userinfo() == "user:pass");
    REQUIRE(test1.host() == "host");
    REQUIRE(test1.port() == 8080);
    REQUIRE(test1.path() == "/path");
    REQUIRE(test1.query() == "query");
    REQUIRE(test1.fragment() == "fragment");
}

TEST_CASE("Partial URI Parse, just scheme, host, and path", "[URI]")
{
    raoe::uri test2("ftp://host/path");
    REQUIRE(test2.scheme() == "ftp");
    REQUIRE(test2.userinfo().empty());
    REQUIRE(test2.host() == "host");
    REQUIRE(test2.port() == 0);
    REQUIRE(test2.path() == "/path");
    REQUIRE(test2.query().empty());
    REQUIRE(test2.fragment().empty());
}

TEST_CASE("URI with no authority, just scheme and path", "[URI]")
{
    raoe::uri test3("texture:/path/to/texture.png");
    REQUIRE(test3.scheme() == "texture");
    REQUIRE(test3.userinfo().empty());
    REQUIRE(test3.host().empty());
    REQUIRE(test3.port() == 0);
    REQUIRE(test3.path() == "/path/to/texture.png");
    REQUIRE(test3.query().empty());
    REQUIRE(test3.fragment().empty());
}

TEST_CASE("URI with file scheme and absolute path and empty host", "[URI]")
{
    raoe::uri test4("file:///path/to/file.txt");
    REQUIRE(test4.scheme() == "file");
    REQUIRE(test4.userinfo().empty());
    REQUIRE(test4.host().empty());
    REQUIRE(test4.port() == 0);
    REQUIRE(test4.path() == "/path/to/file.txt");
    REQUIRE(test4.query().empty());
    REQUIRE(test4.fragment().empty());
}
TEST_CASE("URI with custom scheme and userinfo and host only", "[URI]")
{
    raoe::uri test5("custom-scheme://userinfo@hostname");
    REQUIRE(test5.scheme() == "custom-scheme");
    REQUIRE(test5.userinfo() == "userinfo");
    REQUIRE(test5.host() == "hostname");
    REQUIRE(test5.port() == 0);
    REQUIRE(test5.path().empty());
    REQUIRE(test5.query().empty());
    REQUIRE(test5.fragment().empty());
}

TEST_CASE("URI with custom scheme, userinfo, host and port only", "[URI]")
{
    raoe::uri test5("custom-scheme://userinfo@hostname:1234");
    REQUIRE(test5.scheme() == "custom-scheme");
    REQUIRE(test5.userinfo() == "userinfo");
    REQUIRE(test5.host() == "hostname");
    REQUIRE(test5.port() == 1234);
    REQUIRE(test5.path().empty());
    REQUIRE(test5.query().empty());
    REQUIRE(test5.fragment().empty());
}

TEST_CASE("URI with no scheme, just path", "[URI]")
{
    raoe::uri test6("noschemehost/path");
    REQUIRE(test6.scheme().empty());
    REQUIRE(test6.userinfo().empty());
    REQUIRE(test6.host().empty());
    REQUIRE(test6.port() == 0);
    REQUIRE(test6.path() == "noschemehost/path");
    REQUIRE(test6.query().empty());
    REQUIRE(test6.fragment().empty());
}

TEST_CASE("URI with scheme and path and 3 /", "[URI]")
{
    raoe::uri test6("texture:///path");
    REQUIRE(test6.scheme() == "texture");
    REQUIRE(test6.userinfo().empty());
    REQUIRE(test6.host().empty());
    REQUIRE(test6.port() == 0);
    REQUIRE(test6.path() == "/path");
    REQUIRE(test6.query().empty());
    REQUIRE(test6.fragment().empty());
}
