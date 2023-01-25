/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_media_file_downloader.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/file.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_types.h"
#include "build/build_config.h"
#include "components/download/public/common/download_item_impl.h"
#include "components/download/public/common/download_task_runner.h"
#include "components/download/public/common/in_progress_download_manager.h"
#include "content/browser/blob_storage/blob_registry_wrapper.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/browser/storage_partition_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_request_utils.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "storage/browser/blob/blob_registry_impl.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/blob/blob_url_store_impl.h"
#include "third_party/blink/public/mojom/blob/blob_url_store.mojom.h"
#include "third_party/blink/public/mojom/blob/data_element.mojom.h"
#include "url/gurl.h"

#undef DVLOG
#define DVLOG(V) LOG(ERROR)

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

  if (in_progress_download_manager_) {
    for (auto& download :
         in_progress_download_manager_->TakeInProgressDownloads()) {
      DCHECK(download_item_observation_.IsObservingSource(download.get()));
      download_items_to_be_detached_.push_back(std::move(download));
    }

    while (!download_items_to_be_detached_.empty()) {
      DetachCachedFile(download_items_to_be_detached_.front().get());
    }

    in_progress_download_manager_->ShutDown();
  }
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

void PlaylistMediaFileDownloader::ScheduleToDetachCachedFile(
    download::DownloadItem* item) {
  if (item->media_source.SchemeIsBlob()) {
   // TODO(sko):
  } else {
    for (auto& download :
        in_progress_download_manager_->TakeInProgressDownloads()) {
      DCHECK(download_item_observation_.IsObservingSource(download.get()));
      download_items_to_be_detached_.push_back(std::move(download));
    }
  }
<<<<<<< HEAD

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
=======
  base::SequencedTaskRunnerHandle::Get()->PostTask(
>>>>>>> 2a659dd1b7 (WIP)
      FROM_HERE, base::BindOnce(&PlaylistMediaFileDownloader::DetachCachedFile,
                                weak_factory_.GetWeakPtr(), item));
}

void PlaylistMediaFileDownloader::DetachCachedFile(
    download::DownloadItem* item) {
  // We allow only one item to be downloaded at a time.
  auto iter = base::ranges::find_if(
      download_items_to_be_detached_,
      [item](const auto& download) { return download.get() == item; });
  DCHECK(iter != download_items_to_be_detached_.end());

  // Before removing item from the vector, extend DownloadItems' lifetimes
  // so that it can be deleted after removing the file. i.e. The item should
  // be deleted after the file is released.
  auto will_be_detached = std::move(*iter);
  download_items_to_be_detached_.erase(iter);

  download_item_observation_.RemoveObservation(item);

  if (item->GetLastReason() ==
          download::DownloadInterruptReason::DOWNLOAD_INTERRUPT_REASON_NONE &&
      item->IsDone()) {
    will_be_detached->MarkAsComplete();
  } else {
    will_be_detached->Remove();
  }
}

void PlaylistMediaFileDownloader::CreateInProgressDownloadManagerIfNeeded() {
  if (!in_progress_download_manager_) {
    // Creates our own manager. The arguments below are what's used by
    // AwBrowserContext::RetrieveInProgressDownloadManager().
    auto manager = std::make_unique<download::InProgressDownloadManager>(
        nullptr, base::FilePath(), nullptr,
        /* is_origin_secure_cb, */ base::BindRepeating([](const GURL& origin) {
          return true;
        }),
        base::BindRepeating(&content::DownloadRequestUtils::IsURLSafe),
        /*wake_lock_provider_binder*/ base::NullCallback());
    manager->set_url_loader_factory(url_loader_factory_);
    DCHECK(url_loader_factory_);
    in_progress_download_manager_ = std::move(manager);
    download_manager_observation_.Observe(in_progress_download_manager_.get());
  }
}

void PlaylistMediaFileDownloader::DownloadMediaFileForPlaylistItem(
    const mojom::PlaylistItemPtr& item,
    const base::FilePath& base_dir) {
  DCHECK(!in_progress_);

  ResetDownloadStatus();

  if (item->cached) {
    DVLOG(2) << __func__ << ": media file is already downloaded";
    NotifySucceed(current_item_->id, current_item_->media_path.spec());
    return;
  }

  in_progress_ = true;
  current_item_ = item->Clone();

  if (GURL media_url(current_item_->media_source); media_url.is_valid()) {
    playlist_dir_path_ = base_dir.AppendASCII(current_item_->id);

    if (media_url.SchemeIsBlob()) {
      if (!content_download_manager_) {
        content_download_manager_ = context_->GetDownloadManager();
        content_download_manager_->AddObserver(this);
      }

      DCHECK(content_download_manager_);
    } else {
      CreateInProgressDownloadManagerIfNeeded();
    }

    DCHECK(download::GetIOTaskRunner()) << "This should be set by embedder";

    DownloadMediaFile(media_url);
  } else {
    DVLOG(2) << __func__ << ": media file is empty";
    NotifyFail(current_item_->id);
  }
}

