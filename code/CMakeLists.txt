add_executable(${PROJECT_NAME}
    code/main.cpp
    code/global.h
    code/global.cpp

    code/tools/RWops.h
    code/tools/RWops.cpp
    code/tools/dumbjson.h
    code/tools/dumbjson.cpp
)

#I'm not using this to speed up compilation,
#this just helps clangd to deal with non self-contained headers (but it's bugged in clangd 10.0).
#target_precompile_headers(${PROJECT_NAME} PRIVATE "code/global.h")
