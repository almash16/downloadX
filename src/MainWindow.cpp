#include "MainWindow.h"

#include "../core/DownloadManager.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("DownloadX");
  resize(1100, 700);

  downloadManager = new DownloadManager(this);

  auto *central = new QWidget(this);
  setCentralWidget(central);

  auto *mainLayout = new QVBoxLayout(central);

  mainLayout->setContentsMargins(30, 30, 30, 30);

  // Title

  auto *title = new QLabel("DownloadX");

  QFont titleFont;
  titleFont.setPointSize(28);
  titleFont.setBold(true);

  title->setFont(titleFont);

  mainLayout->addWidget(title);

  // URL input

  auto *urlLayout = new QHBoxLayout();

  urlInput = new QLineEdit();

  urlInput->setPlaceholderText("Paste download URL...");

  auto *addButton = new QPushButton("Download");

  urlLayout->addWidget(urlInput);
  urlLayout->addWidget(addButton);

  mainLayout->addLayout(urlLayout);

  // Progress

  progressBar = new QProgressBar();

  progressBar->setRange(0, 100);
  progressBar->setValue(0);

  mainLayout->addWidget(progressBar);

  // Table

  table = new QTableWidget();

  table->setColumnCount(4);

  table->setHorizontalHeaderLabels({"File", "Size", "Progress", "Status"});

  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  table->setEditTriggers(QAbstractItemView::NoEditTriggers);

  mainLayout->addWidget(table);

  // Button

  connect(addButton, &QPushButton::clicked, this, &MainWindow::startDownload);

  // Progress

  connect(downloadManager, &DownloadManager::progress, this,
          [this](qint64 received, qint64 total) {
            if (total <= 0)
              return;

            totalSize = total;

            int percent = static_cast<int>((received * 100) / total);

            progressBar->setValue(percent);

            /*
             * Calculate speed
             */

            qint64 elapsed = speedTimer.elapsed();

            double speed = 0.0;

            if (elapsed > 0) {
              qint64 difference = received - lastReceived;

              speed = static_cast<double>(difference) /
                      (static_cast<double>(elapsed) / 1000.0);
            }

            /*
             * Reset measurement window
             */

            if (elapsed >= 500) {
              lastReceived = received;
              speedTimer.restart();
            }

            /*
             * ETA
             */

            qint64 remaining = total - received;

            qint64 eta = 0;

            if (speed > 0) {
              eta = static_cast<qint64>(remaining / speed);
            }

            /*
             * Update table
             */

            if (table->rowCount() > 0) {
              int row = table->rowCount() - 1;

              table->item(row, 1)->setText(formatSize(total));

              table->item(row, 2)->setText(QString("%1%").arg(percent));

              table->item(row, 3)->setText(QString("%1/s • ETA %2")
                                               .arg(formatSpeed(speed))
                                               .arg(formatTime(eta)));
            }
          });
  // Status

  connect(downloadManager, &DownloadManager::statusChanged, this,
          [this](const QString &status) {
            if (table->rowCount() > 0) {
              table->item(table->rowCount() - 1, 3)->setText(status);
            }
          });

  // Finished

  connect(downloadManager, &DownloadManager::downloadFinished, this,
          [this](const QString &filePath) {
            QMessageBox::information(this, "Download Complete",
                                     "Downloaded to:\n" + filePath);
          });

  // Error

  connect(downloadManager, &DownloadManager::downloadError, this,
          [this](const QString &error) {
            QMessageBox::critical(this, "Download Error", error);
          });
}

void MainWindow::startDownload() {
  const QString urlText = urlInput->text().trimmed();

  QUrl url(urlText);

  if (!url.isValid() || url.scheme() != "http" && url.scheme() != "https") {
    QMessageBox::warning(this, "Invalid URL",
                         "Please enter a valid HTTP or HTTPS URL.");

    return;
  }
  lastReceived = 0;
  totalSize = 0;
  speedTimer.restart();

  const int row = table->rowCount();

  table->insertRow(row);

  table->setItem(row, 0, new QTableWidgetItem(url.fileName()));

  table->setItem(row, 1, new QTableWidgetItem("Unknown"));

  table->setItem(row, 2, new QTableWidgetItem("0%"));

  table->setItem(row, 3, new QTableWidgetItem("Queued"));

  progressBar->setValue(0);

  downloadManager->download(url);

  urlInput->clear();
}
QString MainWindow::formatSize(qint64 bytes) const {
  if (bytes < 1024)
    return QString("%1 B").arg(bytes);

  if (bytes < 1024 * 1024)
    return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);

  if (bytes < 1024LL * 1024LL * 1024LL)
    return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);

  return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

QString MainWindow::formatSpeed(double bytesPerSecond) const {
  if (bytesPerSecond < 1024) {
    return QString("%1 B").arg(bytesPerSecond, 0, 'f', 0);
  }

  if (bytesPerSecond < 1024 * 1024) {
    return QString("%1 KB").arg(bytesPerSecond / 1024.0, 0, 'f', 1);
  }

  return QString("%1 MB").arg(bytesPerSecond / (1024.0 * 1024.0), 0, 'f', 2);
}

QString MainWindow::formatTime(qint64 seconds) const {
  if (seconds <= 0)
    return "--";

  qint64 minutes = seconds / 60;
  qint64 secs = seconds % 60;

  if (minutes > 0) {
    return QString("%1m %2s").arg(minutes).arg(secs);
  }

  return QString("%1s").arg(secs);
}