void PlaylistMediaFileDownloader::OnDownloadCreated(
    download::DownloadItem* item) {
  // if (item->GetGuid() != download_item_guid_)
  //   return;

  DVLOG(2) << __func__;
  DCHECK(current_item_) << "This shouldn't happen as we unobserve the manager"
                           "when a process for an item is done";

  DCHECK(!download_item_observation_.IsObservingSource(item));
  download_item_observation_.AddObservation(item);
}

void PlaylistMediaFileDownloader::OnDownloadCreated(
    content::DownloadManager* manager,
    download::DownloadItem* item) {
  DVLOG(2) << __func__ << " " << item->GetURL() << " " << item->GetState()
           << " " << item->CanResume();
  OnDownloadCreated(item);

  if (item->GetState() == download::DownloadItem::INTERRUPTED) {
    LOG(ERROR) << " Download was interrupted as soon as it's been created";
    OnDownloadUpdated(item);
  }
}

void PlaylistMediaFileDownloader::OnDownloadDropped(
    content::DownloadManager* manager) {
  NOTREACHED();
}

void PlaylistMediaFileDownloader::OnDownloadUpdated(
    download::DownloadItem* item) {
  if (!current_item_) {
    // Download could be already finished. This seems to be late async callback.
    return;
  }

  if (item->GetLastReason() !=
      download::DownloadInterruptReason::DOWNLOAD_INTERRUPT_REASON_NONE) {
    LOG(ERROR) << __func__ << ": Download interrupted - reason: "
               << download::DownloadInterruptReasonToString(
                      item->GetLastReason());
    DVLOG(2) << base::debug::StackTrace(20);
    ScheduleToDetachCachedFile(item);
    OnMediaFileDownloaded({});
    return;
  }

  DVLOG(2) << __func__ << " Download progressing";
  base::TimeDelta time_remaining;
  item->TimeRemaining(&time_remaining);
  delegate_->OnMediaFileDownloadProgressed(
      current_item_->id, item->GetTotalBytes(), item->GetReceivedBytes(),
      item->PercentComplete(), time_remaining);

  if (item->IsDone()) {
    DVLOG(2) << __func__ << " Download Done";
    ScheduleToDetachCachedFile(item);
    OnMediaFileDownloaded(playlist_dir_path_.Append(media_file_name_));
    return;
  }
}

void PlaylistMediaFileDownloader::OnDownloadRemoved(
    download::DownloadItem* item) {
  NOTREACHED()
      << "`item` was removed out of this class. This could cause flaky tests";
}

