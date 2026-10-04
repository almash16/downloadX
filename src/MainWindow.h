#pragma once

#include <QElapsedTimer>
#include <QMainWindow>

class QLineEdit;
class QTableWidget;
class QProgressBar;
class DownloadManager;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

private:
  QLineEdit *urlInput;
  QTableWidget *table;
  QProgressBar *progressBar;

  DownloadManager *downloadManager;

  QElapsedTimer speedTimer;

  qint64 lastReceived = 0;
  qint64 totalSize = 0;

  void startDownload();

  QString formatSize(qint64 bytes) const;
  QString formatSpeed(double bytesPerSecond) const;
  QString formatTime(qint64 seconds) const;
};
