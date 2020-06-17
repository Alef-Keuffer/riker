#include "ContentVersion.hh"

#include <memory>
#include <optional>

#include <unistd.h>

#include "core/IR.hh"
#include "util/log.hh"

using std::dynamic_pointer_cast;
using std::nullopt;
using std::shared_ptr;

// Is this version saved in a way that can be committed?
bool ContentVersion::isSaved() const noexcept {
  return _fingerprint.has_value() && _fingerprint.value().empty;
}

// Commit this version to the filesystem
void ContentVersion::commit(shared_ptr<Reference> ref) const noexcept {
  ASSERT(isSaved()) << "Attempted to commit unsaved version";

  if (auto a = dynamic_pointer_cast<Access>(ref)) {
    if (_fingerprint.has_value() && _fingerprint.value().empty) {
      int fd = a->open();
      FAIL_IF(fd < 0) << "Failed to commit empty file version: " << ERR;
      close(fd);
    }
  }
}

// Save a fingerprint of this version
void ContentVersion::fingerprint(shared_ptr<Reference> ref) noexcept {
  // Check the reference type
  if (auto a = dynamic_pointer_cast<Access>(ref)) {
    // Get stat data and save it
    auto [info, rc] = a->lstat();
    if (rc == SUCCESS) _fingerprint = info;
  }
}

// Compare this version to another version
bool ContentVersion::matches(shared_ptr<Version> other) const noexcept {
  // A version compares equal to itself, even if we have no saved metadata
  if (other.get() == this) return true;

  // If we have no saved fingerprint, we cannot find a match
  if (!_fingerprint.has_value()) return false;

  // Make sure the other version is a ContentVersion
  auto v = dynamic_pointer_cast<ContentVersion>(other);
  if (!v) return false;

  // Compare. If the other version does not have a fingerprint, optional will compare false
  if (_fingerprint == v->_fingerprint) {
    identify(v);
    return true;
  } else {
    return false;
  }
}