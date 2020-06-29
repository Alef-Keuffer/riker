#include "FileVersion.hh"

#include <memory>
#include <optional>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "core/IR.hh"
#include "util/log.hh"

using std::nullopt;
using std::shared_ptr;

// Is this version saved in a way that can be committed?
bool FileVersion::canCommit() const noexcept {
  if (isCommitted()) return true;
  return _fingerprint.has_value() && _fingerprint.value().empty;
}

// Commit this version to the filesystem
void FileVersion::commit(fs::path path) noexcept {
  if (isCommitted()) return;

  ASSERT(canCommit()) << "Attempted to commit unsaved version " << this << " to " << path;

  int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  FAIL_IF(fd < 0) << "Failed to commit empty file version: " << ERR;
  close(fd);

  // Mark this version as committed
  Version::setCommitted();
}

// Save a fingerprint of this version
void FileVersion::fingerprint(fs::path path) noexcept {
  if (hasFingerprint()) return;

  // Get stat data and save it
  struct stat statbuf;
  int rc = ::lstat(path.c_str(), &statbuf);
  WARN_IF(rc) << "Failed to stat " << path;

  if (rc == 0) _fingerprint = statbuf;
}