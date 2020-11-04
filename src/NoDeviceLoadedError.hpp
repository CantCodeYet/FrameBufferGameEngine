#pragma once
#include <stdexcept>
class NoDeviceLoadedError : public std::runtime_error{
    public:
        NoDeviceLoadedError() noexcept;
};