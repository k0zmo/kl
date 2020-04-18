#include "kl/file_view.hpp"

#include <system_error>
#include <cassert>

struct IUnknown; // Required for /permissive- and WinSDK 8.1
#include <Windows.h>

namespace kl {
namespace {

template <typename NullPolicy>
class handle
{
public:
    handle(HANDLE h = null()) noexcept : h_{h} {}
    ~handle() { reset(); }

    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;

    handle(handle&& other) noexcept : h_{std::exchange(other.h_, null())} {}

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

    void reset(HANDLE h = null()) noexcept
    {
        if (h_ != null())
            ::CloseHandle(h_);
        h_ = h;
    }

    explicit operator bool() const noexcept { return h_ != null(); }

    HANDLE get() const noexcept { return h_; }

private:
    static HANDLE null() { return reinterpret_cast<HANDLE>(NullPolicy::empty); }

private:
    HANDLE h_;
};

struct null_handle_policy
{
    static constexpr LONG_PTR empty{};
};

struct invalid_handle_value_policy
{
    // INVALID_HANDLE_VALUE cannot be assigned to HANDLE in constexpr context
    static constexpr LONG_PTR empty = static_cast<LONG_PTR>(-1);
};
} // namespace

file_view::file_view(const char* file_path)
{
    handle<invalid_handle_value_policy> file_handle{::CreateFileA(
        file_path, GENERIC_READ, 0x0, nullptr, OPEN_EXISTING, 0x0, nullptr)};
    if (!file_handle)
        throw std::system_error{static_cast<int>(::GetLastError()),
                                std::system_category()};

    LARGE_INTEGER file_size = {};
    ::GetFileSizeEx(file_handle.get(), &file_size);
    if (!file_size.QuadPart)
        return; // Empty file

    handle<null_handle_policy> mapping_handle{::CreateFileMappingA(
        file_handle.get(), nullptr, PAGE_READONLY, 0, 0, nullptr)};
    if (!mapping_handle)
        throw std::system_error{static_cast<int>(::GetLastError()),
                                std::system_category()};

    void* file_view =
        ::MapViewOfFile(mapping_handle.get(), FILE_MAP_READ, 0, 0, 0);
    if (!file_view)
        throw std::system_error{static_cast<int>(::GetLastError()),
                                std::system_category()};

    contents_ =
        gsl::span<const byte>{static_cast<const byte*>(file_view),
                              static_cast<std::size_t>(file_size.QuadPart)};
}

file_view::~file_view()
{
    if (!contents_.empty())
        ::UnmapViewOfFile(contents_.data());
}
} // namespace kl
