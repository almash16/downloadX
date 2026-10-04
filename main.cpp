#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

class MainWindow : public QMainWindow {
public:
  MainWindow() {
    setWindowTitle("DownloadX");
    resize(1100, 700);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // =========================
    // Sidebar
    // =========================

    auto *sidebar = new QFrame;
    sidebar->setFixedWidth(220);

    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(20, 30, 20, 20);

    auto *logo = new QLabel("DownloadX");

    QFont logoFont;
    logoFont.setPointSize(20);
    logoFont.setBold(true);
    logo->setFont(logoFont);

    auto *homeButton = new QPushButton("Home");
    auto *downloadsButton = new QPushButton("Downloads");
    auto *completedButton = new QPushButton("Completed");

    auto *settingsButton = new QPushButton("Settings");

    sidebarLayout->addWidget(logo);
    sidebarLayout->addSpacing(30);

    sidebarLayout->addWidget(homeButton);
    sidebarLayout->addWidget(downloadsButton);
    sidebarLayout->addWidget(completedButton);

    sidebarLayout->addStretch();

    sidebarLayout->addWidget(settingsButton);

    // =========================
    // Main content
    // =========================

    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);

    contentLayout->setContentsMargins(30, 30, 30, 30);
    contentLayout->setSpacing(20);

    auto *title = new QLabel("Downloads");

    QFont titleFont;
    titleFont.setPointSize(26);
    titleFont.setBold(true);

    title->setFont(titleFont);

    // URL input

    auto *urlLayout = new QHBoxLayout();

    auto *urlInput = new QLineEdit;
    urlInput->setPlaceholderText("Paste download URL...");

    auto *addButton = new QPushButton("Add Download");
    addButton->setMinimumWidth(140);

    urlLayout->addWidget(urlInput);
    urlLayout->addWidget(addButton);

    // Downloads table

    auto *table = new QTableWidget;

    table->setColumnCount(4);

    table->setHorizontalHeaderLabels({"File", "Size", "Progress", "Status"});

    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Add widgets

    contentLayout->addWidget(title);
    contentLayout->addLayout(urlLayout);
    contentLayout->addWidget(table);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(content);

    // =========================
    // Add download button
    // =========================

    connect(addButton, &QPushButton::clicked, this, [=]() {
      const QString url = urlInput->text().trimmed();

      if (url.isEmpty())
        return;

      const int row = table->rowCount();

      table->insertRow(row);

      table->setItem(row, 0, new QTableWidgetItem(url));

      table->setItem(row, 1, new QTableWidgetItem("Unknown"));

      table->setItem(row, 2, new QTableWidgetItem("0%"));

      table->setItem(row, 3, new QTableWidgetItem("Queued"));

      urlInput->clear();
    });
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  MainWindow window;

  window.show();

  return app.exec();
}
