cmake_minimum_required(VERSION 2.8.12)

project (Linkage)

file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
file(GLOB RESOURCES "*.rc")

# remove anoying cmake stuff
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

if(MSVC)
# use static mfc
set(CMAKE_MFC_FLAG 1)

# switch to "Multi-threaded" CRT instead of default "Multi-threaded DLL" due to mfc
foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
   if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
   endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)
endif()

add_executable(Linkage ${HEADERS} ${SOURCES} ${RESOURCES})
target_link_libraries (Linkage vfw32 Wmvcore Version)
set_target_properties(Linkage PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")

# installer stuff
install(TARGETS Linkage DESTINATION .)
install(FILES Linkage.pdf history.txt DESTINATION .)

set(CPACK_PACKAGE_NAME "Linkage")
set(CPACK_PACKAGE_VENDOR "linkagesimulator.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Linkage Designer and Simulator")
set(CPACK_PACKAGE_VERSION "3.3.8")
set(CPACK_PACKAGE_VERSION_MAJOR "3")
set(CPACK_PACKAGE_VERSION_MINOR "3")
set(CPACK_PACKAGE_VERSION_PATCH "8")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Linkage")

set(CPACK_WIX_UPGRADE_GUID "363E6270-4C3C-4022-80C5-32499430E4BE")

include(CPack)