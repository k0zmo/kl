#include "kl/file_view.hpp"

#ifdef _WIN32

#include <system_error>
#include <cassert>
#include <Windows.h>

namespace kl {
namespace detail {

template <LONG_PTR null>
class handle
{
public:
    handle(HANDLE h = null_handle()) noexcept : h_{h} {}
    ~handle() { reset(); }

    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;

    handle(handle&& other) noexcept
    {
        swap(*this, other);
    }

    handle& operator=(handle&& other) noexcept
    {
        assert(this != &other);
        swap(*this, other);
        return *this;
    }

    friend void swap(handle& l, handle& r) noexcept
    {
        using std::swap;
        swap(l.h_, r.h_);
    }

    void reset(HANDLE h = null_handle()) noexcept
    {
        if (h_ != null_handle())
            ::CloseHandle(h_);
        h_ = h;
    }

    bool operator!() const noexcept { return h_ == null_handle(); }

    operator HANDLE() noexcept { return h_; }

    constexpr static HANDLE null_handle() noexcept
    {
        return reinterpret_cast<HANDLE>(null);
    }

private:
    HANDLE h_{null};
};

struct map_deleter
{
    using pointer = LPVOID;
    void operator()(pointer base_addr)
    { 
        ::UnmapViewOfFile(base_addr); 
    }
};

using file_view = std::unique_ptr<LPVOID, map_deleter>;

} // namespace detail

struct file_view::impl
{
    impl(gsl::cstring_span<> file_path)
    {
        file_handle_.reset(::CreateFileA(file_path.data(), GENERIC_READ, 0x0,
                                         nullptr, OPEN_EXISTING, 0x0, nullptr));
        if (!file_handle_)
            throw std::system_error{static_cast<int>(::GetLastError()),
                                    std::system_category()};

        mapping_handle_.reset(::CreateFileMappingA(
            file_handle_, nullptr, PAGE_READONLY, 0, 0, nullptr));
        if (!mapping_handle_)
        {
            throw std::system_error{static_cast<int>(::GetLastError()),
                                    std::system_category()};
        }

        file_view_.reset(
            ::MapViewOfFile(mapping_handle_, FILE_MAP_READ, 0, 0, 0));
        if (!file_view_)
        {
            throw std::system_error{static_cast<int>(::GetLastError()),
                                    std::system_category()};
        }

        LARGE_INTEGER file_size = {};
        ::GetFileSizeEx(file_handle_, &file_size);

        contents =
            gsl::make_span(static_cast<byte*>(file_view_.get()),
                           static_cast<std::ptrdiff_t>(file_size.QuadPart));
    }

    detail::handle<(LONG_PTR)-1> file_handle_;
    detail::handle<(LONG_PTR)0> mapping_handle_;
    detail::file_view file_view_{HANDLE(nullptr)};

    gsl::span<const byte> contents;
};
} // namespace kl

#else

#include <system_error>
#include <cassert>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace kl {
namespace detail {

class file_descriptor
{
public:
    file_descriptor(int fd = -1) noexcept : fd_{fd} {}
    ~file_descriptor() { reset(); }

    file_descriptor(const file_descriptor&) = delete;
    file_descriptor& operator=(const file_descriptor&) = delete;

    file_descriptor(file_descriptor&& other) noexcept
    {
        swap(*this, other);
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
} // namespace detail

struct file_view::impl
{
    impl(gsl::cstring_span<> file_path)
        : fd{::open(file_path.data(), O_RDONLY)}
    {
        if (!fd)
            throw std::system_error{static_cast<int>(errno),
                                    std::system_category()};

        struct stat file_info;
        if (fstat(fd, &file_info) == -1)
            throw std::system_error{static_cast<int>(errno),
                                    std::system_category()};

        if (!file_info.st_size)
            return;

        file_view =
            ::mmap(nullptr, file_info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (!file_view)
            throw std::system_error{static_cast<int>(errno),
                                    std::system_category()};

        contents =
            gsl::make_span(static_cast<const byte*>(file_view),
                           static_cast<std::ptrdiff_t>(file_info.st_size));
    }

    ~impl()
    {
        if (!contents.empty())
            ::munmap(file_view, contents.size_bytes());
    }

    detail::file_descriptor fd;
    void* file_view{nullptr};
    gsl::span<const byte> contents;
};
} // namespace kl

#endif

namespace kl {

file_view::file_view(gsl::cstring_span<> file_path)
    : impl_{std::make_unique<impl>(std::move(file_path))}
{
}

file_view::~file_view() = default;

gsl::span<const byte> file_view::get_bytes() const
{
    return impl_->contents;
}
} // namespace kl
