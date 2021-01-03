#include "SymlinkArtifact.hh"

#include <memory>

#include "artifacts/DirArtifact.hh"
#include "runtime/Build.hh"
#include "versions/MetadataVersion.hh"
#include "versions/SymlinkVersion.hh"

using std::shared_ptr;

SymlinkArtifact::SymlinkArtifact(shared_ptr<Env> env,
                                 shared_ptr<MetadataVersion> mv,
                                 shared_ptr<SymlinkVersion> sv) noexcept :
    Artifact(env, mv) {
  appendVersion(sv);
  _symlink_version = sv;
}

/// A traced command is about to (possibly) read from this artifact
void SymlinkArtifact::beforeRead(Build& build, const shared_ptr<Command>& c, Ref::ID ref) noexcept {
  // Do nothing before a read
}

/// A traced command just read from this artifact
void SymlinkArtifact::afterRead(Build& build, const shared_ptr<Command>& c, Ref::ID ref) noexcept {
  // The command now depends on the content of this file
  build.traceMatchContent(c, ref, _symlink_version);
}

// Get this artifact's content without creating dependencies
shared_ptr<ContentVersion> SymlinkArtifact::peekContent() noexcept {
  return _symlink_version;
}

/// Check to see if this artifact's content matches a known version
void SymlinkArtifact::matchContent(const shared_ptr<Command>& c,
                                   Scenario scenario,
                                   shared_ptr<ContentVersion> expected) noexcept {
  // The symlink version is an input to command c
  c->currentRun()->addInput(shared_from_this(), _symlink_version, InputType::Accessed);

  // Compare the symlink version to the expected version
  if (!_symlink_version->matches(expected)) {
    LOGF(artifact, "Content mismatch in {} ({} scenario {}): \n  expected {}\n  observed {}", this,
         c, scenario, expected, _symlink_version);

    // Report the mismatch
    c->currentRun()->inputChanged(shared_from_this(), _symlink_version, expected, scenario);
  }
}

bool SymlinkArtifact::canCommit(shared_ptr<Version> v) const noexcept {
  if (v == _symlink_version) {
    return true;
  } else if (v == _metadata_version) {
    return _metadata_version->canCommit();
  } else {
    FAIL << "Attempted to check committable state for unknown version " << v << " in " << this;
    return false;
  }
}

void SymlinkArtifact::commit(shared_ptr<Version> v) noexcept {
  // Get a committed path to this artifact, possibly by committing links above it in the path
  auto path = commitPath();
  ASSERT(path.has_value()) << "Symlink has no path";

  if (v == _symlink_version) {
    _symlink_version->commit(path.value());
  } else if (v == _metadata_version) {
    _metadata_version->commit(path.value());
  } else {
    FAIL << "Attempted to commit unknown version " << v << " in " << this;
  }
}

bool SymlinkArtifact::canCommitAll() const noexcept {
  // Symlink versions are always committable, so just check the metadata version
  return _metadata_version->canCommit();
}

// Commit all final versions of this artifact to the filesystem
void SymlinkArtifact::commitAll() noexcept {
  // Get a committed path to this artifact, possibly by committing links above it in the path
  auto path = commitPath();
  ASSERT(path.has_value()) << "Symlink has no path";

  _symlink_version->commit(path.value());

  // Don't commit symlink metadata for now. We can eventually commit ownership, but not permissions.
  //_metadata_version->commit(path.value());
}

// Compare all final versions of this artifact to the filesystem stateq
void SymlinkArtifact::checkFinalState(fs::path path) noexcept {
  if (!_symlink_version->isCommitted()) {
    // TODO: Compare to on-disk symlink state here
  }

  // Check the metadata state as well
  Artifact::checkFinalState(path);
}

// Commit any pending versions and save fingerprints for this artifact
void SymlinkArtifact::applyFinalState(fs::path path) noexcept {
  // Symlinks are always saved, so no need to fingerprint

  // Make sure this symlink is committed
  _symlink_version->commit(path);

  // Commit and fingerprint metadata
  Artifact::applyFinalState(path);
}

Ref SymlinkArtifact::resolve(const shared_ptr<Command>& c,
                             shared_ptr<Artifact> prev,
                             fs::path::iterator current,
                             fs::path::iterator end,
                             AccessFlags flags,
                             size_t symlink_limit) noexcept {
  if (symlink_limit == 0) return ELOOP;

  // If this is the end of the path and the nofollow flag is set, return this symlink
  if (current == end && flags.nofollow) {
    // Did the access expect to get a symlink?
    if (flags.type == AccessType::Dir) {
      return ENOTDIR;
    } else if (flags.type == AccessType::File) {
      return ELOOP;
    } else {
      return Ref(flags, shared_from_this());
    }
  }

  // Otherwise we follow the symlink. That creates a path resolution dependency on our version
  c->currentRun()->addInput(shared_from_this(), _symlink_version, InputType::PathResolution);

  // Get the symlink destination
  auto dest = _symlink_version->getDestination();

  // Append remaining path entries to the destination
  while (current != end) {
    dest /= *current++;
  }

  // Is the destination relative or absolute?
  if (dest.is_relative()) {
    // Resolve relative to the previous artifact, which must be the dir that holds this symlink
    return prev->resolve(c, dest, flags, symlink_limit - 1);

  } else {
    // Strip the leading slash from the path
    dest = dest.relative_path();

    // Resolve relative to root. First strip the leading slash off the path
    return getEnv()->getRootDir()->resolve(c, dest, flags, symlink_limit - 1);
  }
}
