#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "artifacts/Artifact.hh"
#include "runtime/Ref.hh"
#include "versions/DirVersion.hh"

using std::map;
using std::nullopt;
using std::optional;
using std::shared_ptr;
using std::string;

class Command;
class FileVersion;
class Ref;
class Version;

class DirArtifact final : public Artifact {
 public:
  DirArtifact(shared_ptr<MetadataVersion> mv, shared_ptr<BaseDirVersion> dv) noexcept;

  /************ Core Artifact Operations ************/

  /// Get the name of this artifact type
  virtual string getTypeName() const noexcept override { return "Dir"; }

  /// Can a specific version of this artifact be committed?
  virtual bool canCommit(shared_ptr<ContentVersion> v) const noexcept override;

  /// Commit a specific version of this artifact to the filesystem
  virtual void commit(shared_ptr<ContentVersion> v) noexcept override;

  /// Can this artifact be fully committed?
  virtual bool canCommitAll() const noexcept override;

  /// Commit all final versions of this artifact to the filesystem
  virtual void commitAll(optional<fs::path> path = nullopt) noexcept override;

  /// Commit the minimal set of versions required to ensure this artifact exists on the filesystem
  virtual void commitMinimal(fs::path path) noexcept override;

  /// Compare all final versions of this artifact to the filesystem state
  virtual void checkFinalState(fs::path path) noexcept override;

  /// Commit any pending versions and save fingerprints for this artifact
  virtual void applyFinalState(fs::path path) noexcept override;

  /************ Traced Operations ************/

  /// A traced command is about to (possibly) read from this artifact
  virtual void beforeRead(Build& build,
                          const shared_ptr<Command>& c,
                          Ref::ID ref) noexcept override;

  /// A traced command just read from this artifact
  virtual void afterRead(Build& build, const shared_ptr<Command>& c, Ref::ID ref) noexcept override;

  /************ Content Operations ************/

  /// Get this artifact's current content without creating any dependencies
  virtual shared_ptr<ContentVersion> peekContent() noexcept override;

  /// Check to see if this artifact's content matches a known version
  virtual void matchContent(const shared_ptr<Command>& c,
                            Scenario scenario,
                            shared_ptr<ContentVersion> expected) noexcept override;

  /// Get a version that lists all the entries in this directory
  shared_ptr<DirListVersion> getList(const shared_ptr<Command>& c) noexcept;

  /************ Directory Operations ************/

  /// Add a directory entry to this artifact
  virtual shared_ptr<DirVersion> addEntry(const shared_ptr<Command>& c,
                                          fs::path entry,
                                          shared_ptr<Artifact> target) noexcept override;

  /// Remove a directory entry from this artifact
  virtual shared_ptr<DirVersion> removeEntry(const shared_ptr<Command>& c,
                                             fs::path entry,
                                             shared_ptr<Artifact> target) noexcept override;

  // Un-hide the shorthand version of resolve()
  using Artifact::resolve;

  virtual Ref resolve(const shared_ptr<Command>& c,
                      shared_ptr<Artifact> prev,
                      fs::path::iterator current,
                      fs::path::iterator end,
                      AccessFlags flags,
                      size_t symlink_limit) noexcept override;

 private:
  /// The base directory version is the backstop for all resolution queries. This is either an
  /// on-disk verison, or an empty directory
  shared_ptr<BaseDirVersion> _base_dir_version;

  /// A map of entries in this directory. Each mapped value is a pair of the version that created
  /// that entry and the artifact that entry resolves to (or nullptr if the entry is absent).
  map<string, tuple<shared_ptr<DirVersion>, shared_ptr<Artifact>>> _entries;
};