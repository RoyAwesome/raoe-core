cmake_minimum_required(VERSION 3.26)

raoe_add_test(
        NAME renderer
        CPP_SOURCE_FILES
        "shader_preprocessor_test.cpp"
        DEPENDENCIES
        raoe::core
        raoe::renderer
)
