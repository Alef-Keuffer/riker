#pragma once

#include <filesystem>
#include <memory>
#include <ostream>
#include <string>

#include "artifacts/Artifact.hh"
#include "util/serializer.hh"
#include "versions/ContentVersion.hh"

namespace fs = std::filesystem;

/**
 * A DirVersion represents some part of the state of a directory. Subclasses of DirVersion can
 * encode complete versions that describe all of the contents of the directory, or partial versions
 * that describe an update to a single entry in the directory.
 */
class DirVersion : public Version, public std::enable_shared_from_this<DirVersion> {
 public:
  /// Try to cast this version to one of its subtypes
  template <class T>
  std::shared_ptr<T> as() noexcept {
    return std::dynamic_pointer_cast<T>(shared_from_this());
  }

  /// Get the command that created this version
  std::shared_ptr<Command> getCreator() const noexcept { return _creator.lock(); }

  /// Record that this version was created by command c
  void createdBy(std::shared_ptr<Command> c) noexcept { _creator = c; }

  /// Check if this version has been committed
  bool isCommitted() const noexcept { return _committed; }

  /// Mark this version as committed
  void setCommitted(bool committed = true) noexcept { _committed = committed; }

 private:
  // Declare fields for serialization
  SERIALIZE_EMPTY();

  /******** Transient Fields *********/

  /// Has this version been committed?
  bool _committed = false;

  /// The command that created this version
  std::weak_ptr<Command> _creator;
};

/**
 * A BaseDirVersion encodes the starting state for a directory artifact. Every directory has exactly
 * one of these versions. Any updates to the directory are layered on top of the base version.
 */
class BaseDirVersion : public DirVersion {
 public:
  BaseDirVersion(bool created) noexcept : _created(created) { setCommitted(!created); }

  /// Does the base version represent a newly created directory?
  bool getCreated() const noexcept { return _created; }

  /// Commit this base directory version to
  void commit(fs::path path) noexcept;

  /// Get the name for this version type
  virtual std::string getTypeName() const noexcept override {
    if (_created) {
      return "empty dir";
    } else {
      return "on-disk dir";
    }
  }

  /// Print this directory version
  virtual std::ostream& print(std::ostream& o) const noexcept override {
    if (_created) {
      return o << "[dir: empty]";
    } else {
      return o << "[dir: on-disk]";
    }
  }

 private:
  /// Does this version encode the creation of a new directory? If not, accessing entries requires
  /// checks against the actual filesystem.
  bool _created;

  // Create default constructor and declare fields for serialization
  BaseDirVersion() noexcept = default;
  SERIALIZE(BASE(DirVersion), _created);
};

/// An AddEntry version updates a directory with a new entry
class AddEntry : public DirVersion {
 public:
  /// Create a new version of a directory that adds a named entry to the directory
  AddEntry(std::string entry, std::shared_ptr<Artifact> target) noexcept :
      _entry(entry), _target(target) {}

  /// Get the name of the entry this version links
  std::string getEntryName() const noexcept { return _entry; }

  /// Get the target of the newly-linked entry
  std::shared_ptr<Artifact> getTarget() const noexcept { return _target; }

  /// Get the name for this version type
  virtual std::string getTypeName() const noexcept override { return "+" + std::string(_entry); }

  /// Print a link version
  virtual std::ostream& print(std::ostream& o) const noexcept override {
    return o << "[dir: link " << _entry << " -> " << _target << "]";
  }

 private:
  std::string _entry;
  std::shared_ptr<Artifact> _target;
};

/// A RemoveEntry version updates a directory so it no longer has a specific entry
class RemoveEntry : public DirVersion {
 public:
  /// Create a new version of a directory that removes an entry from a directory
  RemoveEntry(std::string entry, std::shared_ptr<Artifact> target) noexcept :
      _entry(entry), _target(target) {}

  /// Get the name of the entry this version removes
  std::string getEntryName() const noexcept { return _entry; }

  /// Get a reference to the artifact that is unlinked
  std::shared_ptr<Artifact> getTarget() const noexcept { return _target; }

  /// Get the name for this version type
  virtual std::string getTypeName() const noexcept override { return "-" + std::string(_entry); }

  /// Print an unlink version
  virtual std::ostream& print(std::ostream& o) const noexcept override {
    return o << "[dir: unlink " << _entry << "]";
  }

 private:
  /// The name of the entry this version removes
  std::string _entry;

  /// A reference to the artifact that is unlinked by this version
  std::shared_ptr<Artifact> _target;
};
