#pragma once

#include <memory>
#include <string>

#include "artifact/Artifact.hh"

using std::shared_ptr;
using std::string;

class Command;
class Reference;
class Version;
class ContentVersion;

class FileArtifact : public Artifact {
 public:
  FileArtifact(Env& env, bool committed,
               shared_ptr<MetadataVersion> mv = make_shared<MetadataVersion>(),
               shared_ptr<ContentVersion> cv = make_shared<ContentVersion>());

  virtual void checkFinalState(const shared_ptr<Reference>& ref) override;

  virtual shared_ptr<Command> getContentCreator() const override {
    return _content_filter.getLastWriter();
  }

  /// Do we have a saved copy of this artifact that can be committed to the filesystem?
  virtual bool isSaved() const override;

  /// Save a copy of this artifact's versions so it can be restored on a future build
  virtual void save(const shared_ptr<Reference>& ref) override;

  /// Have all modifications to this artifact been committed to the filesystem?
  virtual bool isCommitted() const override;

  /// Commit any un-committed version of this artifact using the provided reference
  virtual void commit(const shared_ptr<Reference>& ref) override;

  /// Do we have a fingerprint of this artifact's versions that will allow us to check for a match?
  virtual bool hasFingerprint() const override;

  /// Save a fingerprint of this artifact's versions so we can check for a match
  virtual void fingerprint(const shared_ptr<Reference>& ref) override;

  virtual shared_ptr<ContentVersion> accessContents(const shared_ptr<Command>& c,
                                                    const shared_ptr<Reference>& ref) override;

  virtual shared_ptr<ContentVersion> setContents(
      const shared_ptr<Command>& c, const shared_ptr<Reference>& ref,
      const shared_ptr<ContentVersion>& v = nullptr) override;

  virtual string getTypeName() const override { return "File"; }

 private:
  /// The latest content version
  shared_ptr<ContentVersion> _content_version;

  /// Is the latest content version committed?
  bool _content_committed;

  /// The access filter that controls content interactions
  AccessFilter _content_filter;
};