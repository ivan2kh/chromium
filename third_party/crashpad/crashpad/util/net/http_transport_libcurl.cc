// Copyright 2017 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/net/http_transport.h"

#include <curl/curl.h>
#include <string.h>
#include <sys/utsname.h>

#include <algorithm>
#include <limits>

#include "base/logging.h"
#include "base/numerics/safe_math.h"
#include "base/scoped_generic.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "package.h"
#include "util/net/http_body.h"
#include "util/numeric/safe_assignment.h"

namespace crashpad {

namespace {

std::string UserAgent() {
  std::string user_agent = base::StringPrintf(
      "%s/%s %s", PACKAGE_NAME, PACKAGE_VERSION, curl_version());

  utsname os;
  if (uname(&os) != 0) {
    PLOG(WARNING) << "uname";
  } else {
    // Match the architecture name that would be used by the kernel, so that the
    // strcmp() below can omit the kernel’s architecture name if it’s the same
    // as the user process’ architecture. On Linux, these names are normally
    // defined in each architecture’s Makefile as UTS_MACHINE, but can be
    // overridden in architecture-specific configuration as COMPAT_UTS_MACHINE.
    // See linux-4.4.52/arch/*/Makefile and
    // linux-4.4.52/arch/*/include/asm/compat.h. In turn, on some systems, these
    // names are further overridden or refined in early kernel startup code by
    // modifying the string returned by linux-4.4.52/include/linux/utsname.h
    // init_utsname() as noted.
#if defined(ARCH_CPU_X86)
    // linux-4.4.52/arch/x86/kernel/cpu/bugs.c check_bugs() sets the first digit
    // to 4, 5, or 6, but no higher. Assume 6.
    const char arch[] = "i686";
#elif defined(ARCH_CPU_X86_64)
    const char arch[] = "x86_64";
#elif defined(ARCH_CPU_ARMEL)
    // linux-4.4.52/arch/arm/kernel/setup.c setup_processor() bases the string
    // on the ARM processor name and a character identifying little- or
    // big-endian. The processor name comes from a definition in
    // arch/arm/mm/proc-*.S. Assume armv7, little-endian.
    const char arch[] = "armv7l";
#elif defined(ARCH_CPU_ARM64)
    // ARM64 uses aarch64 or aarch64_be as directed by ELF_PLATFORM. See
    // linux-4.4.52/arch/arm64/kernel/setup.c setup_arch(). Assume
    // little-endian.
    const char arch[] = "aarch64";
#elif defined(ARCH_CPU_MIPSEL)
    const char arch[] = "mips";
#elif defined(ARCH_CPU_MIPS64EL)
    const char arch[] = "mips64";
#else
#error Port
#endif

    user_agent.append(
        base::StringPrintf(" %s/%s (%s", os.sysname, os.release, arch));
    if (strcmp(arch, os.machine) != 0) {
      user_agent.append(base::StringPrintf("; %s", os.machine));
    }
    user_agent.append(1, ')');
  }

  return user_agent;
}

std::string CurlErrorMessage(CURLcode curl_err, const std::string& base) {
  return base::StringPrintf(
      "%s: %s (%d)", base.c_str(), curl_easy_strerror(curl_err), curl_err);
}

struct ScopedCURLTraits {
  static CURL* InvalidValue() { return nullptr; }
  static void Free(CURL* curl) {
    if (curl) {
      curl_easy_cleanup(curl);
    }
  }
};
using ScopedCURL = base::ScopedGeneric<CURL*, ScopedCURLTraits>;

class CurlSList {
 public:
  CurlSList() : list_(nullptr) {}
  ~CurlSList() {
    if (list_) {
      curl_slist_free_all(list_);
    }
  }

  curl_slist* get() const { return list_; }

  bool Append(const char* data) {
    curl_slist* list = curl_slist_append(list_, data);
    if (!list_) {
      list_ = list;
    }
    return list != nullptr;
  }

 private:
  curl_slist* list_;

  DISALLOW_COPY_AND_ASSIGN(CurlSList);
};

class ScopedClearString {
 public:
  explicit ScopedClearString(std::string* string) : string_(string) {}

  ~ScopedClearString() {
    if (string_) {
      string_->clear();
    }
  }

  void Disarm() { string_ = nullptr; }

 private:
  std::string* string_;

