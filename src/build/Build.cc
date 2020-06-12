#include "Build.hh"

#include <iostream>
#include <memory>
#include <ostream>

#include "artifacts/Artifact.hh"
#include "core/IR.hh"
#include "tracing/Tracer.hh"
#include "ui/options.hh"

using std::cout;
using std::endl;
using std::ostream;
using std::shared_ptr;

void Build::run() noexcept {
  // This is a hack to resolve the stdin, stdout, and stderr pipes before starting emulation.
  for (const auto& [index, info] : _root->getInitialFDs()) {
    info.getReference()->emulate(_root, *this);
    if (index == 0) info.getReference()->getArtifact()->setName("stdin");
    if (index == 1) info.getReference()->getArtifact()->setName("stdout");
    if (index == 2) info.getReference()->getArtifact()->setName("stderr");
  }

  // Resolve the root directory, working directory, and executable references
  _root->getInitialRootDir()->resolve(_root, *this);
  _root->getInitialWorkingDir()->resolve(_root, *this);
  _root->getExecutable()->resolve(_root, *this);

  // Inform observers of the launch action
  for (const auto& o : _observers) {
    o->launchRootCommand(_root);
  }

  // Start the build by running the root command
  runCommand(_root);

  // Ask the environment to check remaining artifacts for changes, and to save metadata and
  // fingerprints for artifacts that were created during the build
  _env.finalize();
}

// Called when an emulated command launches another command
void Build::launch(shared_ptr<Command> parent, shared_ptr<Command> child) noexcept {
  // Inform observers of the launch action
  for (const auto& o : _observers) {
    o->launchChildCommand(parent, child);
  }

  // Now actually run the command
  runCommand(child);
}

void Build::runCommand(shared_ptr<Command> c) noexcept {
  if (checkRerun(c)) {
    // We are rerunning this command, so clear the lists of steps and children
    c->reset();

    // Show the command if printing is on, or if this is a dry run
    if (options::print_on_run || options::dry_run) cout << c->getFullName() << endl;

    // Actually run the command, unless this is a dry run
    if (!options::dry_run) {
      // Set up a tracing context and run the command
      Tracer(*this).run(c);
    }

  } else {
    c->emulate(*this);
  }
}

ostream& Build::print(ostream& o) const noexcept {
  if (_rerun.size() > 0) {
    o << "The following commands will be rerun:" << endl;
    for (const auto& c : _rerun) {
      o << "  " << c << endl;
    }

  } else {
    o << "No commands to rerun" << endl;
  }

  return o;
}
