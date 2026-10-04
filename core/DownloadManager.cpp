#include "DownloadManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>

DownloadManager::DownloadManager(QObject *parent) : QObject(parent) {}

void DownloadManager::download(const QUrl &url) {
  currentUrl = url;

  if (!url.isValid() || url.scheme() != "http" && url.scheme() != "https") {
    emit downloadError("Invalid HTTP/HTTPS URL.");
    return;
  }

  QString downloadDirectory =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

  if (downloadDirectory.isEmpty()) {
    emit downloadError("Could not find Downloads directory.");
    return;
  }

  QDir().mkpath(downloadDirectory);

  QString fileName = determineFileName(url);

  outputFile = downloadDirectory + "/" + fileName;

  /*
   * Temporary directory.
   */

  tempDirectory = downloadDirectory + "/.downloadx_" +
                  QUuid::createUuid().toString(QUuid::WithoutBraces);

  QDir().mkpath(tempDirectory);

  segments.clear();

  totalSize = 0;
  totalReceived = 0;
  completedSegments = 0;

  rangeSupported = false;
  singleConnectionMode = false;

  emit statusChanged("Checking server...");

  probeServer();
}

QString DownloadManager::determineFileName(const QUrl &url) const {
  QString fileName = QFileInfo(url.path()).fileName();

  if (fileName.isEmpty())
    fileName = "download";

  /*
   * Remove characters that are unsafe in filenames.
   */

  fileName.replace("/", "_");
  fileName.replace("\\", "_");

  return fileName;
}

void DownloadManager::probeServer() {
  QNetworkRequest request(currentUrl);

  /*
   * Ask for one byte.
   */

  request.setRawHeader("Range", "bytes=0-0");

  QNetworkReply *probe = manager.get(request);

  connect(probe, &QNetworkReply::finished, this, [this, probe]() {
    int statusCode =
        probe->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    /*
     * 429 = Too Many Requests
     */

    if (statusCode == 429) {
      QByteArray retryAfter = probe->rawHeader("Retry-After");

      int delay = 2000;

      bool ok = false;

      int seconds = retryAfter.toInt(&ok);

      if (ok && seconds > 0)
        delay = seconds * 1000;

      probe->deleteLater();

      emit statusChanged(QString("Server rate limited request. "
                                 "Retrying in %1 seconds...")
                             .arg(delay / 1000));

      QTimer::singleShot(delay, this, [this]() { probeServer(); });

      return;
    }

    if (statusCode == 206) {
      QByteArray range = probe->rawHeader("Content-Range");

      int slash = range.indexOf('/');

      if (slash == -1) {
        probe->deleteLater();

        fail("Server returned invalid Content-Range.");
        return;
      }

      QByteArray sizePart = range.mid(slash + 1);

      bool ok = false;

      totalSize = sizePart.toLongLong(&ok);

      if (!ok || totalSize <= 0) {
        probe->deleteLater();

        fail("Could not determine file size.");
        return;
      }

      rangeSupported = true;

      probe->deleteLater();

      emit statusChanged(QString("Server supports segmented downloading. "
                                 "Size: %1 bytes")
                             .arg(totalSize));

      prepareSegments();

      return;
    }

    /*
     * Some servers ignore Range and return 200.
     *
     * In that case we use one normal connection.
     */

    if (statusCode == 200) {
      QByteArray contentLength = probe->rawHeader("Content-Length");

      bool ok = false;

      totalSize = contentLength.toLongLong(&ok);

      if (!ok)
        totalSize = -1;

      singleConnectionMode = true;

      probe->deleteLater();

      emit statusChanged("Server does not support segmented "
                         "downloading. Using single connection.");

      startSingleDownload();

      return;
    }

    QString error = QString("Server returned HTTP status %1").arg(statusCode);

    probe->deleteLater();

    fail(error);
  });
}

void DownloadManager::prepareSegments() {
  segments.clear();

  qint64 segmentSize = totalSize / segmentCount;

  for (int i = 0; i < segmentCount; ++i) {
    Segment segment;

    segment.start = i * segmentSize;

    if (i == segmentCount - 1) {
      segment.end = totalSize - 1;
    } else {
      segment.end = ((i + 1) * segmentSize) - 1;
    }

    segment.filePath = tempDirectory + QString("/part_%1").arg(i);

    segments.append(segment);
  }

  totalReceived = 0;
  completedSegments = 0;

  emit statusChanged(
      QString("Downloading with %1 connections...").arg(segmentCount));

  for (int i = 0; i < segments.size(); ++i) {
    startSegment(i);
  }
}

