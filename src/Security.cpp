#include "Security.h"
#include "Prefs.h"
#include <Preferences.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/md.h>
#include <cstring>
#include <ctime>
#include <WiFi.h>

// HTTPS Certificate generation and management
// All certificates are stored in NVS and generated on first boot
// Certificates are self-signed with ECC P-256 keys


namespace Security {

// Helper function to get device MAC address
static std::string GetDeviceMACAddress() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(macStr);
}

// Helper function to check if certificate has expired
static bool IsCertificateExpired(const std::string &certPem) {
    if (certPem.empty()) {
        return true;
    }

    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);

    int ret = mbedtls_x509_crt_parse(&cert, (const unsigned char *)certPem.c_str(), certPem.length() + 1);
    if (ret != 0) {
        mbedtls_x509_crt_free(&cert);
        return true;
    }

    // Get current time
    time_t now = time(nullptr);
    struct tm *timeinfo = gmtime(&now);

    // Debug output
    Serial.printf("Certificate valid_to: %04d-%02d-%02d %02d:%02d:%02d\n",
                  cert.valid_to.year + 1900, cert.valid_to.mon, cert.valid_to.day,
                  cert.valid_to.hour, cert.valid_to.min, cert.valid_to.sec);
    Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                  timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    // Check if cert has expired by comparing dates
    bool expired = false;
    if (cert.valid_to.year < 1970 + (timeinfo->tm_year)) {
        expired = true;
    } else if (cert.valid_to.year == 1970 + (timeinfo->tm_year)) {
        if (cert.valid_to.mon < (timeinfo->tm_mon + 1)) {
            expired = true;
        } else if (cert.valid_to.mon == (timeinfo->tm_mon + 1)) {
            if (cert.valid_to.day < timeinfo->tm_mday) {
                expired = true;
            } else if (cert.valid_to.day == timeinfo->tm_mday) {
                if (cert.valid_to.hour < timeinfo->tm_hour) {
                    expired = true;
                }
            }
        }
    }

    mbedtls_x509_crt_free(&cert);
    return expired;
}

