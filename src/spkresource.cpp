
#include <QDir>
#include "page/spkpagebase.h"
#include "spkutils.h"
#include "spkresource.h"

SpkResource* SpkResource::Instance = nullptr;

// If you want to iterate all keys in a QMap, there's no good way to do that, nor does Clazy have.
// Clazy here is just a piece of shit nagging you like crazy and never gives you the actual
// solution.
// clazy:excludeall=container-anti-pattern

SpkResource::SpkResource(QObject *parent) : QObject(parent),
  mMaximumConcurrent(CFG->ReadField("resource/concurrent", 5).toInt()),
  mCacheDirectory(CFG->ReadField("dirs/cache", "*/.cache/spark-store/res/")
    .toString()
    .replace('*', QDir::homePath()))
{
  Q_ASSERT(!Instance);
  qRegisterMetaType<ResourceResult>();
  Instance = this;

  mRequestSemaphore = new QSemaphore(mMaximumConcurrent);

  QString path = mCacheDirectory.section('/', 1, -2, QString::SectionIncludeLeadingSep);
  if(!QDir().exists(path))
  {
    if(!QDir().mkpath(path))
    {
      sErr(tr("Cache directory \"%1\" cannot be created.").arg(path));
      return;
    }
    else
      sLog(tr("Created cache directory \"%1\".").arg(path));
  }
}

ResourceResult
SpkResource::RequestResource(const int aId, const QString &aPkgName, ResourceType aType,
                             const QString &aPath, const QVariant &aInfo)
{
  auto ret = LocateCachedResource(ResourceTask
                                  { .pkgName = aPkgName, .path = aPath, .type = aType,
                                    .info = aInfo, .id = aId });

  if(ret.status == ResourceStatus::Ready)
    return ret;

  mAwaitingRequests.enqueue(ResourceTask
                            { .pkgName = aPkgName, .path = aPath, .type = aType, .info = aInfo,
                              .id = aId });
  TryBeginAwaitingTasks();

  return ResourceResult { .status = ResourceStatus::Deferred, .data = QByteArray() };
}

void SpkResource::ResourceDownloaded()
{
  auto reply = qobject_cast<QNetworkReply*>(sender());
  auto id = mWorkingRequests.value(reply);
  mWorkingRequests.remove(reply); // Remove in time
  mRequestSemaphore->release(); // Release semaphore resource

  ResourceResult ret;
  ret.status = ResourceStatus::Ready;

  if(reply->error() != QNetworkReply::NoError)
  {
    ret.status = ResourceStatus::Failed;
    sWarn(tr("SpkResource: %1 cannot be downloaded, error code %2")
             .arg(reply->url().toString())
             .arg(reply->error()));
    // Tell ResourceContext
    if(!reply->property("outdated").toBool())
      emit AcquisitionFinish(id, ret);
  }

  ret.data = reply->readAll();

  // Save cache to disk
  auto cacheFile = reply->property("dest_file").toString();
  QFile writeCache(cacheFile);
  QString path = cacheFile.section('/', 1, -2, QString::SectionIncludeLeadingSep);
  if(!QDir().exists(path))
  {
    if(!QDir().mkpath(path))
    {
      sWarn(tr("Cache directory \"%1\" cannot be created.").arg(path));
      goto ContinueNext;
    }
  }
  if(writeCache.open(QFile::WriteOnly))
  {
    writeCache.write(ret.data);
    writeCache.close();
  }
  else
    sWarn("Save cache to \"" + cacheFile + "\" failed! Msg: " + writeCache.errorString());

  qInfo() << "Resource " << reply->property("dest_file").toString() << " downloaded";

  // Tell ResourceContext
  if(!reply->property("outdated").toBool())
    emit AcquisitionFinish(id, ret);

ContinueNext:
  // Start next possible mission
  TryBeginAwaitingTasks();
}

void SpkResource::Acquire(SpkPageBase *dest, bool stopOngoing, bool clearQueue)
{
  for(auto &i : mWorkingRequests.keys())
  {
    // Don't let an outdated task falsely report a finish signal.
    // This is the designed way of stopping it from emitting a finish signal
    i->setProperty("outdated", true);
    // And abort as requested.
    if(stopOngoing)
      i->abort();
    mWorkingRequests.remove(i);
  }

  mWorkingRequests.clear();

  if(stopOngoing)
  {
    mRequestSemaphore->release(mMaximumConcurrent); // Release all semaphore users
  }

  if(clearQueue)
    mAwaitingRequests.clear();

  auto discResult = disconnect(this, SIGNAL(AcquisitionFinish(int, ResourceResult)), nullptr, nullptr);
//  qDebug() << "SpkResource: Acquisition disconnection" << (discResult ? "success" : "failure");

  connect(this, &SpkResource::AcquisitionFinish,
          dest, &SpkPageBase::ResourceAcquisitionFinished,
          Qt::QueuedConnection);
}

