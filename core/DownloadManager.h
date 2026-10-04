#pragma once

#include <QNetworkAccessManager>
#include <QObject>

class QNetworkReply;
class QFile;

class DownloadManager : public QObject {
  Q_OBJECT

public:
  explicit DownloadManager(QObject *parent = nullptr);

  void download(const QUrl &url);

signals:
  void progress(qint64 received, qint64 total);
  void statusChanged(const QString &status);
  void downloadFinished(const QString &filePath);
  void downloadError(const QString &error);

private:
  QNetworkAccessManager manager;
};
