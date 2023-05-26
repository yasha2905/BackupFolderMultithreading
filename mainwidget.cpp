#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QDateTime>
#include <QDebug>

MainWidget::MainWidget(QWidget *parent) : QWidget(parent), ui(new Ui::MainWidget)
{
  ui->setupUi(this);
  model = new QFileSystemModel(this);
  model->setFilter(QDir::QDir::AllEntries);
  model->setRootPath("");
  ui->lvBackup->setModel(model);
  ui->lvSource->setModel(model);

  //object of thread creation
  thread = new QThread(this);
  //thread termination main form deleting
  connect(this, SIGNAL(destroyed()), thread, SLOT(quit()));

  connect(ui->lvBackup, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(on_lvSource_doubleClicked(QModelIndex)));

  //Working object creation (!!! if use moveToThread() its forbidden to use parent)

  worker = new BackupWorker();
  //Qt allows to connect signals and slots from different threats
  // Running backuping if signal startOperation
  connect(this, SIGNAL(startOperation(QString,QString)),
          worker, SLOT(runBackup(QString,QString)));


  // moving working object from main thread to thread
    worker->moveToThread(thread);
  //and running the thread
  thread->start();

  //next running preparation after previous finished
  connect(worker, SIGNAL(backupFinished()), this, SLOT(readyToStart()));

  //removing the working object after thread finished
  connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater()));
}

MainWidget::~MainWidget()
{
  delete ui;
}

void BackupWorker::contentDifference(QDir &sDir, QDir &dDir, QFileInfoList &diffList)
{
  foreach(QFileInfo sInfo, sDir.entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name|QDir::DirsFirst)){
    bool fileExists = false;
    foreach(QFileInfo dInfo, dDir.entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name|QDir::DirsFirst)){
      if (sInfo.fileName() == dInfo.fileName()){
        if (sInfo.isDir() || sInfo.lastModified() <= dInfo.lastModified())
          fileExists = true;
        break;
      }
    }
    if (!fileExists)
      diffList.append(sInfo);
    if (sInfo.isFile())
      continue;
    if (fileExists){
      sDir.cd(sInfo.fileName());
      dDir.cd(sInfo.fileName());
      contentDifference(sDir, dDir, diffList);
      sDir.cdUp();
      dDir.cdUp();
    }
    else {
      sDir.cd(sInfo.fileName());
      recursiveContentList(sDir, diffList);
      sDir.cdUp();
    }
  }
}

void BackupWorker::recursiveContentList(QDir &dir, QFileInfoList &contentList)
{
  foreach(QFileInfo info, dir.entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name|QDir::DirsFirst)){
    contentList.append(info);
    if (info.isDir() && dir.cd(info.fileName())){
      recursiveContentList(dir, contentList);
      dir.cdUp();
    }
  }
}

void MainWidget::on_lvSource_doubleClicked(const QModelIndex &index)
{
  QListView* listView = (QListView*)sender();
  QFileInfo fileInfo = model->fileInfo(index);
  if (fileInfo.fileName() == ".."){
    QDir dir = fileInfo.dir();
    dir.cdUp();
    listView->setRootIndex(model->index(dir.absolutePath()));
  }
  else if (fileInfo.fileName() == "."){
    listView->setRootIndex(model->index(""));
  }
  else if (fileInfo.isDir()){
    listView->setRootIndex(index);
  }
}

void MainWidget::on_btnBackup_clicked()
{
  QString sDir = model->filePath(ui->lvSource->rootIndex());
  QString dDir = model->filePath(ui->lvBackup->rootIndex());

  ui->btnBackup->setEnabled(false);
  emit startOperation(sDir, dDir);
}

void MainWidget::readyToStart()
{
  ui->btnBackup->setEnabled(true);
}


BackupWorker::BackupWorker(QObject *parent) : QObject(parent)
{
}

BackupWorker::~BackupWorker()
{
  qDebug() << "destroyed";
}

void BackupWorker::runBackup(QString sPath, QString dPath)
{
  QDir sDir = QDir(sPath);
  QDir dDir = QDir(dPath);

  QFileInfoList diffList = QFileInfoList();
  contentDifference(sDir, dDir, diffList);

  foreach(QFileInfo diffInfo, diffList){
    QString backupPath = diffInfo.filePath().replace(sDir.absolutePath(), dDir.absolutePath());
    if (diffInfo.isFile()){
      QFile::remove(backupPath);
      QFile::copy(diffInfo.absoluteFilePath(), backupPath);
    }
    if (diffInfo.isDir()){
      dDir.mkdir(backupPath);
    }
  }
  emit backupFinished();
}
