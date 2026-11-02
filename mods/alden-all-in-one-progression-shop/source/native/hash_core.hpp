#pragma once

#include <windows.h>
#include <bcrypt.h>
#include <stdint.h>
#include <string.h>

namespace mh067 {

enum class HashFailure : uint8_t {
    none,
    open,
    cng_open_algorithm,
    cng_get_property,
    cng_object_too_large,
    cng_object_alloc,
    cng_create_hash,
    read,
    cng_hash_data,
    cng_finish,
};

struct HashDiagnostics {
    HashFailure failure{HashFailure::none};
    DWORD win32Error{ERROR_SUCCESS};
    NTSTATUS cngStatus{0};
    uint64_t bytesRead{};
    DWORD objectLength{};
    char digest[65]{};
};

inline const char* HashFailureName(HashFailure failure) {
    switch (failure) {
    case HashFailure::none: return "none";
    case HashFailure::open: return "hash_open_failed";
    case HashFailure::cng_open_algorithm: return "hash_cng_open_algorithm_failed";
    case HashFailure::cng_get_property: return "hash_cng_get_property_failed";
    case HashFailure::cng_object_too_large: return "hash_cng_object_too_large";
    case HashFailure::cng_object_alloc: return "hash_cng_object_alloc_failed";
    case HashFailure::cng_create_hash: return "hash_cng_create_hash_failed";
    case HashFailure::read: return "hash_read_failed";
    case HashFailure::cng_hash_data: return "hash_cng_hash_data_failed";
    case HashFailure::cng_finish: return "hash_cng_finish_failed";
    }
    return "hash_failed";
}

inline void HashToHex(const uint8_t* digest, char* out) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < 32; ++i) {
        out[i * 2] = hex[digest[i] >> 4];
        out[i * 2 + 1] = hex[digest[i] & 15];
    }
    out[64] = 0;
}

inline bool Sha256File(const wchar_t* path, char out[65], HashDiagnostics* diagnostic = nullptr) {
    HashDiagnostics local{};
    HashDiagnostics& d = diagnostic ? *diagnostic : local;
    memset(&d, 0, sizeof(d));
    HANDLE file = CreateFileW(path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        d.failure = HashFailure::open;
        d.win32Error = GetLastError();
        return false;
    }

    BCRYPT_ALG_HANDLE alg{};
    BCRYPT_HASH_HANDLE hash{};
    DWORD objLen = 0, got = 0;
    uint8_t* object = nullptr;
    uint8_t digest[32]{};
    uint8_t buf[64 * 1024]{};
    NTSTATUS status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (status != 0) {
        d.failure = HashFailure::cng_open_algorithm; d.cngStatus = status; d.win32Error = GetLastError();
    } else if ((status = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objLen), sizeof(objLen), &got, 0)) != 0) {
        d.failure = HashFailure::cng_get_property; d.cngStatus = status; d.win32Error = GetLastError();
    } else if ((d.objectLength = objLen) > 4096) {
        d.failure = HashFailure::cng_object_too_large; d.cngStatus = static_cast<NTSTATUS>(0x80000005L); d.win32Error = GetLastError();
    } else if (!(object = static_cast<uint8_t*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, objLen)))) {
        d.failure = HashFailure::cng_object_alloc; d.win32Error = GetLastError();
    } else if ((status = BCryptCreateHash(alg, &hash, object, objLen, nullptr, 0, 0)) != 0) {
        d.failure = HashFailure::cng_create_hash; d.cngStatus = status; d.win32Error = GetLastError();
    } else {
        for (;;) {
            DWORD read = 0;
            if (!ReadFile(file, buf, sizeof(buf), &read, nullptr)) {
                d.failure = HashFailure::read; d.win32Error = GetLastError(); break;
            }
            if (read == 0) {
                status = BCryptFinishHash(hash, digest, sizeof(digest), 0);
                if (status != 0) {
                    d.failure = HashFailure::cng_finish; d.cngStatus = status; d.win32Error = GetLastError();
                } else {
                    HashToHex(digest, out); memcpy(d.digest, out, sizeof(d.digest) - 1);
                    BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(alg, 0); HeapFree(GetProcessHeap(), 0, object); CloseHandle(file); return true;
                }
                break;
            }
            d.bytesRead += read;
            status = BCryptHashData(hash, buf, read, 0);
            if (status != 0) { d.failure = HashFailure::cng_hash_data; d.cngStatus = status; d.win32Error = GetLastError(); break; }
        }
    }
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    if (object) HeapFree(GetProcessHeap(), 0, object);
    CloseHandle(file);
    return false;
}

} // namespace mh067
