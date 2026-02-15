# Archiver

Archiver is a CLI tool that helps programs embed external data into their applications, such as images or shader code.
While most tools use things like `xxd` to export raw data into C-compatible byte data, Archiver does this by directly embedding the binary data into the application itself,
greatly reducing the data processing and bloat that is added on top when using `xxd`, which requires the source file and compiler to read through + verify ~5x the amount of memory.

Archiver does this by attaching files to the data section in the assembly, with little overhead. 

While Archiver does not physically do any of the building & compiling, it does manage the object file generation and data retrieval.

## Requirements

- `ld` (GNU Linker) - required to turn files into object files. Other linkers are not yet supported. You can install `ld` by installing the MinGW compiler suite.

- `g++`/`gcc`/`clang`/some other compiler that can link object files. Not required to run but is required to embed files.

## Usage

Obtain the `archive` executable (which may be downloaded from the Releases tab if your on Windows) and specify all the resource files you may want to embed, 8

and also specify the output file that should contain all the binaries with the `-o` option. This is usually specified as a `.o` (object) file, which will contain all the data from the specified items.

The `-o` option is required for output.

```shell
archive -o arch.o resource1.txt resource2.png resource3.json
```

If all of your resources are stored in the same directory, you may specify the search directory with the `-r` option, followed by the resource path. `archive` will only search files in that directory.

This step is optional but can help in reducing assembly overhead and will make accessing the data easier.
Doing this also produces the same behavior as executing in the resource search directory, only that the output files are written in the execution directory instead. 

```shell
archive -o arch.o -r path/to/res [your files here...]
```

By default, `archive` will output the linking header file required for compiling (see [Compiling](#compiling)) to stdout. 
If you want to write the output to an actual header file instead, use the `-f` option to specify the output header file. 
```shell
archive -o arch.o -r path/to/res -f Resources.h [your files here...]
```
The above command will take resources from `path/to/res/...`, output the binaries to `arch.o`, and write the required header file in `Resources.h`. 

If you need help, `-h` or `--help` will show the above information. 

---

Once `archive` is run, two files are output that must be provided to the compiler, namely `res.o` and `res.h`:
- the object file, which contains the raw data, and 
- the header file, used to tell the compiler how to link the data into the program

If you want to verify that the object file does indeed contain all the information you need, run
```shell
objdump -x res.o
```
and at the bottom you should see your resources written in `_binary_[YOUR_RESOURCE_NAME]_start` and `_binary_[YOUR_RESOURCE_NAME]_end`.
(`objdump` is available for all OSes)
This means that the files have been exported successfully.

The header file should look something like this (if you exported a singular file `image.png`):
```c++
namespace _ar {
    extern char image_png_s[] asm("_binary_image_png_start"), image_png_e[] asm("_binary_image_png_end"); /* more data here... */
    static constexpr const char* data_ptrs[] = { image_png_s, image_png_e, }, *names[] = { "image.png", };
}
#define FETCH_ARCHIVE() Archive::FromPtrs(_ar::data_ptrs, _ar::names, 1)
```
The first line contains all the addresses to the files, linked to the assembly symbol `asm(...)`, which the compiler is able to find in `res.o`. 
Confirm all the resources are all contained in this header file, or else `Archive` will not be able to find it.

Finally, in your code, include `res.h` in your source file and to access your files, use the `Archive` object defined in the QUtils library.
```c++
#include "res.h" // crucial for finding ur resources
#include "Utils/IO/Archive.h" // in the QUtils library

int main(int argc, char* argv[]) {
    // you may need to do namespace scope resolution for this;
    // use the FETCH_ARCHIVE() macro to automatically create an Archive object will all ur data
    Quasi::Archive archive = Quasi::FETCH_ARCHIVE();
    
    // now read the raw bytes by using .Get(filename), no obfuscation/name mangling needed 
    // this returns an Option<Bytes>, returning null if the file was not found. 
    Quasi::Bytes imagePng = archive.Get("image.png").Assert("image.png not found!");
    
    // use `imagePng`...
    // ex: Quasi::Image image = Quasi::Image::FromBytes(imagePng);
     
    return 0;
}
```
Now you can use `image.png` in your code, without worrying about resource management! 

## Compiling
If you're compiling your program yourself, you may simply run the following commands:
```shell
archive -o res.o -r res -f res.h image.png
g++ -o main main.cpp res.o   
```
Any compiler that supports compiling `.o` object files (`g++`, `gcc`, `clang`...) will successfully compile your data into the executable `main`.

If your using CMake however, you may want to automate this process.
To do this in CMake, we have to use a pre-build step to export the files before compiling. 
This is usually done using `add_custom_command`:
```cmake
set(RESOURCE_FILES
    image.png
    shader.glsl
    userdata.json
    # put more files here ...
)

add_custom_command(
    OUTPUT res.o res.h # tells cmake these two files are generated
    COMMAND archive -o res.o -r res -i res.h ${RESOURCE_FILES}
    DEPENDS
        res/image.png
        # add your dependencies (aka resources) here
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_executable(${PROJECT_NAME}
    main.cpp
    res.h # this is optional; ur header file
    res.o # this is ur object file; mandatory for linking
)
```

## Notes
- This program does not do any input sanitization or any checks on the input files. 
- If you have invalid files or files with similar names that may collide when mangling,
  you may want to rename your resources to something else that won't collide.
  `ld` will turn non-alphanumeric characters into underscores, so there's not much you can do about it to combat it.
  Basically this'll result in `file_1.txt` and `file-1.txt` resulting in the same identifier.
- Some compilers may not support `asm`, so a quick hack you can do to fix this is to `#define asm __asm__` or whatever it is before including the `res.h` header file, and the undefine it after.
- Should you use this for your project? Probably not. But guess I can't stop you. It's free to use but I don't know how to use the free license thingy. Also don't expect any bugfixes or whatever; but I mean if you want to contribute you're welcome to do so.