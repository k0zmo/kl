#include "kl/file_view.hpp"

#include <system_error>
#include <cassert>

struct IUnknown; // Required for /permissive- and WinSDK 8.1
#include <Windows.h>

namespace kl {
namespace {

template <LONG_PTR null>
class handle
{
public:
    handle(HANDLE h = null_handle()) noexcept : h_{h} {}
    ~handle() { reset(); }

    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;

    handle(handle&& other) noexcept : h_{std::exchange(other.h_, null)} {}

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
} // namespace

file_view::file_view(const char* file_path)
{
    handle<(LONG_PTR)-1> file_handle{::CreateFileA(
        file_path, GENERIC_READ, 0x0, nullptr, OPEN_EXISTING, 0x0, nullptr)};
    if (!file_handle)
        throw std::system_error{static_cast<int>(::GetLastError()),
                                std::system_category()};

    LARGE_INTEGER file_size = {};
    ::GetFileSizeEx(file_handle, &file_size);
    if (!file_size.QuadPart)
        return; // Empty file

    handle<(LONG_PTR)0> mapping_handle{::CreateFileMappingA(
        file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr)};
    if (!mapping_handle)
        throw std::system_error{static_cast<int>(::GetLastError()),
                                std::system_category()};

    void* file_view = ::MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);
    if (!file_view)
        throw std::system_error{static_cast<int>(::GetLastError()),
                                std::system_category()};

    contents_ =
        gsl::make_span(static_cast<const byte*>(file_view), file_size.QuadPart);
}

file_view::~file_view()
{
    if (!contents_.empty())
        ::UnmapViewOfFile(contents_.data());
}
} // namespace kl
