#include "DownloadManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

DownloadManager::DownloadManager(QObject *parent) : QObject(parent) {}

void DownloadManager::download(const QUrl &url) {
  if (!url.isValid()) {
    emit downloadError("Invalid URL");
    return;
  }

  QString downloadDirectory =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

  QDir().mkpath(downloadDirectory);

  QString fileName = QFileInfo(url.path()).fileName();

  if (fileName.isEmpty()) {
    fileName = "download";
  }

  QString filePath = downloadDirectory + "/" + fileName;

  auto *file = new QFile(filePath, this);

  if (!file->open(QIODevice::WriteOnly)) {
    emit downloadError("Cannot create file: " + filePath);

    delete file;
    return;
  }

  emit statusChanged("Downloading...");

  QNetworkRequest request(url);

  QNetworkReply *reply = manager.get(request);

  connect(reply, &QNetworkReply::readyRead, this,
          [reply, file]() { file->write(reply->readAll()); });

  connect(reply, &QNetworkReply::downloadProgress, this,
          &DownloadManager::progress);

  connect(reply, &QNetworkReply::finished, this,
          [this, reply, file, filePath]() {
            file->write(reply->readAll());
            file->close();

            if (reply->error() != QNetworkReply::NoError) {
              file->remove();

              emit downloadError(reply->errorString());
            } else {
              emit statusChanged("Completed");

              emit downloadFinished(filePath);
            }

            reply->deleteLater();
            file->deleteLater();
          });
}
