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
            if (total > 0) {
              int percent = static_cast<int>((received * 100) / total);

              progressBar->setValue(percent);

              if (table->rowCount() > 0) {
                table->item(table->rowCount() - 1, 2)
                    ->setText(QString("%1%").arg(percent));
              }
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
