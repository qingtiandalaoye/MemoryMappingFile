#pragma once

#ifndef _MemoryMappingFile_H
#define _MemoryMappingFile_H

#include <iostream>
#include <assert.h>
#include <cstring>
#if defined(_WIN32)

    #include <windows.h>

//sync data to file:
//FlushViewOfFile(mappedFileAddress, MMAP_ALLOCATOR_SIZE);
    class MemoryMappingFile
    {
    public:
        MemoryMappingFile(std::string path, int size, bool isWriting, bool quickMode);
        ~MemoryMappingFile();

        bool is_open();
        size_t get_size();
        int size;
        bool quickMode;
        char* data();

    private:
        void cleanup();
        HANDLE hFile_;
        HANDLE hMapping_;
        size_t size_;
        void*  addr_;
    };


    inline MemoryMappingFile::MemoryMappingFile(std::string path, int size, bool isWriting, bool quickMode) :
        hFile_(NULL), hMapping_(NULL), size_(0), addr_(NULL), size(size), quickMode(quickMode)
    {
        //https://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
        hFile_ = ::CreateFileA(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile_ == INVALID_HANDLE_VALUE) {
            //std::cerr << "MemoryMappingFile fail! Could not CreateFileA:" << path << std::endl;
            std::cout << GetLastError() << std::endl;
            exit(EXIT_FAILURE);
        }

        size_ = ::GetFileSize(hFile_, NULL);
        //https://msdn.microsoft.com/en-us/library/aa366537(VS.85).aspx
        hMapping_ = ::CreateFileMapping(hFile_,// use paging file
            NULL,// default security
            PAGE_READWRITE,
            0,// max. object size
            64 * 1024, // buffer size
            NULL);// name of mapping object

        if (hMapping_ == NULL) {
            cleanup();
            //std::cerr << "MemoryMappingFile fail! Could not CreateFileMapping:" + path << std::endl;
            std::cout << GetLastError() << std::endl;
            exit(EXIT_FAILURE);
        }

        //https://msdn.microsoft.com/en-us/library/aa366761(VS.85).aspx
        addr_ = ::MapViewOfFile(hMapping_, // handle to map object
            FILE_MAP_ALL_ACCESS,// read/write permission
            0,
            0,
            0);

        if (addr_ == NULL) {
            cleanup();
            //std::cerr << "MemoryMappingFile fail! Could not MapViewOfFile:" + path << std::endl;
            std::cout << GetLastError() << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    inline MemoryMappingFile::~MemoryMappingFile()
    {
        cleanup();
    }

    inline bool MemoryMappingFile::is_open()
    {
        return addr_ != NULL;
    }

    inline size_t MemoryMappingFile::get_size()
    {
        return size_;
    }

    inline char* MemoryMappingFile::data()
    {
        return (char*)addr_;
    }

    inline void MemoryMappingFile::cleanup()
    {

        if (addr_) {
            ::UnmapViewOfFile(addr_);
            addr_ = NULL;
        }

        if (hMapping_) {
            ::CloseHandle(hMapping_);
            hMapping_ = NULL;
        }

        if (hFile_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
        }

        //if want to delete the file
        //unlink(mmFileName);
    }

#else  // LINUX??


#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
//sync data to file:
//msync(mappedFileAddress, MMAP_ALLOCATOR_SIZE, MS_SYNC);
    class MemoryMappingFile
    {
    public:
        MemoryMappingFile(std::string path, int size, bool isWriting, bool quickMode);
        ~MemoryMappingFile();

        bool is_open();
        size_t get_size();
        char* data();

    private:
        void cleanup();
        int    fd_;
        size_t size_;
        int size;
        bool quickMode;
        void*  addr_;
    };

    inline MemoryMappingFile::MemoryMappingFile(std::string path, int size, bool isWriting, bool quickMode):
        fd_(-1), size_(0), addr_(NULL), size(size), quickMode(quickMode) {


        int fd_ = open(path.c_str(), (isWriting) ? (O_RDWR | O_CREAT) : O_RDONLY, (mode_t)0600);
        if (fd_ < 0)
        {
            std::cerr << "MemoryMappingFile fail! Could not open file:" + path << std::endl;
            exit(EXIT_FAILURE);
        }

        struct stat sb;
        if (fstat(fd_, &sb) == -1) {
            cleanup();
            std::cerr << "MemoryMappingFile fail! Could not fstat file:" + path << std::endl;
            exit(EXIT_FAILURE);
        }
        size_ = sb.st_size;

        if (isWriting)
        {
            if (lseek(fd_, size - 1, SEEK_SET) == -1)
            {
                cleanup();
                std::cerr << "MemoryMappingFile fail! Error calling lseek() to 'stretch' the file:" + path << std::endl;
                exit(EXIT_FAILURE);
            }
            if (write(fd_, "", 1) == -1)
            {
                cleanup();
                std::cerr << "MemoryMappingFile fail! Error writing last byte of the file:" + path << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        addr_ = mmap(0, size, (isWriting) ? (PROT_READ | PROT_WRITE) : PROT_READ, MAP_SHARED, fd_, 0);

        if (addr_ == NULL)
        {
            cleanup();
            std::cerr << "MemoryMappingFile fail! Error mapping file to buffer:" + path << std::endl;
            exit(EXIT_FAILURE);
        }

        if (!quickMode && madvise(addr_, size, MADV_RANDOM) != 0 && mlock(addr_, size) != 0)
           {
               munmap(addr_, size);
               cleanup();
               std::cerr << "MemoryMappingFile fail! Error in madvise or mlock:" + path << std::endl;
               exit(EXIT_FAILURE);
           }
    }

    inline MemoryMappingFile::~MemoryMappingFile()
    {
        cleanup();
    }

    inline bool MemoryMappingFile::is_open()
    {
        return addr_ != NULL;
    }

    inline size_t MemoryMappingFile::get_size()
    {
        return size_;
    }

    inline char* MemoryMappingFile::data()
    {
        return (char*)addr_;
    }

    inline void MemoryMappingFile::cleanup()
    {
        if (!quickMode && munlock(addr_, size) != 0)
        {
            std::cerr << "MemoryMappingFile fail! ERROR in munlock" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (addr_ != NULL) {
            if(munmap(addr_, size)!=0)
            {
                std::cerr << "MemoryMappingFile fail! ERROR in munmap" << std::endl;
                exit(EXIT_FAILURE);
            }
            addr_ = NULL;
        }

        if (fd_ != -1) {
            close(fd_);
            fd_ = -1;
        }

        //if want to delete the file
        //unlink(mmFileName);
    }


#endif   //_WIN32

#endif  //_MemoryMappingFile_H
