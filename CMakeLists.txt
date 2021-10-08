cmake_minimum_required(VERSION 3.16)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the source code and call cmake from there")
endif()

project (dumbjson CXX)

set(CMAKE_CXX_STANDARD 17)


#This is a hack, uses a weird regex to copy libraries into the /bin dir.
#this is a command that shows the rpath: readelf -d exe |head -30
option(PORTABLE "Helps bundle DLL's when you install, also sets rpath on linux" OFF)

#usually you don't really have control over static linking, SDL is an exception.
option(STATIC_BUILD "prefer static libraries" OFF)


#for clang-cl
#clang-cl.exe acts exactly like MSVC (drag-n-drop replacement), 
#but you can use clang++.exe which can generate exactly the same binary,
#but with different command line syntax (and MSVC=FALSE and GNUC=TRUE).
#so MY_CLANGCL means both MSVC and the GNUC.
if("x${CMAKE_CXX_COMPILER_ID}" MATCHES "x.*Clang")
    if("x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
        #called MY_ because I don't want it to clash with anything
        set(MY_CLANGCL TRUE)
    endif()
endif()



#
# flags start (must be done after FetchContent_Declare or else you get warnings injected into the projects, somehow)
#

#also includes defines.
set(MY_WARNING_FLAGS "")

if(MY_CLANGCL)
    #if I ever needed to increase the stack size on clang-cl...
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker /STACK:10000000")
endif()

if (MSVC)
    #I don't like MSVC, but I like to just check if it even runs.

    #keep asserts for reldeb
    string( REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")


    set(MY_WARNING_FLAGS ${MY_WARNING_FLAGS} /wd4244) # C4244: 'conversion_type': conversion from 'type1' to 'type2', possible loss of data
    set(MY_WARNING_FLAGS ${MY_WARNING_FLAGS} /wd4068) # C4068: unknown pragma

    #silence the warning.
    if(USE_ASAN)
        string( REPLACE "/INCREMENTAL" "/INCREMENTAL:NO" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
    endif()
else()

    #keep asserts for reldeb
    #NOTE: this will taint FetchContent compile options... 
    #but I don't mind library asserts being enabled.
    string( REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    #this is a big chunk taken from openrtc2, I hope they don't mind, but I like warnings.
    include(CheckCXXCompilerFlag)

    # Check if a flag exists and add it to the list of compiler options
    function (ADD_CHECK_CXX_COMPILER_FLAG _CXXFLAGS _CACHE_VAR _FLAG)
        CHECK_CXX_COMPILER_FLAG("${_FLAG}" "${_CACHE_VAR}")
        if (${_CACHE_VAR})
            set(${_CXXFLAGS} ${${_CXXFLAGS}} ${_FLAG} PARENT_SCOPE)
        else ()
            message(STATUS "Unsupported CXXFLAG: ${_FLAG}")
        endif ()
    endfunction ()

    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_SUGGEST_OVERRIDE -Wsuggest-override)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_DUPLICATED_COND -Wduplicated-cond)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_NON_VIRTUAL_DTOR -Wnon-virtual-dtor)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_MISSING_VARIABLE_DECLARATIONS -Wmissing-variable-declarations)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_DUPLICATED_BRANCHES -Wduplicated-branches)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_RESTRICT -Wrestrict)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_MISSING_FIELD_INITIALIZERS -Wmissing-field-initializers)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_UNREACHABLE_CODE_BREAK -Wunreachable-code-break)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_LOGICAL_OP -Wlogical-op)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_RANGE_LOOP_ANALYSIS -Wrange-loop-analysis)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_TAUTOLOGICAL_ZERO_COMPARE -Wtautological-unsigned-zero-compare)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WNO_CLOBBERED -Wno-clobbered)


    # Do not enable for MinGW, as its headers contain redundant declarations of builtin functions (from orct2)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_REDUNDANT_DECLS -Wredundant-decls)

    # Currently used MinGW w/GCC 7.2 doesn't provide sane error messages for this (from orct2)
    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_NULL_DEREFERENCE -Wnull-dereference)

    #these 2 warnings can be tested once in a while, but they can be annoying since final is not neccessary everywhere.
    #ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_SUGGEST_FINAL_TYPES -Wsuggest-final-types)
    #ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_SUGGEST_FINAL_METHODS -Wsuggest-final-methods)

    ADD_CHECK_CXX_COMPILER_FLAG(MY_WARNING_FLAGS CXX_WARN_IGNORED_QUALIFIERS -Wignored-qualifiers)


    set(MY_WARNING_FLAGS ${MY_WARNING_FLAGS} -fstrict-aliasing -Wundef -Wmissing-declarations -Winit-self -Wall -Wextra -Wshadow -Werror=vla)
    set(MY_WARNING_FLAGS ${MY_WARNING_FLAGS} -Wno-unknown-pragmas -Wno-missing-braces)

	#probably should be a property
    if(PORTABLE AND GNUC)
        set(CMAKE_INSTALL_RPATH "$ORIGIN")
    endif()

