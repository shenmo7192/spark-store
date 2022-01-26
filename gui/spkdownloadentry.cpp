﻿
#include "spkdownloadentry.h"
#include "spklogging.h"
#include "spkutils.h"
#include <QDebug>

constexpr QSize SpkDownloadEntry::IconSize;

SpkDownloadEntry::SpkDownloadEntry(QWidget *parent)
{
  mIcon = new QLabel;
  mAppName = new ElidedLabel;
  mMessage = new QLabel;
  mProgress = new QProgressBar;
  mLoading = new SpkLoading;
  mBtnDelete = new QPushButton;
  mBtnActions = new QPushButton;

  mLayInfo = new QVBoxLayout;
  mLayMsgs = new QHBoxLayout;
  mLayMain = new QHBoxLayout;

  mLoading->setVisible(false);
  mIcon->setFixedSize(IconSize);
  mProgress->setRange(0, 1000);

  mLayMsgs->addWidget(mAppName);
  mLayMsgs->addStretch();
  mLayMsgs->addWidget(mMessage);

  mLayInfo->addLayout(mLayMsgs);
  mLayInfo->addWidget(mProgress);
  mLayInfo->setAlignment(Qt::AlignVCenter);

  mLayMain->addWidget(mIcon);
  mLayMain->addLayout(mLayInfo);
  mLayMain->addWidget(mLoading);
  mLayMain->addWidget(mBtnActions);
  mLayMain->addWidget(mBtnDelete);

  setLayout(mLayMain);

  mLastReportTime = QTime::currentTime();
}


SpkDownloadEntry::~SpkDownloadEntry()
{
  // TODO
}

void SpkDownloadEntry::SetTotalBytes(qint64 total)
{
  mTotalBytes = total;
  mReadableTotalSize = SpkUtils::BytesToSize(total);
  mLastReportTime = QTime::currentTime();
}

void SpkDownloadEntry::SetBasicInfo(QString name, QPixmap icon)
{
  mAppName->setText(name);
  mIcon->setPixmap(icon.scaled(IconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

void SpkDownloadEntry::SetStatus(DownloadEntryStatus status, QString msg)
{
  switch(status)
  {
    case Waiting:
      mMessage->setText(tr("Waiting for download"));
      mProgress->setVisible(false);
      mBtnActions->setVisible(false);
      mBtnDelete->setVisible(true);
      break;

    case Downloading:
      mMessage->setText(tr(""));
      mProgress->setVisible(true);
      mBtnActions->setVisible(true);
      break;

    case Paused:
      mMessage->setText(tr("Paused"));
      break;

    case Failed:
      mMessage->setText(msg);
      mProgress->setVisible(false);
      break;

    case ToBeInstalled:
      mMessage->setText(tr("Download Finished"));
      mProgress->setVisible(false);
      break;

    case Installing:
      mMessage->setText("");
      mProgress->setVisible(false);
      mLoading->setVisible(true);
      mLoading->Begin();

    case Installed:
      mMessage->setText(tr("Installed"));
      mLoading->End();
      mLoading->setVisible(false);
      break;

    case InstallFailed:
      mMessage->setText(msg.isEmpty() ? tr("Install Failed") : msg);
      mLoading->End();
      mLoading->setVisible(false);
      break;

    case Invalid:
      break;
  }
}

void SpkDownloadEntry::Progress(qint64 bytes)
{
  auto now = QTime::currentTime();
  auto msecDiff = mLastReportTime.msecsTo(now);

  if(msecDiff != 0)
  {
    auto bytesPerSec = (bytes - mDownloadedBytes) / (msecDiff / 1000.0);
    qDebug() << "Bytes" << bytes - mDownloadedBytes
             << "MsDiff" << msecDiff / 1000.0
             << "Bytes-Per-Seg" << bytesPerSec;
    auto speedSize = SpkUtils::BytesToSize(static_cast<size_t>(bytesPerSec));
    mMessage->setText(QString("%1/%2(%3/s)")
                      .arg(SpkUtils::BytesToSize(bytes), mReadableTotalSize, speedSize));
  }
  mDownloadedBytes = bytes;
  mProgress->setValue(static_cast<int>(((double)bytes) / mTotalBytes * 1000));
  mLastReportTime = now;
}
