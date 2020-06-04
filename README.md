# new-words-discoverer
## Introduction
A new words discoverer without any existing knowledge base. Only Chinese is supported.

## Prerequisites
- C++14 or later
- [CMake](https://cmake.org/) 3.0 or later
- [Boost](https://www.boost.org/)

## Building
```shell
mkdir build
cd build
cmake [-DBOOST_ROOT=/path/to/your/boost/root/dir] [-DBOOST_LIBRARYDIR=/path/to/your/boost/lib] ..
cmake --build . --target install --config Release
cd ..
```

## Example
You can simply run the example by following steps:
- Build the application
- Run the example: `cd install && ./new_words_discoverer -f ../corpus/红楼梦.txt`
- Check the output in `红楼梦_out.txt`
## Reference
http://www.matrix67.com/blog/archives/5044

