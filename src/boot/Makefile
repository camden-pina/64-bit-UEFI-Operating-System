CXX=x86_64-w64-mingw32-g++
CXX_FLAGS=-std=c++11 -nostdlib -nodefaultlibs -fno-builtin -ffreestanding -mno-red-zone -fshort-wchar -fno-omit-frame-pointer -Wall -Wextra -fno-exceptions -fno-rtti -Wl,-dll -shared -I ../../ext_lib -I ../lib
LD_FLAGS=-nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main

OBJ=main.o logger.o video.o allocator.o memory_map.o hardware.o bootconfig.o fs.o loader.o ../lib/elf/file.o

TARGET=bootx64.efi

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

all: $(OBJ)
	x86_64-w64-mingw32-g++ $(CXX_FLAGS) $(LD_FLAGS) $(OBJ) -o $(TARGET)

clean:
	rm $(OBJ)
	rm *.efi
