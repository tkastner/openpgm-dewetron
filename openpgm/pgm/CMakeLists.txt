# CMake build script for OpenPGM on Windows

cmake_minimum_required (VERSION 2.8)
project (OpenPGM)

#-----------------------------------------------------------------------------
# adoptions for DEWETRON build environment
# eg: all binaries in one directory
if (DEWETRON_BUILD)
  include(CMakeLists.dewetron)
endif()

#-----------------------------------------------------------------------------
# force off-tree build

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
message(FATAL_ERROR "CMake generation is not allowed within the source directory! 
Remove the CMakeCache.txt file and try again from another folder, e.g.: 

   del CMakeCache.txt 
   mkdir cmake-make 
   cd cmake-make
   cmake ..
")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

#-----------------------------------------------------------------------------
# dependencies

include (${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/TestOpenPGMVersion.cmake)

#-----------------------------------------------------------------------------
# default to Release build

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/lib)

#-----------------------------------------------------------------------------
# platform specifics

add_definitions(
	-DWIN32
	-D_CRT_SECURE_NO_WARNINGS
	-DHAVE_FTIME
	-DHAVE_ISO_VARARGS
	-DHAVE_RDTSC
	-DHAVE_WSACMSGHDR
	-DHAVE_DSO_VISIBILITY
	-DUSE_BIND_INADDR_ANY
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(
		-DPGM_DEBUG
	)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

# Enables the use of Intel Advanced Vector Extensions 2 instructions.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")

# Parallel make.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

# Optimization flags.
# http://msdn.microsoft.com/en-us/magazine/cc301698.aspx
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /LTCG")

#-----------------------------------------------------------------------------
# source files

set(c99-sources
	cpu.c
        thread.c
        mem.c
        string.c
        list.c
        slist
        queue.c
        hashtable.c
        messages.c
        error.c
        math.c
        packet_parse.c
        packet_test.c
        sockaddr.c
        time.c
        if.c
	inet_lnaof.c
        getifaddrs.c
	get_nprocs.c
        getnetbyname.c
        getnodeaddr.c
        getprotobyname.c
        indextoaddr.c
        indextoname.c
        nametoindex.c
        inet_network.c
        md5.c
        rand.c
        gsi.c
        tsi.c
        txw.c
        rxw.c
        skbuff.c
        socket.c
        source.c
        receiver.c
        recv.c
        engine.c
        timer.c
        net.c
        rate_control.c
        checksum.c
        reed_solomon.c
        wsastrerror.c
        histogram.c
)

include_directories(
	include
)
set(headers
	include/pgm/atomic.h
	include/pgm/engine.h
	include/pgm/error.h
	include/pgm/gsi.h
	include/pgm/if.h
	include/pgm/in.h
	include/pgm/list.h
	include/pgm/macros.h
	include/pgm/mem.h
	include/pgm/messages.h
	include/pgm/msgv.h
	include/pgm/packet.h
	include/pgm/pgm.h
	include/pgm/skbuff.h
	include/pgm/socket.h
	include/pgm/time.h
	include/pgm/tsi.h
	include/pgm/types.h
	include/pgm/version.h
	include/pgm/winint.h
	include/pgm/wininttypes.h
	include/pgm/zinttypes.h
)

source_group("Public Header Files" FILES ${headers})

set(private_headers
	include/impl/checksum.h
	include/impl/engine.h
	include/impl/errno.h
	include/impl/fixed.h
	include/impl/framework.h
	include/impl/galois.h
	include/impl/getifaddrs.h
	include/impl/getnetbyname.h
	include/impl/getnodeaddr.h
	include/impl/getprotobyname.h
	include/impl/get_nprocs.h
	include/impl/hashtable.h
	include/impl/histogram.h
	include/impl/i18n.h
	include/impl/indextoaddr.h
	include/impl/indextoname.h
	include/impl/inet_lnaof.h
	include/impl/inet_network.h
	include/impl/ip.h
	include/impl/list.h
	include/impl/math.h
	include/impl/md5.h
	include/impl/mem.h
	include/impl/messages.h
	include/impl/nametoindex.h
	include/impl/net.h
	include/impl/net_os.h
	include/impl/notify.h
	include/impl/packet_parse.h
	include/impl/packet_test.h
	include/impl/pgmMIB.h
	include/impl/pgmMIB_columns.h
	include/impl/pgmMIB_enums.h
	include/impl/processor.h
	include/impl/queue.h
	include/impl/rand.h
	include/impl/rate_control.h
	include/impl/receiver.h
	include/impl/reed_solomon.h
	include/impl/rwspinlock.h
	include/impl/rxw.h
	include/impl/security.h
	include/impl/slist.h
	include/impl/sn.h
	include/impl/sockaddr.h
	include/impl/socket.h
	include/impl/source.h
	include/impl/sqn_list.h
	include/impl/string.h
	include/impl/thread.h
	include/impl/ticket.h
	include/impl/time.h
	include/impl/timer.h
	include/impl/tsi.h
	include/impl/txw.h
	include/impl/wsastrerror.h
	include/impl/net_os.h
)
source_group("Private Header Files" FILES ${private_headers})

add_definitions(
	-DUSE_TICKET_SPINLOCK
	-DUSE_DUMB_RWSPINLOCK
	-DUSE_GALOIS_MUL_LUT
	-DGETTEXT_PACKAGE='"pgm"'
)

#-----------------------------------------------------------------------------
# source generators

# version stamping
add_executable(mkversion ${CMAKE_CURRENT_SOURCE_DIR}/mkversion.c)
add_custom_command(
	OUTPUT version.c
	COMMAND mkversion
	ARGS > version.c
	DEPENDS mkversion
)

set(sources
	${c99-sources}
	galois_tables.c
        ${CMAKE_CURRENT_BINARY_DIR}/version.c
)

source_group("Source Files" FILES ${sources})

#-----------------------------------------------------------------------------
# output

add_library(libpgm STATIC ${sources} ${headers} ${private_headers})
set_target_properties(libpgm PROPERTIES
	RELEASE_POSTFIX "${_pgm_COMPILER}-mt-${OPENPGM_VERSION_MAJOR}_${OPENPGM_VERSION_MINOR}_${OPENPGM_VERSION_MICRO}"
	DEBUG_POSTFIX "${_pgm_COMPILER}-mt-gd-${OPENPGM_VERSION_MAJOR}_${OPENPGM_VERSION_MINOR}_${OPENPGM_VERSION_MICRO}")

add_executable(purinsend examples/purinsend.c examples/getopt.c examples/getopt_long.c)
target_link_libraries(purinsend libpgm)
set_target_properties(purinsend PROPERTIES FOLDER "Examples")

add_executable(purinrecv examples/purinrecv.c examples/getopt.c examples/getopt_long.c)
target_link_libraries(purinrecv libpgm)
set_target_properties(purinrecv PROPERTIES FOLDER "Examples")

add_executable(daytime examples/daytime.c examples/getopt.c examples/getopt_long.c)
target_link_libraries(daytime libpgm)
set_target_properties(daytime PROPERTIES FOLDER "Examples")

add_executable(shortcakerecv examples/shortcakerecv.c examples/async.c examples/getopt.c examples/getopt_long.c)
target_link_libraries(shortcakerecv libpgm)
set_target_properties(shortcakerecv PROPERTIES FOLDER "Examples")

#-----------------------------------------------------------------------------
# installer

set(docs
	COPYING
	LICENSE
	README
)
file(GLOB mibs "${CMAKE_CURRENT_SOURCE_DIR}/mibs/*.txt")
set(examples
	examples/async.c
	examples/async.h
	examples/daytime.c
	examples/getopt.c
	examples/getopt.h
	examples/purinrecv.c
	examples/purinsend.c
	examples/shortcakerecv.c
)


# CPack now requires either .txt or .rtf license file.
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt
	COMMAND ${CMAKE_COMMAND}
	ARGS    -E
		copy
		${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
		${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
)
set (CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")

install (TARGETS libpgm DESTINATION lib)
install (TARGETS purinsend purinrecv daytime shortcakerecv DESTINATION bin)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	install (
		FILES ${CMAKE_BINARY_DIR}/lib/libpgm${_pgm_COMPILER}-mt-gd-${OPENPGM_VERSION_MAJOR}_${OPENPGM_VERSION_MINOR}_${OPENPGM_VERSION_MICRO}.pdb
		DESTINATION lib
	)
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")
install (FILES ${headers} DESTINATION include/pgm)
foreach (doc ${docs})
	configure_file (${CMAKE_CURRENT_SOURCE_DIR}/${doc} ${CMAKE_CURRENT_BINARY_DIR}/${doc}.txt)
	install (FILES ${CMAKE_BINARY_DIR}/${doc}.txt DESTINATION doc)
endforeach (doc ${docs})
install (FILES ${mibs} DESTINATION mibs)
install (FILES ${examples} DESTINATION examples)

# Only need to ship CRT if distributing executable binaries.
# include (InstallRequiredSystemLibraries)
set (CPACK_INSTALL_CMAKE_PROJECTS
		"${CMAKE_CURRENT_SOURCE_DIR}/build/v140;OpenPGM;ALL;/"
		"${CMAKE_CURRENT_SOURCE_DIR}/build/v120;OpenPGM;ALL;/"
)
set (CPACK_PACKAGE_VENDOR "Miru")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")
set (CPACK_PACKAGE_VERSION_MAJOR ${OPENPGM_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${OPENPGM_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${OPENPGM_VERSION_MICRO})
set (CPACK_WIX_UPGRADE_GUID "832A8F90-C7A6-4F1E-8562-2068A7C9B29C")
include (CPack)

# end of file