  DISALLOW_COPY_AND_ASSIGN(ScopedClearString);
};

class HTTPTransportLibcurl final : public HTTPTransport {
 public:
  HTTPTransportLibcurl();
  ~HTTPTransportLibcurl() override;

  // HTTPTransport:
  bool ExecuteSynchronously(std::string* response_body) override;

 private:
  static size_t ReadRequestBody(char* buffer,
                                size_t size,
                                size_t nitems,
                                void* userdata);
  static size_t WriteResponseBody(char* buffer,
                                  size_t size,
                                  size_t nitems,
                                  void* userdata);

  DISALLOW_COPY_AND_ASSIGN(HTTPTransportLibcurl);
};

HTTPTransportLibcurl::HTTPTransportLibcurl() : HTTPTransport() {}

HTTPTransportLibcurl::~HTTPTransportLibcurl() {}

bool HTTPTransportLibcurl::ExecuteSynchronously(std::string* response_body) {
  DCHECK(body_stream());

  response_body->clear();

  // curl_easy_init() will do this on the first call if it hasn’t been done yet,
  // but not in a thread-safe way as is done here.
  static CURLcode curl_global_init_err = []() {
    return curl_global_init(CURL_GLOBAL_DEFAULT);
  }();
  if (curl_global_init_err != CURLE_OK) {
    LOG(ERROR) << CurlErrorMessage(curl_global_init_err, "curl_global_init");
    return false;
  }

  CurlSList curl_headers;
  ScopedCURL curl(curl_easy_init());
  if (!curl.get()) {
    LOG(ERROR) << "curl_easy_init";
    return false;
  }

// These macros wrap the repetitive “try something, log an error and return
// false on failure” pattern. Macros are convenient because the log messages
// will point to the correct line number, which can help pinpoint a problem when
// there are as many calls to these functions as there are here.
#define TRY_CURL_EASY_SETOPT(curl, option, parameter)                    \
  do {                                                                   \
    CURLcode curl_err = curl_easy_setopt((curl), (option), (parameter)); \
    if (curl_err != CURLE_OK) {                                          \
      LOG(ERROR) << CurlErrorMessage(curl_err, "curl_easy_setopt");      \
      return false;                                                      \
    }                                                                    \
  } while (false)
#define TRY_CURL_SLIST_APPEND(slist, data) \
  do {                                     \
    if (!(slist).Append(data)) {           \
      LOG(ERROR) << "curl_slist_append";   \
      return false;                        \
    }                                      \
  } while (false)

  TRY_CURL_EASY_SETOPT(curl.get(), CURLOPT_USERAGENT, UserAgent().c_str());

  // Accept and automatically decode any encoding that libcurl understands.
  TRY_CURL_EASY_SETOPT(curl.get(), CURLOPT_ACCEPT_ENCODING, "");

  TRY_CURL_EASY_SETOPT(curl.get(), CURLOPT_URL, url().c_str());

  const int kMillisecondsPerSecond = 1E3;
  TRY_CURL_EASY_SETOPT(curl.get(),
                       CURLOPT_TIMEOUT_MS,
                       static_cast<long>(timeout() * kMillisecondsPerSecond));

  // If the request body size is known ahead of time, a Content-Length header
  // field will be present. Store that to use as CURLOPT_POSTFIELDSIZE_LARGE,
  // which will both set the Content-Length field in the request header and
  // inform libcurl of the request body size. Otherwise, use Transfer-Encoding:
  // chunked, which does not require advance knowledge of the request body size.
  bool chunked = true;
  size_t content_length;
  for (const auto& pair : headers()) {
    if (pair.first == kContentLength) {
      chunked = !base::StringToSizeT(pair.second, &content_length);
      DCHECK(!chunked);
    } else {
      TRY_CURL_SLIST_APPEND(curl_headers,
                            (pair.first + ": " + pair.second).c_str());
    }
  }

  if (method() == "POST") {
    TRY_CURL_EASY_SETOPT(curl.get(), CURLOPT_POST, 1l);

    // By default when sending a POST request, libcurl includes an “Expect:
    // 100-continue” header field. Althogh this header is specified in HTTP/1.1
    // (RFC 2616 §8.2.3, RFC 7231 §5.1.1), even collection servers that claim to
    // speak HTTP/1.1 may not respond to it. When sending this header field,
    // libcurl will wait for one second for the server to respond with a “100
    // Continue” status before continuing to transmit the request body. This
    // delay is avoided by telling libcurl not to send this header field at all.
    // The drawback is that certain HTTP error statuses may not be received
    // until after substantial amounts of data have been sent to the server.
    TRY_CURL_SLIST_APPEND(curl_headers, "Expect:");

    if (chunked) {
      TRY_CURL_SLIST_APPEND(curl_headers, "Transfer-Encoding: chunked");
    } else {
      curl_off_t content_length_curl;
      if (!AssignIfInRange(&content_length_curl, content_length)) {
        LOG(ERROR) << base::StringPrintf("Content-Length %zu too large",
                                         content_length);
        return false;
      }
      TRY_CURL_EASY_SETOPT(
          curl.get(), CURLOPT_POSTFIELDSIZE_LARGE, content_length_curl);
    }
  } else if (method() != "GET") {
    // Untested.
    TRY_CURL_EASY_SETOPT(curl.get(), CURLOPT_CUSTOMREQUEST, method().c_str());
  }

  TRY_CURL_EASY_SETOPT(curl.get(), CURLOPT_HTTPHEADER, curl_headers.get());

  TRY_CURL_EASY_SETOPT(curl.get(), CURLOPT_READFUNCTION, ReadRequestBody);
  TRY_CURL_EASY_SETOPT(curl.get(), CURLOPT_READDATA, this);
  TRY_CURL_EASY_SETOPT(curl.get(), CURLOPT_WRITEFUNCTION, WriteResponseBody);
  TRY_CURL_EASY_SETOPT(curl.get(), CURLOPT_WRITEDATA, response_body);

#undef TRY_CURL_EASY_SETOPT
#undef TRY_CURL_SLIST_APPEND

  // If a partial response body is received and then a failure occurs, ensure
  // that response_body is cleared.
  ScopedClearString clear_response_body(response_body);

  // Do it.
  CURLcode curl_err = curl_easy_perform(curl.get());
  if (curl_err != CURLE_OK) {
    LOG(ERROR) << CurlErrorMessage(curl_err, "curl_easy_perform");
    return false;
  }

  long status;
  curl_err = curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
  if (curl_err != CURLE_OK) {
    LOG(ERROR) << CurlErrorMessage(curl_err, "curl_easy_getinfo");
    return false;
  }

  if (status != 200) {
    LOG(ERROR) << base::StringPrintf("HTTP status %ld", status);
    return false;
  }

  // The response body is complete. Don’t clear it.
  clear_response_body.Disarm();

  return true;
}

// static
size_t HTTPTransportLibcurl::ReadRequestBody(char* buffer,
                                             size_t size,
                                             size_t nitems,
                                             void* userdata) {
  HTTPTransportLibcurl* self =
      reinterpret_cast<HTTPTransportLibcurl*>(userdata);

  // This libcurl callback mimics the silly stdio-style fread() interface: size
  // and nitems have been separated and must be multiplied.
  base::CheckedNumeric<size_t> checked_len = base::CheckMul(size, nitems);
  size_t len = checked_len.ValueOrDefault(std::numeric_limits<size_t>::max());

  // Limit the read to what can be expressed in a FileOperationResult.
  len = std::min(
      len,
      static_cast<size_t>(std::numeric_limits<FileOperationResult>::max()));

  FileOperationResult bytes_read = self->body_stream()->GetBytesBuffer(
      reinterpret_cast<uint8_t*>(buffer), len);
  if (bytes_read < 0) {
    return CURL_READFUNC_ABORT;
  }

  return bytes_read;
}

// static
size_t HTTPTransportLibcurl::WriteResponseBody(char* buffer,
                                               size_t size,
                                               size_t nitems,
                                               void* userdata) {
  std::string* response_body = reinterpret_cast<std::string*>(userdata);

  // This libcurl callback mimics the silly stdio-style fread() interface: size
  // and nitems have been separated and must be multiplied.
  base::CheckedNumeric<size_t> checked_len = base::CheckMul(size, nitems);
  size_t len = checked_len.ValueOrDefault(std::numeric_limits<size_t>::max());

  response_body->append(buffer, len);
  return len;
}

}  // namespace

// static
std::unique_ptr<HTTPTransport> HTTPTransport::Create() {
  return std::unique_ptr<HTTPTransport>(new HTTPTransportLibcurl());
}

}  // namespace crashpad
