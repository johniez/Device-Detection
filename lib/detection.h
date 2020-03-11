/**
 * \file
 * \brief Device detection from user agent header.
 */

#pragma once

#include <memory>
#include <string>

namespace devicedetection {


/**
 * Device detection from User-Agent header strings.
 */
class DeviceDetection {
    class Impl;
    std::unique_ptr<Impl> impl;
  public:
    /**
     * Create the detector using 51Degree Trie dataset file \p fileName.
     * Throws an exception when data could not be loaded.
     */
    explicit DeviceDetection(const std::string &fileName);
    ~DeviceDetection();

    /**
     * Reload dataset using same file as the detector was initially constructed with.
     * Throws an exception when data could not be loaded and the old data are in
     * use. The caller must consider fatality of this failure.
     */
    void reload();

    /// Detection result.
    enum class Result {
        UNKNOWN,    ///< Unknown or any other device than desktop, mobile or tablet.
        DESKTOP,    ///< Device detected as a desktop.
        MOBILE,     ///< Device detected as a cell phone.
        TABLET      ///< Device detected as a tablet.
    };

    /**
     * Detect device from given user agent. Thread safe.
     */
    Result detect(const std::string &userAgent);
};


}
