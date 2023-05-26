#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QFileSystemModel>
#include <QDir>
#include <QThread>

namespace Ui {
class MainWidget;
}

/* Class of working objext (just a class for longlive execution */
class BackupWorker : public QObject
{
  Q_OBJECT
public:
  explicit BackupWorker(QObject* parent = 0);
  ~BackupWorker();
public slots:
  void runBackup(QString sPath, QString dPath);
signals:
  void backupFinished();
private:
  //looking for an absence or unactual elements in backup dir
  void contentDifference(QDir &sDir, QDir &dDir, QFileInfoList &diffList);
  //to fill up all included dirs and files
  void recursiveContentList(QDir &dir, QFileInfoList &contentList);
};

class MainWidget : public QWidget
{
  Q_OBJECT

public:
  explicit MainWidget(QWidget *parent = 0);
  ~MainWidget();
private slots:

  void on_lvSource_doubleClicked(const QModelIndex &index);

  void on_btnBackup_clicked();
  void readyToStart();
signals:
  void startOperation(QString sPath, QString dPath);
private:
  Ui::MainWidget *ui;
  QFileSystemModel *model;
  //working object
  BackupWorker *worker;
  //Class QThread object to execute in separate thread
  QThread *thread;
};

#endif // MAINWIDGET_H