ResourceResult SpkResource::LocateCachedResource(const ResourceTask &task)
{
  // TODO: Test overhead of all these?
  auto dir = QDir(mCacheDirectory + task.pkgName + '/', ResourceName[task.type] + '*');
  auto list = dir.entryList();

  // If there's not even a file of the desired type, then tell invoker it's deferred
  if(list.isEmpty())
    return ResourceResult { .status = ResourceStatus::Deferred, .data = QByteArray() };

  // If there is the desired file, then we return the resource in binary
  auto cacheFullPath = GetCachePath(task);

  bool isCacheHit;
  // Wildcard support
  if(task.path == "*")
  {
    cacheFullPath.chop(1);
    auto filterResult = list.filter(SpkUtils::CutFileName(cacheFullPath));
    isCacheHit = !filterResult.isEmpty();
    if(isCacheHit)
      cacheFullPath = SpkUtils::CutPath(cacheFullPath) + '/' + filterResult[0];
  }
  else
  {
    isCacheHit = list.contains(SpkUtils::CutFileName(cacheFullPath));
  }

  if(isCacheHit)
  {
//    qInfo() << "Cache hit:" << GetCachePath(task);
    QFile cacheFile(cacheFullPath);
    if(cacheFile.open(QFile::ReadOnly))
    {
      ResourceResult ret { .status = ResourceStatus::Ready,
                           .data = cacheFile.readAll()};
      cacheFile.close();
      return ret;
    }
    else
    {
      // Cache file is unreadable, error and will be deleted
      sErr(tr("Desired cache file \"%1\" is unreadable by you!"));
    }
  }

  // If above two cases are not met, then we have outdated resources. Remove them.
  for(auto &i : list)
    QFile::remove(i);

  // Nothing to give you. Wait for completion.
  return ResourceResult { .status = ResourceStatus::Deferred, .data = QByteArray() };
}

void SpkResource::TryBeginAwaitingTasks()
{
  if(mAwaitingRequests.isEmpty())
    return;
  while(!mAwaitingRequests.isEmpty() && mRequestSemaphore->tryAcquire())
  {
    auto task = mAwaitingRequests.dequeue();
    // If no cached items exist, go download it
    auto reply = STORE->SendResourceRequest(task.path);
    // Store the destination cache file location in the property
    reply->setProperty("dest_file", GetCachePath(task));
    mWorkingRequests[reply] = task.id;
    connect(reply, &QNetworkReply::finished, this, &SpkResource::ResourceDownloaded);
  }
}

void SpkResource::PurgeCachedResource(const QString &aPkgName, SpkResource::ResourceType aType,
                                      const QVariant &aInfo)
{
  auto dir = QDir(mCacheDirectory + aPkgName + '/', ResourceName[aType] + '*');
  auto list = dir.entryList();
  sLog("Resource \"" + dir.absolutePath() + '/' + dir.nameFilters().constFirst() +
       "\" was requested to be removed.");

  if(list.isEmpty())
    return;
  for(auto &i : list)
    if(!QFile::remove(dir.absolutePath() + '/' + i))
      qInfo() << "Fail to remove broken cache " << dir.absolutePath() + '/' + i;
}

QString SpkResource::GetCachePath(const ResourceTask &task)
{
  return mCacheDirectory + task.pkgName + '/' + ResourceName.value(task.type) + '.' +
      task.info.toString() + '.' + SpkUtils::CutFileName(task.path);
}

ResourceResult SpkResource::CacheLookup(QString pkgName, ResourceType type, QVariant info)
{
  ResourceTask task { .pkgName = pkgName, .path = "*", .type = type, .info = info, .id = -1 };
  return LocateCachedResource(task);
}

const QMap<SpkResource::ResourceType, QString> SpkResource::ResourceName
{
  { SpkResource::ResourceType::AppIcon, "icon" },
  { SpkResource::ResourceType::AppScreenshot, "scrshot" },
  { SpkResource::ResourceType::HomeImage, "img" },
  { SpkResource::ResourceType::TagIcon, "tag" },
};