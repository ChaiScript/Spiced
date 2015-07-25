cmake_minimum_required(VERSION 2.8)

project(Spiced)

find_library(SFML_GRAPHICS sfml-graphics)
find_library(SFML_WINDOW sfml-window)
find_library(SFML_SYSTEM sfml-system)


# MINGW does not yet support C++11's concurrency features
if(MINGW)
  option(MULTITHREAD_SUPPORT_ENABLED "Multithreaded Support Enabled" FALSE)
else()
  option(MULTITHREAD_SUPPORT_ENABLED "Multithreaded Support Enabled" TRUE)
endif()


if(CMAKE_COMPILER_IS_GNUCC)
  option(ENABLE_COVERAGE "Enable Coverage Reporting in GCC" FALSE)

  if(ENABLE_COVERAGE)
    add_definitions(--coverage -O0)
    set(LINKER_FLAGS "${LINKER_FLAGS} --coverage")
  endif()
endif()

if(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  option(ENABLE_THREAD_SANITIZER "Enable thread sanitizer testing in gcc/clang" FALSE)
  if(ENABLE_THREAD_SANITIZER)
    add_definitions(-fsanitize=thread -g)
    set(LINKER_FLAGS "${LINKER_FLAGS} -fsanitize=thread")
  endif()

  option(ENABLE_ADDRESS_SANITIZER "Enable address sanitizer testing in gcc/clang" FALSE)
  if(ENABLE_ADDRESS_SANITIZER)
    add_definitions(-fsanitize=address -g)
    set(LINKER_FLAGS "${LINKER_FLAGS} -fsanitize=address")
  endif()

  option(ENABLE_MEMORY_SANITIZER "Enable memory sanitizer testing in gcc/clang" FALSE)
  if(ENABLE_MEMORY_SANITIZER)
    add_definitions(-fsanitize=memory -g)
    set(LINKER_FLAGS "${LINKER_FLAGS} -fsanitize=memory")
  endif()

  option(ENABLE_UNDEFINED_SANITIZER "Enable undefined behavior sanitizer testing in gcc/clang" FALSE)
  if(ENABLE_UNDEFINED_SANITIZER)
    add_definitions(-fsanitize=undefined -g)
    set(LINKER_FLAGS "${LINKER_FLAGS} -fsanitize=undefined")
  endif()

  option(ENABLE_LTO "Enable Link Time Optimization" FALSE)

  if (ENABLE_LTO)
    add_definitions(-flto)
    set(LINKER_FLAGS "${LINKER_FLAGS} -flto")
  endif()

  option(PROFILE_GENERATE "Generate profile data" FALSE)
  if (PROFILE_GENERATE)
    add_definitions(-fprofile-generate)
    set(LINKER_FLAGS "${LINKER_FLAGS} -fprofile-generate")
  endif()

  option(PROFILE_USE "Use profile data" FALSE)
  if (PROFILE_USE)
    add_definitions(-fprofile-use)
    set(LINKER_FLAGS "${LINKER_FLAGS} -fprofile-use")
  endif()


endif()

#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/license.txt")
#set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/readme.md")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/description.txt")

set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 1)

set(CPACK_PACKAGE_EXECUTABLES "spiced;Spiced Game Interperter")
set(CPACK_PACKAGE_VENDOR "ChaiScript.com")
set(CPACK_PACKAGE_CONTACT "contact@chaiscript.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A game engine")

set(CPACK_DEBIAN_PACKAGE_SECTION "game")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

set(CPACK_RPM_PACKAGE_LICENSE "BSD")
set(CPACK_RPM_PACKAGE_GROUP "Gaming")

set(CHAI_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})

#configure_file(Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile)

include(CPack)


if(CMAKE_COMPILER_IS_GNUCC)
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

  if(GCC_VERSION VERSION_LESS 4.8)
    set(CPP11_FLAG "-std=c++0x")
  else()
    set(CPP11_FLAG "-std=c++11")
  endif()
else()
  set(CPP11_FLAG "-std=c++11")
endif()

if(MSVC)
  add_definitions(/W4 /w44640)
  add_definitions(/bigobj)
#  add_definitions(-DSFML_STATIC)
  # Note on MSVC compiler flags.
  # The code base selective disables warnings as necessary when the compiler is complaining too much
  # about something that is perfectly valid, or there is simply no technical way around it
  # This particular warning, C4503 is in regards to the decorated names that MSVC generates internally.
  # The error did not come up until the move to C++11, but the compiler doesn't give enough information
  # to determine where the error is coming from, and the internet provides no real information for
  # how to workaround or fix the error. So I'm disabling it globally.
  add_definitions(/wd4503)
else()
  add_definitions(-Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic ${CPP11_FLAG})

  if(APPLE)
    add_definitions(-Wno-sign-compare)
  endif()
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  option(USE_LIBCXX "Use clang's libcxx" TRUE)

  if(USE_LIBCXX)
    add_definitions(-stdlib=libc++)
    set(LINKER_FLAGS "${LINKER_FLAGS} ${CPP11_FLAG} -stdlib=libc++")
  else()
    set(LINKER_FLAGS "${LINKER_FLAGS} ${CPP11_FLAG}")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC)
  set(LINKER_FLAGS "${LINKER_FLAGS} ${CPP11_FLAG}")
endif()

# limitations in MinGW require us to make an optimized build
# for the sake of object sizes or something
if(MINGW OR CYGWIN)
  add_definitions(-O3)
endif()

include_directories(include)
include_directories(c:/programming/sfml-2.3/include)

if(NOT MULTITHREAD_SUPPORT_ENABLED)
  add_definitions(-DCHAISCRIPT_NO_THREADS -DCHAISCRIPT_NO_THREADS_WARNING)
endif()

if(CMAKE_HOST_UNIX)
  if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Haiku")
    if (NOT MINGW AND NOT CYGWIN)
      list(APPEND LIBS "dl")
    endif()
  endif()

  if(MULTITHREAD_SUPPORT_ENABLED)
    if(CMAKE_COMPILER_IS_GNUCC)
      execute_process(COMMAND ${CMAKE_C_COMPILER} --version OUTPUT_VARIABLE GCC_FULL_VERSION)
      if(GCC_FULL_VERSION MATCHES "4.8.1.*ubuntu")
        set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,--no-as-needed -pthread")
      else()
        set(LINKER_FLAGS "${LINKER_FLAGS} -pthread")
      endif()
    else()
      set(LINKER_FLAGS "${LINKER_FLAGS} -pthread")
    endif()

    add_definitions(-pthread)
  endif()
endif()


set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

add_executable(spiced src/main.cpp src/game.cpp src/game_event.cpp src/map.cpp src/chaiscript_stdlib.cpp src/chaiscript_bindings.cpp src/chaiscript_creator.cpp)
target_link_libraries(spiced ${SFML_GRAPHICS} ${SFML_WINDOW} ${SFML_SYSTEM} ${LIBS})
file(COPY sample_game DESTINATION ${CMAKE_BINARY_DIR})

install(TARGETS spiced RUNTIME DESTINATION bin)


