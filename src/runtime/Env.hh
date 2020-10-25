#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <sys/types.h>

#include "data/AccessFlags.hh"

using std::map;
using std::set;
using std::shared_ptr;
using std::string;

namespace fs = std::filesystem;

class Artifact;
class Build;
class Command;
class DirArtifact;
class PathRef;
class PipeArtifact;
class Ref;
class SymlinkArtifact;

/**
 * An Env instance represents the environment where a build process executes. This captures all of
 * the files, directories, and pipes that the build process interacts with. The primary job of the
 * Env is to produce artifacts to model each of these entities in response to accesses from traced
 * or emulated commands.
 */
class Env : public std::enable_shared_from_this<Env> {
 public:
  /**
   * Create an environment for build emulation or execution.
   */
  Env() noexcept = default;

  // Disallow Copy
  Env(const Env&) = delete;
  Env& operator=(const Env&) = delete;

  /// Get the standard input pipe
  shared_ptr<PipeArtifact> getStdin(Build& build, const shared_ptr<Command>& c) noexcept;

  /// Get the standard output pipe
  shared_ptr<PipeArtifact> getStdout(Build& build, const shared_ptr<Command>& c) noexcept;

  /// Get the standard error pipe
  shared_ptr<PipeArtifact> getStderr(Build& build, const shared_ptr<Command>& c) noexcept;

  /// Get the root directory
  shared_ptr<DirArtifact> getRootDir() noexcept;

  /// Get a unique path to a temporary file in the build directory
  fs::path getTempPath() noexcept;

  /// Get a set of all the artifacts in the build
  const set<shared_ptr<Artifact>>& getArtifacts() const noexcept { return _artifacts; }

  /**
   * Get an artifact to represent a statted file/dir/pipe/symlink.
   * If an artifact with the same inode and device number already exists, return that same instance.
   * \param path  The path to this artifact on the filesystem
   * \returns an artifact pointer
   */
  shared_ptr<Artifact> getFilesystemArtifact(fs::path path);

  /**
   * Create a pipe artifact
   * \param c The command that creates the pipe
   * \returns a pipe artifact
   */
  shared_ptr<PipeArtifact> getPipe(Build& build, const shared_ptr<Command>& c) noexcept;

  /**
   * Create a symlink artifact
   * \param c         The command creating the symlink
   * \param target    The destination of the symlink
   * \param committed If true, the symlink is already committed
   * \returns a symlink artifact
   */
  shared_ptr<SymlinkArtifact> getSymlink(Build& build,
                                         const shared_ptr<Command>& c,
                                         fs::path target,
                                         bool committed) noexcept;

  /**
   * Create a directory artifact
   * \param c         The command that is creating the directory
   * \param mode      The mode (permissions) specified when creating the directory
   * \param committed If true, the directory is already committed
   * \returns a directory artifact
   */
  shared_ptr<DirArtifact> getDir(Build& build,
                                 const shared_ptr<Command>& c,
                                 mode_t mode,
                                 bool committed) noexcept;

  /**
   * Create a file artifact that exists only in the filesystem model
   * \param path    The path where this file will eventually appear
   * \param creator The command that creates this file
   * \returns a file artifact
   */
  shared_ptr<Artifact> createFile(Build& build,
                                  const shared_ptr<Command>& creator,
                                  mode_t mode,
                                  bool committed) noexcept;

 private:
  /// The next unique ID for a temporary file
  size_t _next_temp_id = 0;

  // Special artifacts
  shared_ptr<PipeArtifact> _stdin;    //< Standard input
  shared_ptr<PipeArtifact> _stdout;   //< Standard output
  shared_ptr<PipeArtifact> _stderr;   //< Standard error
  shared_ptr<DirArtifact> _root_dir;  //< The root directory

  /// A set of all the artifacts used during the build
  set<shared_ptr<Artifact>> _artifacts;

  /// A map of artifacts identified by inode
  map<pair<dev_t, ino_t>, shared_ptr<Artifact>> _inodes;
};
