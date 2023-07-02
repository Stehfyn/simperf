#!/usr/bin/env bash
find include/ -type f \( -iname *.hpp -o -iname *.h -o -iname *.cpp \) -exec clang-format -i {} \;