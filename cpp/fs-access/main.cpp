#include <root.hpp>
#include <core/fs.hpp>

using namespace aqua::core;

static const std::string DRIVE = "unq";
static const std::string PATH  = "test-file";

static void write_file(void) {
	// open a file for writing at 'unq:test-file'
	// this file is created in the unique directory of your app, which is a directory normally exclusively accessible by this program
	// we want to create it if it doesn't already exist, so pass the 'fs::FLAGS_CREATE' flag too
	// similarly, we don't want to overwrite anything already in the file, so pass the 'fs::FLAGS_APPEND' flag

	fs::Descr file(DRIVE, PATH, fs::FLAGS_WRITE | fs::FLAGS_APPEND | fs::FLAGS_CREATE);

	// we may now write whatever we want to this file

	file.write("Hello world!\n");
}

static void read_file(void) {
	// open that file we wrote to

	fs::Descr file(DRIVE, PATH, fs::FLAGS_READ);

	// get the size in bytes of our file

	size_t size = file.size();
	std::cout << "File is " << size << " bytes long" << std::endl;

	// map the file to memory
	// you can also use 'file.read(...)' if you intend to read the entire file at once, but memory mapping is much more efficient for reading chunks of large files

	uint8_t* mem = static_cast<uint8_t*>(file.mmap());

	for (size_t i = 0; i < size; i++) {
		std::cout << mem[i];
	}

	std::cout << std::endl;
}

int main(void) {
	try {
		write_file();
		read_file();
	}

	// filesystem-related errors throw 'fs::Generic_error' exceptions

	catch (fs::Generic_error& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
