#include "detection.h"

#include <stdexcept>
#include <stdio.h>
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


/// Dataset private data (ABI compatibility)
struct DataSet::Data {
    /// Memory for dataset, would be transfered into 51deg library
    std::unique_ptr<void, decltype(&free)> mem;
    /// Size of dataset allocated into mem.
    long bufSize;

    Data(): mem(nullptr, &free), bufSize(-1) {}
};


DataSet::DataSet(const std::string &fileName)
    : data(new Data{})
{
    std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(fileName.c_str(), "rb"), &fclose);
    if (!fp) {
        throwDatasetInitStatusMessage(DATA_SET_INIT_STATUS_FILE_NOT_FOUND, fileName);
    }

    // read file size using seek to end, ftell and seek to begin
    // note: not using stat(2) because it can point to different file than the open one
    if (fseek(fp.get(), 0L, SEEK_END) != 0) {
        throwDatasetInitStatusMessage(DATA_SET_INIT_STATUS_NOT_SET, fileName);
    }
    data->bufSize = ftell(fp.get());
    if (data->bufSize < 0) {
        throwDatasetInitStatusMessage(DATA_SET_INIT_STATUS_NOT_SET, fileName);
    }
    if (fseek(fp.get(), 0L, SEEK_SET) != 0) {
        throwDatasetInitStatusMessage(DATA_SET_INIT_STATUS_NOT_SET, fileName);
    }

    // allocate necessary memory using malloc(2), so it can be easily passed
    // to the underlying C library
    data->mem.reset(malloc(sizeof(char) * (data->bufSize + 1)));

    // read data into memory
    const size_t lenRead = fread(data->mem.get(), data->bufSize, 1, fp.get());
    if (lenRead != 1) {
        throwDatasetInitStatusMessage(DATA_SET_INIT_STATUS_CORRUPT_DATA, fileName);
    }
}


DataSet::~DataSet() = default;


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

    /// Reload data using provided dataset memory
    void reload(std::unique_ptr<DataSet::Data> &&data) {
        fiftyoneDegreesDataSetInitStatus status
            = fiftyoneDegreesProviderReloadFromMemory(
                    &provider, data->mem.get(), data->bufSize);
        if (status != DATA_SET_INIT_STATUS_SUCCESS) {
            throwDatasetInitStatusMessage(status, provider.active->dataSet->fileName);
        }
        // pass the ownership of data memory to the provider
        provider.active->dataSet->memoryToFree = data->mem.release();
    }

    /**
     * Detect device from User-Agent string.
     */
    DeviceDetection::Result detect(const std::string &userAgent) {
        // until func end it should not throw, or the ref-count won't be decremented
        fiftyoneDegreesDeviceOffsets *offsets
            = fiftyoneDegreesProviderCreateDeviceOffsets(&provider);
        offsets->size = 1;

        fiftyoneDegreesSetDeviceOffset(
                offsets->active->dataSet, userAgent.c_str(), 0, offsets->firstOffset);
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


void DeviceDetection::reload(DataSet &&data) {
    impl->reload(std::move(data.data));
}


DeviceDetection::Result DeviceDetection::detect(const std::string &userAgent) {
    return impl->detect(userAgent);
}


}
