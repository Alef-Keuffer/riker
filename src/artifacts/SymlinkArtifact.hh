#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include "artifacts/Artifact.hh"

using std::map;
using std::shared_ptr;
using std::string;

namespace fs = std::filesystem;

class RefResult;

class SymlinkArtifact : public Artifact {
 public:
  SymlinkArtifact(shared_ptr<Env> env,
                  shared_ptr<MetadataVersion> mv,
                  shared_ptr<SymlinkVersion> sv) noexcept;

  /************ Core Artifact Operations ************/

  /// Get a printable name for this artifact type
  virtual string getTypeName() const noexcept override { return "Symlink"; }

  /// Can a specific version of this artifact be committed?
  virtual bool canCommit(shared_ptr<Version> v) const noexcept override;

  /// Commit a specific version of this artifact to the filesystem
  virtual void commit(shared_ptr<Version> v) noexcept override;

  /// Can this artifact be fully committed?
  virtual bool canCommitAll() const noexcept override;

  /// Commit all final versions of this artifact to the filesystem
  virtual void commitAll() noexcept override;

  /// Command c requires that this artifact exists in its current state. Create dependency edges.
  virtual void mustExist(Build& build, shared_ptr<Command> c) noexcept override;

  /// Compare all final versions of this artifact to the filesystem state
  virtual void checkFinalState(Build& build, fs::path path) noexcept override;

  /// Commit any pending versions and save fingerprints for this artifact
  virtual void applyFinalState(Build& build, fs::path path) noexcept override;

  /************ Content Operations ************/

  /// A traced command is about to (possibly) read from this artifact
  virtual void beforeRead(Build& build,
                          shared_ptr<Command> c,
                          shared_ptr<RefResult> ref) noexcept override;

  /// A traced command just read from this artifact
  virtual void afterRead(Build& build,
                         shared_ptr<Command> c,
                         shared_ptr<RefResult> ref) noexcept override;

  /// A traced command is about to (possibly) write to this artifact
  virtual void beforeWrite(Build& build,
                           shared_ptr<Command> c,
                           shared_ptr<RefResult> ref) noexcept override;

  /// A trace command just wrote to this artifact
  virtual void afterWrite(Build& build,
                          shared_ptr<Command> c,
                          shared_ptr<RefResult> ref) noexcept override;

  /// Check to see if this artifact's content matches a known version
  virtual void matchContent(Build& build,
                            shared_ptr<Command> c,
                            shared_ptr<Version> expected) noexcept override;

  /************ Symlink Operations ************/

  virtual Resolution resolve(Build& build,
                             shared_ptr<Command> c,
                             shared_ptr<Artifact> prev,
                             fs::path::iterator current,
                             fs::path::iterator end,
                             AccessFlags flags,
                             size_t symlink_limit) noexcept override;

 private:
  /// The currrent version of this symlink
  shared_ptr<SymlinkVersion> _symlink_version;
};