endif()

#
# include files
#

include(code/CMakeLists.txt)

#set flags.
target_compile_options(${PROJECT_NAME} PUBLIC ${MY_WARNING_FLAGS})

#prevent debugging problems on reldeb.
if(NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_options(${PROJECT_NAME} PUBLIC  -fno-omit-frame-pointer)
endif()

#these flags affect both mingw and msvc
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC WIN32_SILENCE_FLAGS _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
endif()

if(MSVC OR MY_CLANGCL)
    #CRASHRTP_HANDLER doesn't work on MINGW because it doesn't support codeview.
    #set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DUSE_WIN32_CRASHRTP_HANDLER")
    #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DUSE_WIN32_CRASHRTP_HANDLER")

    #this is only relevant to msvc (TODO: does this affect clang-cl? I think it should)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _SCL_SECURE_NO_WARNINGS)
endif()

#
# set properties
#

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    option(USE_CCACHE "some stupid snake oil" ON)
    if(USE_CCACHE)
        message(STATUS "ccache found")
        #set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
        set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_COMPILER_LAUNCHER ccache)
    endif()
endif()

if(NOT MSVC)
	#no-pie purely because gcc defaults to pie, 
	#which means that the executable is a "shared library"
	#which means that I can't just open the program from the file explorer.
	#TODO: remove this for security.
	#also this affects addr2line.
	include(CheckPIESupported)
	check_pie_supported()
	set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE FALSE)
endif()

option(USE_LTO "Use link time optimizations" OFF)
if(USE_LTO)
    #for -flto, and also something in msvc.
    #the only problem is that you might want to customize the lto target, like -flto=thin on clang.
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
endif()

if(MSVC OR MY_CLANGCL)
    #so you have 2 choices, override CMAKE_CXX_FLAGS_..., or set these flag.
    #note that it is impossible to create a static debug runtime (I would never need it)
    option(FORCE_STATIC_VCRT "for msvc" OFF)
    option(FORCE_NON_DEBUG_VCRT "for msvc" OFF)
    if(FORCE_STATIC_VCRT)
        set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    elseif(FORCE_NON_DEBUG_VCRT)
        set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    endif()
endif()


#
# sanitizer flags
#

#note that you cannot mix sanitizers with eachother.
#asan + ubsan + lsan should be a supported combination

#add_compile_options(-fprofile-instr-generate -fcoverage-mapping)

#Control Flow Integrity is interesting, I wonder if cfi is closer to UBSAN or ASAN?
#-fsanitize=cfi




option(USE_ASAN "Use address sanitizer" OFF)
if(USE_ASAN)
    
    if(MSVC)
        #1: use "set ASAN_OPTIONS=windows_hook_rtl_allocators=true"
        #and "set ASAN_SAVE_DUMPS=MyFileName.dmp"
        #2: note for vscode, if you don't have the windows system symbols
        #the stack will not show where the error occurred for some reason,
        #a fix is to use /fno-sanitize-address-vcasan-lib or get symbols
        #3: set ASAN_VCASAN_DEBUGGING=1 globally for running without a debugger
		target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
	else()
        #also use ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1
        #note windows uses semicolons (but I think other than utility options, only windows_hook_rtl_allocators=true has use)
    #or define it through __asan_default_options
		target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
		target_link_libraries(${PROJECT_NAME} -fsanitize=address)
	endif()
endif()


option(USE_UBSAN "Use undefined behavior sanitizer" OFF)
if(USE_UBSAN)
	if(MSVC)
        #technically you could link ubsan, but you gotta use explicit files...
        message(WARNING "msvc does not support /fsanitize=undefined, but static CRT clang-cl using gnuc works")
	else()
        if(MY_CLANGCL AND NOT FORCE_STATIC_VCRT)
            message(WARNING "atm (can be fixed) clang-cl ubsan requires a static CRT.")
        endif()
		target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=undefined)
        #make it stop
		target_compile_options(${PROJECT_NAME} PUBLIC -fno-sanitize-recover=all)
		#this wont work on gcc for some reason.
		if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
			target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=unsigned-integer-overflow -fsanitize=nullability -fsanitize=implicit-conversion -fsanitize=local-bounds)
		endif()
		#On windows you would want your code to "not respond" so you can hook a JIT debugger.
		#but you lose the ubsan message if you use this...
		#target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize-trap=all)
		
		target_link_libraries(${PROJECT_NAME} -fsanitize=undefined)
	endif()
endif ()




# I am unsure if TSAN would work, the FAQ say that C++11 threads are buggy
# or require the libc/libstdc++ standard library to be built with TSAN
option(USE_TSAN "Use thread sanitizer" OFF)
if(USE_TSAN)
    target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=thread)
    target_link_libraries(${PROJECT_NAME} -fsanitize=thread)
endif()

