#include <QApplication>
#include <QLabel>
#include <QMainWindow>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QMainWindow window;

  window.setWindowTitle("DownloadX");
  window.resize(1000, 650);

  auto *label = new QLabel("DownloadX - Download Manager");

  label->setAlignment(Qt::AlignCenter);

  window.setCentralWidget(label);

  window.show();

  return app.exec();
}
