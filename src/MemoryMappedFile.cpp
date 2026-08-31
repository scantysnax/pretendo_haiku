
#include "MemoryMappedFile.h"

#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(__linux__) || defined(__HAIKU__)
	#include <sys/mman.h>
	#include <unistd.h>
#endif // (__linux__ || __HAIKU__)


// -----------------------------------------------------------------------------
// MemoryMappedFile::MemoryMappedFile
//
// Opens or creates a backing file and maps it into memory for persistent SRAM
// storage.
//
// On Linux and Haiku, the file is resized to the requested length and mapped
// with shared read/write access.  If memory mapping fails, or when building on
// an unsupported platform, a heap-allocated buffer is used instead.
//
// Parameters:
//   filename - Path to the backing file.
//   size     - Requested mapped-buffer size in bytes.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
MemoryMappedFile::MemoryMappedFile(const std::string &filename, size_t size) {
	(void)filename;

#if defined(__linux__) || defined(__HAIKU__)
	int fd = ::open(filename.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd != -1) {
		::ftruncate(fd, size);

		auto p = static_cast<uint8_t *>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
		if (p != MAP_FAILED) {

			ptr_     = p;
			deleter_ = [size](uint8_t *ptr) {
				::munmap(ptr, size);
			};
		} else {
			std::cerr << "Failed to map SRAM to file, using fallback implementation" << std::endl;

			ptr_     = new uint8_t[size];
			deleter_ = [](uint8_t *ptr) {
				delete[] ptr;
			};
		}

		::close(fd);
	}
#else
	std::cerr << "Failed to map SRAM to file, using fallback implementation" << std::endl;

	ptr_     = new uint8_t[size];
	deleter_ = [](uint8_t *ptr) {
		delete[] ptr;
	};
#endif
}


// -----------------------------------------------------------------------------
// MemoryMappedFile::~MemoryMappedFile
//
// Releases the mapped or heap-allocated buffer using the deleter selected when
// the object was constructed.
//
// Parameters:
//   None.
//
// Returns:
//   Nothing.
// -----------------------------------------------------------------------------
MemoryMappedFile::~MemoryMappedFile() {
	if (deleter_) {
		deleter_(ptr_);
	}
}


// -----------------------------------------------------------------------------
// MemoryMappedFile::operator[]
//
// Returns the byte stored at the specified position in the mapped buffer.
//
// Parameters:
//   index - Zero-based byte index.
//
// Returns:
//   Byte value at the requested position.
// -----------------------------------------------------------------------------
uint8_t MemoryMappedFile::operator[](size_t index) const {
	return ptr_[index];
}


// -----------------------------------------------------------------------------
// MemoryMappedFile::operator[]
//
// Returns a writable reference to the byte at the specified position in the
// mapped buffer.
//
// Parameters:
//   index - Zero-based byte index.
//
// Returns:
//   Reference to the byte at the requested position.
// -----------------------------------------------------------------------------
uint8_t &MemoryMappedFile::operator[](size_t index) {
	return ptr_[index];
}