// Generate a new self-signed HTTPS certificate
static bool GenerateHTTPSCertificate(std::string &certPem, std::string &keyPem) {
    int ret = 0;
    int cert_len = 0;
    int key_len = 0;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_pk_context key;
    mbedtls_x509write_cert crt;
    mbedtls_mpi serial_mpi;
    unsigned char serial[16];
    std::string cn;  // Moved here to avoid goto crossing initialization
    
    // Use static buffers to avoid malloc fragmentation when HTTP server is running
    // These are large but allocated once and reused
    static unsigned char cert_buffer[8192];
    static unsigned char key_buffer[8192];
    const size_t pem_buffer_size = sizeof(cert_buffer);
    
    // Zero buffers before use to ensure no garbage data
    memset(cert_buffer, 0, pem_buffer_size);
    memset(key_buffer, 0, pem_buffer_size);

    // Initialize contexts
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_mpi_init(&serial_mpi);
    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&crt);

    // Seed the random number generator
    const char *pers = "gamepad_https";
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        Serial.printf("Failed to seed RNG: %d\n", ret);
        goto cleanup;
    }

    // Generate ECC P-256 key pair
    ret = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    if (ret != 0) {
        Serial.printf("Failed to setup key: %d\n", ret);
        goto cleanup;
    }

    ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1,
                              mbedtls_pk_ec(key),
                              mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        Serial.printf("Failed to generate ECC key: %d\n", ret);
        goto cleanup;
    }

    // Set certificate fields
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    
    // Generate random serial number
    mbedtls_ctr_drbg_random(&ctr_drbg, serial, sizeof(serial));
    ret = mbedtls_mpi_read_binary(&serial_mpi, serial, sizeof(serial));
    if (ret != 0) {
        Serial.printf("Failed to read serial into mpi: %d\n", ret);
        goto cleanup;
    }

    ret = mbedtls_x509write_crt_set_serial(&crt, &serial_mpi);
    if (ret != 0) {
        Serial.printf("Failed to set serial: %d\n", ret);
        goto cleanup;
    }

    // Set issuer and subject name with MAC address
    cn = "TheController:" + GetDeviceMACAddress();
    
    ret = mbedtls_x509write_crt_set_subject_name(&crt, ("CN=" + cn).c_str());
    if (ret != 0) {
        Serial.printf("Failed to set subject name: %d\n", ret);
        goto cleanup;
    }

    ret = mbedtls_x509write_crt_set_issuer_name(&crt, ("CN=" + cn).c_str());
    if (ret != 0) {
        Serial.printf("Failed to set issuer name: %d\n", ret);
        goto cleanup;
    }

    // Valid from 2026-01-01 00:00:01, valid for 10 years
    ret = mbedtls_x509write_crt_set_validity(&crt, "20260101000001", "20360101000001");
    if (ret != 0) {
        Serial.printf("Failed to set validity: %d\n", ret);
        goto cleanup;
    }

    // Attach subject/issuer keys (self-signed)
    mbedtls_x509write_crt_set_subject_key(&crt, &key);
    mbedtls_x509write_crt_set_issuer_key(&crt, &key);

    // Set MD algorithm
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    
    // Add basicConstraints extension (mark as CA for self-signed)
    ret = mbedtls_x509write_crt_set_basic_constraints(&crt, 1, -1);
    if (ret != 0) {
        Serial.printf("Failed to set basic constraints: %d\n", ret);
        goto cleanup;
    }

    // Self-sign the certificate
    ret = mbedtls_x509write_crt_pem(&crt, cert_buffer, sizeof(cert_buffer),
                                    mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret < 0) {
        Serial.printf("Failed to sign/write certificate: %d\n", ret);
        goto cleanup;
    }
    // mbedtls_x509write_crt_pem returns negative on success (negative of bytes written)
    // mbedtls already includes null terminator in the output, so just use it
    cert_len = -ret;
    certPem = std::string((const char *)cert_buffer);

    // Write private key to PEM format
    ret = mbedtls_pk_write_key_pem(&key, key_buffer, sizeof(key_buffer));
    if (ret < 0) {
        Serial.printf("Failed to write key PEM: %d\n", ret);
        goto cleanup;
    }
    // mbedtls_pk_write_key_pem returns negative on success (negative of bytes written)
    // mbedtls already includes null terminator in the output, so just use it
    key_len = -ret;
    keyPem = std::string((const char *)key_buffer);

    Serial.println("✅ HTTPS certificate generated successfully");
    Serial.print("   CN: ");
    Serial.println(cn.c_str());

cleanup:
    mbedtls_x509write_crt_free(&crt);
    mbedtls_mpi_free(&serial_mpi);
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    
    // Static buffers - no need to free
    return (ret == 0);
}

// Load or generate HTTPS certificates
bool LoadOrGenerateHTTPSCertificates(std::string &certPem, std::string &keyPem) {
    // Try to load from NVS first
    if (LoadHTTPSCertificates(certPem, keyPem)) {
        // Check if certificate has expired
        if (IsCertificateExpired(certPem)) {
            Serial.println("⚠️  Certificate has expired, regenerating...");
            if (GenerateHTTPSCertificate(certPem, keyPem)) {
                SaveHTTPSCertificates(certPem, keyPem);
                return true;
            }
            return false;
        }
        Serial.println("✅ HTTPS certificates loaded from NVS");
        return true;
    }

    // Generate new certificates
    Serial.println("📝 Generating new HTTPS certificates...");
    if (GenerateHTTPSCertificate(certPem, keyPem)) {
        SaveHTTPSCertificates(certPem, keyPem);
        return true;
    }

    return false;
}

// Regenerate HTTPS certificates on demand
bool RegenerateHTTPSCertificates(std::string &certPem, std::string &keyPem) {
    Serial.println("🔄 Regenerating HTTPS certificates...");
    if (GenerateHTTPSCertificate(certPem, keyPem)) {
        SaveHTTPSCertificates(certPem, keyPem);
        Serial.println("✅ HTTPS certificates regenerated and saved");
        return true;
    }
    Serial.println("❌ Failed to regenerate HTTPS certificates");
    return false;
}

// Load HTTPS certificates from NVS
bool LoadHTTPSCertificates(std::string &certPem, std::string &keyPem) {
    return Prefs::LoadHTTPSCertificates(certPem, keyPem);
}

// Save HTTPS certificates to NVS
void SaveHTTPSCertificates(const std::string &certPem, const std::string &keyPem) {
    Prefs::SaveHTTPSCertificates(certPem, keyPem);
}

}  // namespace Security
