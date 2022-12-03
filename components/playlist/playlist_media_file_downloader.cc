/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_media_file_downloader.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/task_runner_util.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_types.h"
#include "build/build_config.h"
#include "components/download/public/common/in_progress_download_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_request_utils.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace playlist {
namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTagForURLLoad() {
  return net::DefineNetworkTrafficAnnotation("playlist_service", R"(
      semantics {
        sender: "Brave Playlist Service"
        description:
          "Fetching media file for newly created playlist"
        trigger:
          "User-initiated for creating new playlist "
        data:
          "media file for playlist"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
      })");
}

}  // namespace

PlaylistMediaFileDownloader::PlaylistMediaFileDownloader(
    Delegate* delegate,
    content::BrowserContext* context,
    base::FilePath::StringType media_file_name)
    : delegate_(delegate),
      context_(context),
      url_loader_factory_(
          context->content::BrowserContext::GetDefaultStoragePartition()
              ->GetURLLoaderFactoryForBrowserProcess()),
      media_file_name_(media_file_name) {}

PlaylistMediaFileDownloader::~PlaylistMediaFileDownloader() {
  ResetDownloadStatus();
}

void PlaylistMediaFileDownloader::NotifyFail(const std::string& id) {
  CHECK(!id.empty());
  delegate_->OnMediaFileGenerationFailed(id);
  ResetDownloadStatus();
}

void PlaylistMediaFileDownloader::NotifySucceed(
    const std::string& id,
    const std::string& media_file_path) {
  DCHECK(!id.empty());
  DCHECK(!media_file_path.empty());
  delegate_->OnMediaFileReady(id, media_file_path);
  ResetDownloadStatus();
}

void PlaylistMediaFileDownloader::DownloadMediaFileForPlaylistItem(
    const PlaylistItemInfo& item,
    const base::FilePath& base_dir) {
  DCHECK(!in_progress_);

  ResetDownloadStatus();

  if (item.media_file_cached) {
    VLOG(2) << __func__ << ": media file is already downloaded";
    NotifySucceed(current_item_->id, current_item_->media_file_path);
    return;
  }

  in_progress_ = true;
  current_item_ = std::make_unique<PlaylistItemInfo>(item);
  if (!download_manager_) {
#if BUILDFLAG(IS_ANDROID)
    // ProfileAndroid doesn't provide InProgressDownloadManager. We should
    // create our own manager. The arguments below are what's used by
    // AwBrowserContext::RetriveInProgressDownloadManager().
    auto manager = std::make_unique<download::InProgressDownloadManager>(
        nullptr, base::FilePath(), nullptr,
        /* is_origin_secure_cb, */ base::BindRepeating([](const GURL& origin) {
          return true;
        }),
        base::BindRepeating(&content::DownloadRequestUtils::IsURLSafe),
        /*wake_lock_provider_binder*/ base::NullCallback());
    manager->set_url_loader_factory(url_loader_factory_);
    download_manager_ = std::move(manager);
#else
    download_manager_ = context_->RetriveInProgressDownloadManager();
#endif
  }

  DCHECK(download_manager_) << "DownloadManager isn't available";
  download_manager_observation_.Observe(download_manager_.get());

  if (GURL media_url(current_item_->media_src); media_url.is_valid()) {
    playlist_dir_path_ = base_dir.AppendASCII(current_item_->id);
    DownloadMediaFile(media_url);
  } else {
    VLOG(2) << __func__ << ": media file is empty";
    NotifyFail(current_item_->id);
  }
}

void PlaylistMediaFileDownloader::OnDownloadCreated(
    download::DownloadItem* item) {
  LOG(ERROR) << __FUNCTION__;
  DCHECK(current_item_) << "This shouldn't happen as we unobserve the manager "
                           "when a process for an item is done";
#if BUILDFLAG(IS_ANDROID)
  DCHECK_EQ(item->GetGuid(), current_item_->id);
#else
  if (item->GetGuid() != current_item_->id) {
    // As download manager could shared by other components, we might get
    // other download items triggered from them.
    return;
  }
#endif

  DCHECK(!download_item_observation_.IsObserving());
  download_item_observation_.Observe(item);
}

void PlaylistMediaFileDownloader::OnDownloadUpdated(
    download::DownloadItem* item) {
  if (!current_item_) {
    // Download could be already finished. This seems to be late async callback.
    return;
  }

  if (item->GetLastReason() !=
      download::DownloadInterruptReason::DOWNLOAD_INTERRUPT_REASON_NONE) {
    VLOG(2) << __func__ << ": Download interrupted";
    base::SequencedTaskRunnerHandle::Get()->PostTask(FROM_HERE, 
        base::BindOnce(&download::DownloadItem::Remove, base::Unretained(item)));
    OnMediaFileDownloaded({});
    return;
  }

  VLOG(2) << __func__ << ": Download Progressed";
  base::TimeDelta time_remaining;
  item->TimeRemaining(&time_remaining);
  delegate_->OnMediaFileDownloadProgressed(
      current_item_->id, item->GetTotalBytes(), item->GetReceivedBytes(),
      item->PercentComplete(), time_remaining);

  if (item->AllDataSaved()) {
    VLOG(2) << __func__ << ": Download done";
    base::SequencedTaskRunnerHandle::Get()->PostTask(FROM_HERE, 
        base::BindOnce(&download::DownloadItem::Remove, base::Unretained(item)));
    OnMediaFileDownloaded(playlist_dir_path_.Append(media_file_name_));
    return;
  }
}

void PlaylistMediaFileDownloader::DownloadMediaFile(const GURL& url) {
  VLOG(2) << __func__ << ": " << url.spec();

  const base::FilePath file_path = playlist_dir_path_.Append(media_file_name_);
  auto params = std::make_unique<download::DownloadUrlParameters>(
      url, GetNetworkTrafficAnnotationTagForURLLoad());
  params->set_file_path(file_path);
  params->set_guid(current_item_->id);
  params->set_transient(true);
  params->set_require_safety_checks(false);
  DCHECK(download_manager_->CanDownload(params.get()));
  download_manager_->DownloadUrl(std::move(params));
}

void PlaylistMediaFileDownloader::OnMediaFileDownloaded(base::FilePath path) {
  VLOG(2) << __func__ << ": downloaded media file at " << path;

  DCHECK(current_item_);

  if (path.empty()) {
    // This fail is handled during the generation.
    // See |has_skipped_source_files| in DoGenerateSingleMediaFile().
    // |has_skipped_source_files| will be set to true.
    VLOG(1) << __func__ << ": failed to download media file: " << current_item_;
    NotifyFail(current_item_->id);
    return;
  }

  NotifySucceed(current_item_->id, path.AsUTF8Unsafe());
}

void PlaylistMediaFileDownloader::RequestCancelCurrentPlaylistGeneration() {
  ResetDownloadStatus();
}

base::SequencedTaskRunner* PlaylistMediaFileDownloader::task_runner() {
  if (!task_runner_) {
    task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return task_runner_.get();
}

void PlaylistMediaFileDownloader::ResetDownloadStatus() {
  in_progress_ = false;
  download_manager_observation_.Reset();
  download_item_observation_.Reset();
  current_item_.reset();
  playlist_dir_path_.clear();
}

}  // namespace playlist
