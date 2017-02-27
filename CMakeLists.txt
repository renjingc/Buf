project(Buf)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
add_definitions(-std=c++11)
include_directories(/home/ren/work/finish15/Buf/Buf
                                /home/ren/work/finish15/Buf/Buf/exceptions)
add_executable(${PROJECT_NAME} ${SRC_LIST}
                    /home/ren/work/finish15/Buf/Buf/exceptions/bad_buffer_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/badgerdb_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/buffer_exceeded_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/file_exists_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/file_not_found_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/file_open_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/hash_already_present_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/hash_not_found_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/hash_table_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/insufficient_space_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/invalid_page_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/invalid_record_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/invalid_slot_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/page_not_pinned_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/page_pinned_exception.cpp
                    /home/ren/work/finish15/Buf/Buf/exceptions/slot_in_use_exception.cpp)

