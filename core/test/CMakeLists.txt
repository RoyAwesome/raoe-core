cmake_minimum_required(VERSION 3.26)

raoe_add_test(
        NAME core
        CPP_SOURCE_FILES
        "uuid_test.cpp"
        "tag_test.cpp"
        "stream_test.cpp"
        "uri_test.cpp"
        DEPENDENCIES
        raoe::core
)
