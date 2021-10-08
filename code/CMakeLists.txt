add_executable(${PROJECT_NAME}
    code/main.cpp
    code/global.h
    code/global.cpp

    code/tools/RWops.h
    code/tools/RWops.cpp
    code/tools/dumb_doc_json.h
    code/tools/dumb_doc_json.cpp
    code/tools/dumb_stream_json.h
)

#I'm not using this to speed up compilation,
#this just helps clangd to deal with non self-contained headers (but it's bugged in clangd 10.0).
#target_precompile_headers(${PROJECT_NAME} PRIVATE "code/global.h")
