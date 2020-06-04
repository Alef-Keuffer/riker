#pragma once

#include <memory>
#include <ostream>
#include <set>

#include "build/BuildObserver.hh"
#include "data/Command.hh"
#include "data/Version.hh"

using std::endl;
using std::ostream;
using std::set;
using std::shared_ptr;

/**
 * An instance of this class is used to gather statistics as it traverses a build.
 * Usage:
 */
class Stats : public BuildObserver {
 public:
  /**
   * Gather statistics from a build as it runs
   * \param list_artifacts  If true, include a list of artifacts and versions in the final output
   */
  Stats(bool list_artifacts) : _list_artifacts(list_artifacts) {}

  /// Print the results of our stats gathering
  ostream& print(ostream& o) {
    // Total versions for all artifacts
    size_t version_count = 0;
    for (auto& a : _artifacts) {
      version_count += a->getVersionCount();
    }

    o << "Build Statistics:" << endl;
    o << "  Commands: " << _command_count << endl;
    o << "  Steps: " << _step_count << endl;
    o << "  Artifacts: " << _artifacts.size() << endl;
    o << "  Artifact Versions: " << version_count << endl;

    if (_list_artifacts) {
      o << endl;
      o << "Artifacts:" << endl;
      for (auto& a : _artifacts) {
        o << "  " << a->getName() << endl;

        size_t index = 0;
        for (auto& v : a->getVersions()) {
          bool metadata = v->hasMetadata();
          bool fingerprint = v->hasFingerprint();
          bool saved = v->isSaved();

          o << "    v" << index << ":";
          if (metadata) o << " metadata";
          if (fingerprint) o << " fingerprint";
          if (saved) o << " saved";
          if (!metadata && !fingerprint && !saved) o << " no data";
          o << endl;

          index++;
        }
      }
    }

    return o;
  }

  /// Print a Stats reference
  friend ostream& operator<<(ostream& o, Stats& s) { return s.print(o); }

  /// Print a Stats pointer
  friend ostream& operator<<(ostream& o, Stats* s) { return s->print(o); }

 private:
  /// Gather stats for a command
  void processCommand(shared_ptr<Command> c) {
    // Count this command and its steps
    _command_count++;
    _step_count += c->getSteps().size();
  }

  /// Called during emulation to report an output from command c
  virtual void metadataOutput(shared_ptr<Command> c, shared_ptr<Artifact> a,
                              shared_ptr<Version> v) override {
    _artifacts.insert(a);
  }

  /// Called during emulation to report an output from command c
  virtual void contentOutput(shared_ptr<Command> c, shared_ptr<Artifact> a,
                             shared_ptr<Version> v) override {
    _artifacts.insert(a);
  }

  /// Called during emulation to report an input to command c
  virtual void metadataInput(shared_ptr<Command> c, shared_ptr<Artifact> a,
                             shared_ptr<Version> v) override {
    _artifacts.insert(a);
  }

  /// Called during emulation to report an input to command c
  virtual void contentInput(shared_ptr<Command> c, shared_ptr<Artifact> a,
                            shared_ptr<Version> v) override {
    _artifacts.insert(a);
  }

  virtual void launchRootCommand(shared_ptr<Command> root) override { processCommand(root); }

  /// Called each time a command emulates a launch step
  virtual void launchChildCommand(shared_ptr<Command> parent, shared_ptr<Command> child) override {
    // Process the child command
    processCommand(child);
  }

 private:
  /// Should the stats include a list of artifacts?
  bool _list_artifacts;

  /// The total number of commands in the build trace
  size_t _command_count = 0;

  /// The total number of IR steps in the build trace
  size_t _step_count = 0;

  /// A set of artifacts visited during the stats collection
  set<shared_ptr<Artifact>> _artifacts;
};