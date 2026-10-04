#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>
#include <QtGlobal>

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
  // ========================================================
  // DOWNLOAD SEGMENT
  // ========================================================

  struct Segment {
    qint64 start = 0;
    qint64 end = 0;
    qint64 received = 0;

    int retryCount = 0;

    QString filePath;

    QNetworkReply *reply = nullptr;

    QFile *file = nullptr;
  };

  // ========================================================
  // DOWNLOAD FUNCTIONS
  // ========================================================

  void probeServer();

  void prepareSegments();

  void startSegment(int index);

  void retrySegment(int index, int delayMs);

  void startSingleDownload();

  void mergeSegments();

  void cleanupSegments();

  void fail(const QString &error);

  // ========================================================
  // HELPERS
  // ========================================================

  QString determineFileName(const QUrl &url) const;

  // ========================================================
  // NETWORK
  // ========================================================

  QNetworkAccessManager manager;

  QUrl currentUrl;

  // ========================================================
  // OUTPUT
  // ========================================================

  QString outputFile;

  QString tempDirectory;

  // ========================================================
  // SEGMENTS
  // ========================================================

  QVector<Segment> segments;

  int segmentCount = 4;

  int completedSegments = 0;

  int maxRetries = 4;

  // ========================================================
  // PROGRESS
  // ========================================================

  qint64 totalSize = 0;

  qint64 totalReceived = 0;

  // ========================================================
  // SERVER CAPABILITIES
  // ========================================================

  bool rangeSupported = false;

  bool singleConnectionMode = false;
};
