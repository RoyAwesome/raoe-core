cmake_minimum_required(VERSION 3.26)

raoe_add_test(
        NAME engine
        CPP_SOURCE_FILES
        "test_assets.cpp"
        DEPENDENCIES
        raoe::engine
        PACK_DIRECTORY
        "packs/"
)
