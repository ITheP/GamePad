#pragma once

#include <Arduino.h>
#include <string>

namespace Security {

// Load existing HTTPS certificates from NVS, or return false if not found
// These are declared in Prefs but defined here for convenience
bool LoadHTTPSCertificates(std::string &certPem, std::string &keyPem);

// Save HTTPS certificates to NVS
// These are declared in Prefs but defined here for convenience
void SaveHTTPSCertificates(const std::string &certPem, const std::string &keyPem);

// Load from NVS if available, or generate new certificates on first boot
// Automatically regenerates if certificate has expired
bool LoadOrGenerateHTTPSCertificates(std::string &certPem, std::string &keyPem);

// Manually regenerate HTTPS certificates (e.g., after security breach)
bool RegenerateHTTPSCertificates(std::string &certPem, std::string &keyPem);

}  // namespace Security
