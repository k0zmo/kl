#include "kl/file_view.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <system_error>
#include <cassert>

namespace kl {
namespace {

class file_descriptor
{
public:
    explicit file_descriptor(int fd = -1) noexcept : fd_{fd} {}
    ~file_descriptor() { reset(); }

    file_descriptor(const file_descriptor&) = delete;
    file_descriptor& operator=(const file_descriptor&) = delete;

    file_descriptor(file_descriptor&& other) noexcept
        : fd_{std::exchange(other.fd_, -1)}
    {
    }

    file_descriptor& operator=(file_descriptor&& other) noexcept
    {
        assert(this != &other);
        swap(*this, other);
        return *this;
    }

    friend void swap(file_descriptor& l, file_descriptor& r) noexcept
    {
        using std::swap;
        swap(l.fd_, r.fd_);
    }

    void reset(int fd = -1) noexcept
    {
        if (fd_ != -1)
            ::close(fd_);
        fd_ = fd;
    }

    bool operator!() const noexcept { return fd_ == -1; }

    operator int() noexcept { return fd_; }

private:
    int fd_{-1};
};
} // namespace

file_view::file_view(const char* file_path)
{
    file_descriptor fd{::open(file_path, O_RDONLY)};
    if (!fd)
        throw std::system_error{static_cast<int>(errno),
                                std::system_category()};

    struct stat file_info;
    if (fstat(fd, &file_info) == -1)
        throw std::system_error{static_cast<int>(errno),
                                std::system_category()};

    if (!file_info.st_size)
        return; // Empty file

    void* file_view =
        ::mmap(nullptr, file_info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!file_view)
        throw std::system_error{static_cast<int>(errno),
                                std::system_category()};

    contents_ = gsl::span{static_cast<const std::byte*>(file_view),
                          static_cast<std::size_t>(file_info.st_size)};
}

file_view::~file_view()
{
    if (!contents_.empty())
    {
        ::munmap(const_cast<std::byte*>(contents_.data()),
                 contents_.size_bytes());
    }
}
} // namespace kl
