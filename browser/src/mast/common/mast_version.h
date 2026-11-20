// Copyright 2026 The Mast Authors

#ifndef MAST_COMMON_MAST_VERSION_H_
#define MAST_COMMON_MAST_VERSION_H_

#include <string>

namespace mast {

// Mast's own version (0.1.0), which packaging/package.sh writes into the
// mast-version file beside the browser once the release is numbered (after
// the build, so it can't be compiled in). Read off the UI thread at startup
// (mast_update_check.cc) and set here. Empty for a development build (no file)
// and until it's read. UI thread only.
const std::string& InstalledVersion();
void SetInstalledVersion(std::string version);

// Chromium's version line ("Version 157.0.8103.78 (Official Build) (64-bit)",
// Settings > About) with Mast's version in front of Chromium's:
// "Version 0.1.0 (Chromium 157.0.8103.78) (Official Build) (64-bit)".
// Unchanged without a Mast version.
std::u16string WithMastVersion(const std::u16string& chromium_line);

}  // namespace mast

#endif  // MAST_COMMON_MAST_VERSION_H_
