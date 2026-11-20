// Copyright 2026 The Mast Authors

#include "mast/common/mast_version.h"

#include <string>
#include <utility>

#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"

namespace mast {

namespace {

std::string& Version() {
  static base::NoDestructor<std::string> version;
  return *version;
}

bool IsDigit(char16_t c) {
  return c >= u'0' && c <= u'9';
}

}  // namespace

const std::string& InstalledVersion() {
  return Version();
}

void SetInstalledVersion(std::string version) {
  Version() = std::move(version);
}

std::u16string WithMastVersion(const std::u16string& chromium_line) {
  if (Version().empty()) {
    return chromium_line;
  }
  // Chromium's version: the first run of digits and dots with three dots
  // (157.0.8103.78). The words around it are translated, so look for the
  // number, not for "Version".
  for (size_t start = 0; start < chromium_line.size(); ++start) {
    if (!IsDigit(chromium_line[start]) ||
        (start > 0 && (IsDigit(chromium_line[start - 1]) ||
                       chromium_line[start - 1] == u'.'))) {
      continue;
    }
    size_t end = start;
    int dots = 0;
    while (end < chromium_line.size() &&
           (IsDigit(chromium_line[end]) || chromium_line[end] == u'.')) {
      dots += chromium_line[end] == u'.';
      ++end;
    }
    if (dots == 3 && IsDigit(chromium_line[end - 1])) {
      const std::u16string chromium = chromium_line.substr(start, end - start);
      return chromium_line.substr(0, start) + base::UTF8ToUTF16(Version()) +
             u" (Chromium " + chromium + u")" + chromium_line.substr(end);
    }
    start = end;
  }
  return chromium_line;
}

}  // namespace mast
