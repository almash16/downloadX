#pragma once

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

  void startDownload();
};
