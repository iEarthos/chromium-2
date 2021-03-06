// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/file_system/touch_operation.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/drive/file_errors.h"
#include "chrome/browser/chromeos/drive/file_system/operation_observer.h"
#include "chrome/browser/chromeos/drive/resource_metadata.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace drive {
namespace file_system {

namespace {

// Updates the timestamps of the entry specified by |file_path|.
FileError UpdateLocalState(internal::ResourceMetadata* metadata,
                           const base::FilePath& file_path,
                           const base::Time& last_access_time,
                           const base::Time& last_modified_time,
                       std::string* local_id) {
  ResourceEntry entry;
  FileError error = metadata->GetResourceEntryByPath(file_path, &entry);
  if (error != FILE_ERROR_OK)
    return error;
  *local_id = entry.local_id();

  PlatformFileInfoProto* file_info = entry.mutable_file_info();
  if (!last_access_time.is_null())
    file_info->set_last_accessed(last_access_time.ToInternalValue());
  if (!last_modified_time.is_null())
    file_info->set_last_modified(last_modified_time.ToInternalValue());
  entry.set_metadata_edit_state(ResourceEntry::DIRTY);
  return metadata->RefreshEntry(entry);
}

}  // namespace

TouchOperation::TouchOperation(base::SequencedTaskRunner* blocking_task_runner,
                               OperationObserver* observer,
                               internal::ResourceMetadata* metadata)
    : blocking_task_runner_(blocking_task_runner),
      observer_(observer),
      metadata_(metadata),
      weak_ptr_factory_(this) {
}

TouchOperation::~TouchOperation() {
}

void TouchOperation::TouchFile(const base::FilePath& file_path,
                               const base::Time& last_access_time,
                               const base::Time& last_modified_time,
                               const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  std::string* local_id = new std::string;
  base::PostTaskAndReplyWithResult(
      blocking_task_runner_.get(),
      FROM_HERE,
      base::Bind(&UpdateLocalState,
                 metadata_,
                 file_path,
                 last_access_time,
                 last_modified_time,
                 local_id),
      base::Bind(&TouchOperation::TouchFileAfterUpdateLocalState,
                 weak_ptr_factory_.GetWeakPtr(),
                 file_path,
                 callback,
                 base::Owned(local_id)));
}

void TouchOperation::TouchFileAfterUpdateLocalState(
    const base::FilePath& file_path,
    const FileOperationCallback& callback,
    const std::string* local_id,
    FileError error) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  if (error == FILE_ERROR_OK) {
    observer_->OnDirectoryChangedByOperation(file_path.DirName());
    observer_->OnEntryUpdatedByOperation(*local_id);
  }
  callback.Run(error);
}

}  // namespace file_system
}  // namespace drive
