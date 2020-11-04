#include "NoDeviceLoadedError.hpp"

NoDeviceLoadedError::NoDeviceLoadedError() noexcept:runtime_error("Error: No device loaded after engine construction"){}