void PlaylistMediaFileDownloader::DownloadMediaFile(const GURL& url) {
  DVLOG(2) << __func__ << ": " << url.spec();

  const base::FilePath file_path = playlist_dir_path_.Append(media_file_name_);
  auto params = std::make_unique<download::DownloadUrlParameters>(
      url, GetNetworkTrafficAnnotationTagForURLLoad());
  params->set_file_path(file_path);
  download_item_guid_ = base::GUID::GenerateRandomV4().AsLowercaseString();
  DCHECK(base::IsValidGUID(download_item_guid_));
  params->set_guid(download_item_guid_);
  params->set_transient(true);
  params->set_require_safety_checks(false);
  params->set_download_source(download::DownloadSource::FROM_RENDERER);

  if (url.SchemeIsBlob()) {
    if (!site_instance_) {
      site_instance_ = content::SiteInstance::Create(context_);
    }
#if token_version || 0
    mojo::PendingRemote<blink::mojom::BlobURLToken> blob_url_token;
    blob_url_token_receiver_ = blob_url_token.InitWithNewPipeAndPassReceiver();

    // blob_url_loader_factory_ =
    //     content::ChromeBlobStorageContext::URLLoaderFactoryForUrl(
    //         context_->GetStoragePartition(site_instance_.get()), url);

    // blob_url_registry->AddReceiver(storage_key, blob_url_store_impl_);

    // blob_url_store_impl_-> void ResolveAsURLLoaderFactory(
    //   const GURL& url,
    //   mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    //   ResolveAsURLLoaderFactoryCallback callback) override;
#else
    /*
      BlobRegistry
      Blob

      BlobUrlRegistry
      BlobURLStore

      BlobStorageContext
      ChromeBlobStorageContext

    */

    // mojo::PendingRemote<blink::mojom::Blob> blob;
    // TODO(sko) Should i create BlobImpl here?
    // blob_receiver_ = blob.InitWithNewPipeAndPassReceiver();

    auto* storage_partition = static_cast<content::StoragePartitionImpl*>(
        context_->GetStoragePartition(site_instance_.get()));
    auto* blob_url_registry = storage_partition->GetBlobUrlRegistry();

    auto storage_key = blink::StorageKey::CreateWithOptionalNonce(
        url::Origin::Create(url), net::SchemefulSite(url), &blob_nonce_,
        blink::mojom::AncestorChainBit::kSameSite);
    blob_store_ = std::make_unique<storage::BlobURLStoreImpl>(
        storage_key, blob_url_registry->AsWeakPtr());

    // auto blob_uuid = base::GenerateGUID();

    // mojo::PendingRemote<blink::mojom::Blob> blob =
    // content::ChromeBlobStorageContext::GetBlobRemote(context_, blob_uuid);
    // storage::BlobImpl::Create(
    //     std::make_unique<storage::BlobDataHandle>(*GetHandleFromBuilder()),
    //     blob_remote.InitWithNewPipeAndPassReceiver());

    auto blob_uuid = base::GenerateGUID();
    mojo::PendingRemote<blink::mojom::Blob> blob;
#if 0
    auto* blob_storage = content::ChromeBlobStorageContext::GetFor(context_);
    content::GetIOThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(
            [blob_uuid](scoped_refptr<content::ChromeBlobStorageContext>
                              chrome_blob_storage_context) {
              // scoped_refptr<content::ChromeBlobStorageContext>
              // chrome_blob_storage_context,
              // mojo::PendingReceiver<blink::mojom::Blob> blob_receiver) {

              auto future_blob =
                  chrome_blob_storage_context->context()->AddFutureBlob(
                      blob_uuid, {}, {}, base::DoNothing());

              // storage::BlobImpl::Create(
              //     std::move(future_blob),
              //     std::move(blob_receiver));
              // }, base::WrapRefCounted(blob_storage),
              // blob.InitWithNewPipeAndPassReceiver()));
            },
            base::WrapRefCounted(blob_storage)));
#endif
    mojo::PendingRemote<blink::mojom::BlobRegistry> remote_registry;
    auto init_blob_registry_on_io_thread = base::BindOnce(
        [](scoped_refptr<content::BlobRegistryWrapper> registry,
           mojo::PendingReceiver<blink::mojom::BlobRegistry>
               registry_receiver) {
          registry->Bind(/* browser_process_id */ 0,
                         std::move(registry_receiver));
          return true;
        },
        base::WrapRefCounted(storage_partition->GetBlobRegistry()),
        remote_registry.InitWithNewPipeAndPassReceiver());

    auto register_blob_registry_on_ui_thread = base::BindOnce(
        [](std::string blob_uuid,
           mojo::PendingRemote<blink::mojom::BlobRegistry> remote_registry, 
           mojo::PendingReceiver<blink::mojom::Blob> blob_receiver,
           bool) {
          mojo::Remote<blink::mojom::BlobRegistry>(std::move(remote_registry)).get()->Register(
              std::move(blob_receiver), blob_uuid, {}, {}, {},
              base::BindOnce([]() { LOG(ERROR) << "Registered"; }));
        },
        blob_uuid, std::move(remote_registry), blob.InitWithNewPipeAndPassReceiver());

    content::GetIOThreadTaskRunner({})->PostTaskAndReplyWithResult(
        FROM_HERE, std::move(init_blob_registry_on_io_thread),
        std::move(register_blob_registry_on_ui_thread));

    // Calling register will call AddUrlMapping below()
    blob_store_->Register(std::move(blob), url, {}, {},
                          base::BindOnce([]() { LOG(ERROR) << "Registered"; }));
    // blob_url_registry->AddUrlMapping(url, std::move(blob), storage_key,
    //                                  blob_nonce_, {});

    // Wrong guess: this is for PartitionedBlobUrl
    // mojo::PendingRemote<blink::mojom::BlobURLStore> storage_remote;
    // blob_url_registry->AddReceiver(storage_key,
    // storage_remote.InitWithNewPipeAndPassReceiver());

    // Can't call this on browser thread. only for IO thread
    // storage::BlobImpl::Create(content::ChromeBlobStorageContext::GetFor(context_)->context()->GetBlobDataFromUUID(blob_nonce_.ToString()),
    // std::move(blob_receiver_));

    // TODO(sko) Should i create BlobImpl here?
    // blob_receiver_ = blob.InitWithNewPipeAndPassReceiver();

    blob_url_loader_factory_ =
        content::ChromeBlobStorageContext::URLLoaderFactoryForUrl(
            context_->GetStoragePartition(site_instance_.get()), url);
#endif

    content_download_manager_->DownloadUrl(std::move(params),
                                           blob_url_loader_factory_);
  } else {
    DCHECK(in_progress_download_manager_->CanDownload(params.get()));
    in_progress_download_manager_->DownloadUrl(std::move(params));
  }
}

void PlaylistMediaFileDownloader::OnMediaFileDownloaded(base::FilePath path) {
  DVLOG(2) << __func__ << ": downloaded media file at " << path;

  DCHECK(current_item_);

  if (path.empty()) {
    // This fail is handled during the generation.
    // See |has_skipped_source_files| in DoGenerateSingleMediaFile().
    // |has_skipped_source_files| will be set to true.
    DVLOG(1) << __func__ << ": failed to download media file: "
             << current_item_->media_source;
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
  current_item_.reset();
  playlist_dir_path_.clear();
}

}  // namespace playlist