void DownloadManager::startSegment(int index) {
  if (index < 0 || index >= segments.size())
    return;

  Segment &segment = segments[index];

  /*
   * New attempt starts from zero.
   */

  segment.received = 0;

  QNetworkRequest request(currentUrl);

  QString range = QString("bytes=%1-%2").arg(segment.start).arg(segment.end);

  request.setRawHeader("Range", range.toUtf8());

  QNetworkReply *reply = manager.get(request);

  segment.reply = reply;

  QFile *file = new QFile(segment.filePath, this);

  if (!file->open(QIODevice::WriteOnly)) {
    reply->abort();
    reply->deleteLater();

    delete file;

    fail("Could not create temporary segment file.");
    return;
  }

  segment.file = file;

  /*
   * Incoming binary data.
   *
   * This works for:
   *
   * ZIP
   * MP4
   * EXE
   * ISO
   * PDF
   * MP3
   * APK
   * etc.
   */

  connect(reply, &QNetworkReply::readyRead, this,
          [reply, file]() { file->write(reply->readAll()); });

  connect(reply, &QNetworkReply::downloadProgress, this,
          [this, index](qint64 received, qint64) {
            if (index < 0 || index >= segments.size())
              return;

            Segment &segment = segments[index];

            qint64 difference = received - segment.received;

            segment.received = received;

            totalReceived += difference;

            emit progress(totalReceived, totalSize);
          });

  connect(reply, &QNetworkReply::finished, this, [this, reply, file, index]() {
    file->write(reply->readAll());

    file->flush();
    file->close();

    int statusCode =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    /*
     * HTTP 429
     */

    if (statusCode == 429) {
      QByteArray retryAfter = reply->rawHeader("Retry-After");

      int delay = 2000;

      bool ok = false;

      int seconds = retryAfter.toInt(&ok);

      if (ok && seconds > 0)
        delay = seconds * 1000;

      reply->deleteLater();

      file->deleteLater();

      /*
       * Remove bytes from this failed
       * attempt from global progress.
       */

      totalReceived -= segments[index].received;

      segments[index].received = 0;

      if (segments[index].retryCount >= maxRetries) {
        fail(QString("Segment %1 was rate limited "
                     "too many times.")
                 .arg(index + 1));

        return;
      }

      segments[index].retryCount++;

      retrySegment(index, delay);

      return;
    }

    /*
     * Other network errors.
     */

    if (reply->error() != QNetworkReply::NoError) {
      QString error = reply->errorString();

      reply->deleteLater();

      file->deleteLater();

      totalReceived -= segments[index].received;

      segments[index].received = 0;

      if (segments[index].retryCount >= maxRetries) {
        fail(QString("Segment %1 failed: %2").arg(index + 1).arg(error));

        return;
      }

      segments[index].retryCount++;

      /*
       * Exponential backoff:
       *
       * 1s
       * 2s
       * 4s
       * 8s
       */

      int delay = 1000 * (1 << (segments[index].retryCount - 1));

      retrySegment(index, delay);

      return;
    }

    /*
     * Make sure this was actually a
     * partial response.
     */

    if (statusCode != 206) {
      reply->deleteLater();
      file->deleteLater();

      totalReceived -= segments[index].received;

      segments[index].received = 0;

      fail(QString("Segment %1 did not receive "
                   "HTTP 206.")
               .arg(index + 1));

      return;
    }

    reply->deleteLater();
    file->deleteLater();

    segments[index].reply = nullptr;
    segments[index].file = nullptr;

    ++completedSegments;

    emit statusChanged(QString("Segment %1/%2 completed.")
                           .arg(completedSegments)
                           .arg(segments.size()));

    if (completedSegments == segments.size()) {
      emit statusChanged("Merging segments...");

      mergeSegments();
    }
  });
}

void DownloadManager::retrySegment(int index, int delayMs) {
  emit statusChanged(QString("Retrying segment %1 in %2 seconds...")
                         .arg(index + 1)
                         .arg(delayMs / 1000));

  QTimer::singleShot(delayMs, this, [this, index]() {
    if (index >= 0 && index < segments.size()) {
      startSegment(index);
    }
  });
}

void DownloadManager::startSingleDownload() {
  QNetworkRequest request(currentUrl);

  QNetworkReply *reply = manager.get(request);

  QFile *file = new QFile(outputFile, this);

  if (!file->open(QIODevice::WriteOnly)) {
    reply->abort();
    reply->deleteLater();

    delete file;

    fail("Could not create output file.");
    return;
  }

  connect(reply, &QNetworkReply::readyRead, this,
          [reply, file]() { file->write(reply->readAll()); });

  connect(reply, &QNetworkReply::downloadProgress, this,
          [this](qint64 received, qint64 total) {
            totalReceived = received;

            if (totalSize <= 0)
              totalSize = total;

            emit progress(received, totalSize);
          });

  connect(reply, &QNetworkReply::finished, this, [this, reply, file]() {
    file->write(reply->readAll());

    file->flush();
    file->close();

    if (reply->error() != QNetworkReply::NoError) {
      QString error = reply->errorString();

      reply->deleteLater();
      file->deleteLater();

      fail(error);
      return;
    }

    reply->deleteLater();
    file->deleteLater();

    emit statusChanged("Completed");

    emit progress(totalReceived, totalSize);

    cleanupSegments();

    emit downloadFinished(outputFile);
  });
}

void DownloadManager::mergeSegments() {
  QFile output(outputFile);

  if (!output.open(QIODevice::WriteOnly)) {
    fail("Could not create output file.");
    cleanupSegments();
    return;
  }

  for (const Segment &segment : segments) {
    QFile part(segment.filePath);

    if (!part.open(QIODevice::ReadOnly)) {
      output.close();

      fail(QString("Could not read segment: %1").arg(segment.filePath));

      cleanupSegments();
      return;
    }

    while (!part.atEnd()) {
      QByteArray data = part.read(1024 * 1024);

      if (data.isEmpty())
        break;

      output.write(data);
    }

    part.close();
  }

  output.flush();
  output.close();

  cleanupSegments();

  emit statusChanged("Completed");

  emit progress(totalSize, totalSize);

  emit downloadFinished(outputFile);
}

void DownloadManager::cleanupSegments() {
  QDir directory(tempDirectory);

  if (directory.exists())
    directory.removeRecursively();
}

void DownloadManager::fail(const QString &error) {
  cleanupSegments();

  emit downloadError(error);
}
