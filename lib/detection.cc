#include "detection.h"

#include <stdexcept>
#include <string.h>
#include "../src/trie/51Degrees.h"


/**
 * Throws an exception based on status of the data file initialization.
 */
static void throwDatasetInitStatusMessage(
        fiftyoneDegreesDataSetInitStatus status, const std::string &fileName)
{
    switch (status) {
        case DATA_SET_INIT_STATUS_INSUFFICIENT_MEMORY:
            throw std::runtime_error{"Insufficient memory to load '" + fileName + "'"};
        case DATA_SET_INIT_STATUS_CORRUPT_DATA:
            throw std::runtime_error{"Device data file '" + fileName + "' is corrupted."};
        case DATA_SET_INIT_STATUS_INCORRECT_VERSION:
            throw std::runtime_error{"Device data file '" + fileName + "' is not correct version."};
        case DATA_SET_INIT_STATUS_FILE_NOT_FOUND:
            throw std::runtime_error{"Device data file '" + fileName + "' not found."};
        default:
            break;
    }
    throw std::runtime_error{"Device data file '" + fileName + "' could not be loaded."};
}


namespace devicedetection {


class DeviceDetection::Impl {
    /// Detection implementation from 51Degrees library.
    fiftyoneDegreesProvider provider;
  public:
    /**
     * Construct Device Detection for properties IsTablet, IsMobile and DeviceType.
     * \param fileName  Path to 51Degrees trie data file.
     */
    Impl(const std::string &fileName) {
        const char *properties = "IsTablet,IsMobile,DeviceType";
        fiftyoneDegreesDataSetInitStatus status
            = fiftyoneDegreesInitProviderWithPropertyString(
                    fileName.c_str(), &provider, properties);
        if (status != DATA_SET_INIT_STATUS_SUCCESS) {
            throwDatasetInitStatusMessage(status, fileName);
        }
    }

    ~Impl() {
        fiftyoneDegreesProviderFree(&provider);
    }

    /// Reload data file instance was constructed with.
    void reload() {
        fiftyoneDegreesDataSetInitStatus status
            = fiftyoneDegreesProviderReloadFromFile(&provider);
        if (status != DATA_SET_INIT_STATUS_SUCCESS) {
            throwDatasetInitStatusMessage(status, provider.active->dataSet->fileName);
        }
    }

    /**
     * Detect device from User-Agent string.
     */
    DeviceDetection::Result detect(const std::string &userAgent) {
        fiftyoneDegreesDataSet* dataSet = provider.active->dataSet;
        // until func end it should not throw, or the ref-count won't be decremented
        fiftyoneDegreesDeviceOffsets *offsets
            = fiftyoneDegreesProviderCreateDeviceOffsets(&provider);
        offsets->size = 1;

        fiftyoneDegreesSetDeviceOffset(dataSet, userAgent.c_str(), 0, offsets->firstOffset);
        const char *device = readProperty(offsets, "DeviceType");
        const char *tablet = readProperty(offsets, "IsTablet");
        const char *mobile = readProperty(offsets, "IsMobile");

        DeviceDetection::Result res = DeviceDetection::Result::UNKNOWN;
        if (tablet && strcmp(tablet, "True") == 0) {
            res = DeviceDetection::Result::TABLET;
        } else if (mobile && strcmp(mobile, "True") == 0) {
            res = DeviceDetection::Result::MOBILE;
        } else if (device && strcmp(device, "Desktop") == 0) {
            res = DeviceDetection::Result::DESKTOP;
        }
        // decrement ref-count for dataset and free offsets
        fiftyoneDegreesProviderFreeDeviceOffsets(offsets);
        return res;
    }
  private:
    /**
     * Read detection property value from given offsets.
     * \param property  Property name.
     * \retval nullptr  Property was not found.
     */
    static const char *readProperty(
            fiftyoneDegreesDeviceOffsets *offsets,
            const char *property) noexcept
    {
        fiftyoneDegreesDataSet *dataSet = offsets->active->dataSet;
        const int32_t requiredPropertyIndex
            = fiftyoneDegreesGetRequiredPropertyIndex(dataSet, property);
        if (requiredPropertyIndex >= 0
                && requiredPropertyIndex <
                   fiftyoneDegreesGetRequiredPropertiesCount(dataSet)) {
            return fiftyoneDegreesGetValuePtrFromOffsets(
                    dataSet, offsets, requiredPropertyIndex);
        }

        return nullptr;
    }
};


DeviceDetection::DeviceDetection(const std::string &fileName)
    : impl(new Impl(fileName))
{}


DeviceDetection::~DeviceDetection() = default;


void DeviceDetection::reload() {
    impl->reload();
}


DeviceDetection::Result DeviceDetection::detect(const std::string &userAgent) {
    return impl->detect(userAgent);
}


}
