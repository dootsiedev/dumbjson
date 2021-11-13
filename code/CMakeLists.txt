add_executable(${PROJECT_NAME}
    code/main.cpp
    code/global.h
    code/global.cpp

    code/tools/RWops.h
    code/tools/RWops.cpp
    #code/tools/dumb_json.h
    #code/tools/dumb_json.cpp
    #code/tools/kson_serializer.h

    code/tools/BS_archive.h
    code/tools/BS_binary.h
    code/tools/BS_json.h
    code/tools/BS_json.cpp
    code/tools/BS_stream.h
    code/tools/BS_stream.cpp
)

#I'm not using this to speed up compilation,
#this just helps clangd to deal with non self-contained headers (but it's bugged in clangd 10.0).
#target_precompile_headers(${PROJECT_NAME} PRIVATE "code/global.h")