#I think MSAN requires all libraries to also be compiled with msan, unlike other sanitizers.
#I don't think I can get it to work with SDL2
option(USE_MSAN "Use memory sanitizer" OFF)
if(USE_MSAN)
    target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=memory)
    target_link_libraries(${PROJECT_NAME} -fsanitize=memory)
endif()



#
# link libraries
#

#FetchContent is for laziness, it load a git repo and compiles it (note .git folders are big)
#I prevent FetchContent from inserting garbage into the install,
#but it will not prevent your compile_commands.json from being filled with garbage.

option(USE_FETCH "always compile" OFF)

if(USE_FETCH)
    
    include(FetchContent)

    #this is because FetchContent_Declare paired with GIT_REPOSITORY will ignore SOURCE_DIR, yet it will still copy the files into it.
    #I don't think it's flawed (it probably checks the .git folder for changes), but it isn't what I want.
    option(NO_DOWNLOAD "USE_FETCH will download libraries to /external, but new builds will ignore /external" OFF)
	if(NOT NO_DOWNLOAD)
		find_package(Git REQUIRED)
	endif()
endif()


#
# SDL2
#
option(USE_OLD_SDL2 "for SDL2 older than 2.0.6" OFF)

find_package(SDL2 CONFIG REQUIRED)

if(USE_OLD_SDL2)
    #the old way of linking.
    #note that there is no static linking because
	#this is PURELY for package managers, which usually HATE static libraries.
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
else()
    if(NOT STATIC_BUILD)
        target_link_libraries(${PROJECT_NAME} SDL2::SDL2)
    else()
        target_link_libraries(${PROJECT_NAME} SDL2::SDL2-static)
    endif()
    target_link_libraries(${PROJECT_NAME} SDL2::SDL2main)
endif()

#
# RAPIDJSON
#

find_package(RapidJSON CONFIG REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${RapidJSON_INCLUDE_DIRS})

#I use a really new version of rapidjson, so I can't copy the include files
#because then the system packages would override the includes...
#if(RapidJSON-NOTFOUND)
#    message(WARNING "rapidjson was not found, assuming you just copied the headers.")
#endif()

#
# install
#

install(TARGETS ${PROJECT_NAME})

#3.16 black magic install all shared libraries.
#excluding some system paths, on linux I exlucde paths with /lib 
#so that only FetchContent binaries are copied (otherwise you get .so hell)
#warning: this probably will not update stale DLL's, but I should double check.
if(PORTABLE OR WIN32)

    #sets PATH because it includes address sanitizer DLL's... not set by CMAKE_SYSTEM_LIBRARY_PATH
    set(MY_DEPENDENCY_PATHS ${CMAKE_SYSTEM_LIBRARY_PATH})

	#I like to use CMAKE_PREFIX_PATH for prefixes of installed libraries, but I don't want to bundle the garbage
	foreach(_file ${CMAKE_PREFIX_PATH})
		set(MY_DEPENDENCY_PATHS ${MY_DEPENDENCY_PATHS} "${_file}/bin")
	endforeach()
    
    # Transfer the value of ${MY_DEPENDENCY_PATHS} into the install script
	install(CODE "set(MY_DESTINATION_PATH \"${CMAKE_INSTALL_PREFIX}/bin\")")
    install(CODE "set(MY_DEPENDENCY_PATHS \"${MY_DEPENDENCY_PATHS}\")")
    install(CODE "set(MY_EXECUTABLE_NAME \"$<TARGET_FILE:${PROJECT_NAME}>\")")
    install(CODE [[
		file(GET_RUNTIME_DEPENDENCIES
			EXECUTABLES ${MY_EXECUTABLE_NAME}
			RESOLVED_DEPENDENCIES_VAR _r_deps
			UNRESOLVED_DEPENDENCIES_VAR _u_deps
			DIRECTORIES ${MY_DEPENDENCY_PATHS} $ENV{PATH}
			PRE_EXCLUDE_REGEXES "api-ms-*" "ext-ms-*"
			POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" "/lib/.*\\.so"
		)
		foreach(_file ${_r_deps})
			if(WIN32)
				#this fixes the filename being lowercase.
				get_filename_component(_file "${_file}" ABSOLUTE)
                #NOTE: I could try to replace the extension with pdb and install that too.
			endif()
			file(INSTALL
				DESTINATION ${MY_DESTINATION_PATH}
				TYPE SHARED_LIBRARY
				FOLLOW_SYMLINK_CHAIN
				FILES "${_file}"
			)
		endforeach()
        foreach(_file ${_u_deps})
            message(WARNING "Unresolved dependency: ${_file}")
		endforeach()
    ]])

    if(MSVC AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        #copy the pdb files so the debugger can see them.
        install(CODE "set(MY_DEBUG_SYMBOL $<TARGET_PDB_FILE:${PROJECT_NAME}>)")
        install(CODE [[file(INSTALL FILES ${MY_DEBUG_SYMBOL}  DESTINATION ${MY_DESTINATION_PATH})]])
        #install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" OPTIONAL)
    endif()

endif()
