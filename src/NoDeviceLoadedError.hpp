#ifndef __NODEVICELOADEDERROR_HPP__
#define __NODEVICELOADEDERROR_HPP__
#include <stdexcept>
class NoDeviceLoadedError : public std::runtime_error{
    public:
        NoDeviceLoadedError() noexcept;
};

#endif