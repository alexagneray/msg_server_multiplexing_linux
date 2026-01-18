cmake_minimum_required(VERSION 3.0.0)
project(MsgServerMultiplexingLinux VERSION 0.1.0)

enable_testing()

add_executable(MsgServer main.c)

target_compile_options(MsgServer PRIVATE -Wall -Wextra -Werror -Wpedantic)


