#include "downloadthreadmanager.h"
#include "downloadobject.h"

#include <QDebug>
#include <QFileInfo>
#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

DownloadThreadManager::DownloadThreadManager(QObject *parent)
    : QObject(parent)
{
    m_file = nullptr;
    m_state = DownloadThread::D_Waiting;
}

DownloadThreadManager::~DownloadThreadManager()
{
    delete m_file;
    qDeleteAll(m_threads);
}

QString DownloadThreadManager::getClassName()
{
    return staticMetaObject.className();
}

qint64 DownloadThreadManager::getFileSize(const QString &url, int tryTimes)
{
    qint64 size = -1;
    while(tryTimes--)
    {
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply = manager.head(QNetworkRequest(url));
        if(!reply)
        {
            continue;
        }
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        if(reply->error() != QNetworkReply::NoError)
        {
            continue;
        }

        size = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        reply->deleteLater();
        break;
    }
    return size;
}

bool DownloadThreadManager::downloadFile(const QString &url)
{
    emit stateChanged(tr("D_Waiting"));

    if(m_state == DownloadThread::D_Download)
    {
        qDebug() << "Current is downloading a file";
        return false;
    }

    if(THREADCOUNT < 1 || THREADCOUNT > 15)
    {
        qDebug() << "Download thread count error";
        return false;
    }

    if((m_totalSize = getFileSize(url)) == -1)
    {
        qDebug() << "Download file size error";
        return false;
    }

#ifdef DOWNLOAD_WINEXTRAS
    QString fileName = QUrl(url).fileName();
#else
    const QString ourPath = QUrl(url).path();
    const int slash = ourPath.lastIndexOf(QLatin1Char('/'));
    QString fileName = (slash == -1) ? ourPath : ourPath.mid(slash + 1);
#endif
    emit updateFileInfoChanged(fileName, m_totalSize);

    m_readySize = 0;
    m_file = new QFile(fileName + ".download", this);
    if(!m_file->open(QFile::WriteOnly))
    {
        m_file->close();
        qDebug() << "Can not open file : " + m_file->errorString();
        delete m_file;
        m_file = nullptr;
        return false;
    }

    m_file->resize(m_totalSize);
    qDeleteAll(m_threads);
    m_threads.clear();

    for(int i=0; i<THREADCOUNT; ++i)
    {
        qint64 startPoint = m_totalSize * i / THREADCOUNT;
        qint64 endPoint = m_totalSize * (i + 1) / THREADCOUNT;
        DownloadThread *thread = new DownloadThread(this);
        connect(thread, SIGNAL(finished(int)), SLOT(finishedSlot(int)));
        connect(thread, SIGNAL(downloadChanged()), SLOT(progressChangedSlot()));
        connect(thread, SIGNAL(errorCode(int,QString)), SLOT(errorSlot(int,QString)));
        thread->startDownload(i, url, m_file, startPoint, endPoint);
        m_threads.append(thread);
    }

    m_state = DownloadThread::D_Download;
    emit stateChanged(tr("D_Download"));
    m_runningCount = THREADCOUNT;

    return true;
}

void DownloadThreadManager::downloadingFinish()
{
    QString fileName = m_file->fileName();
    m_file->flush();
    m_file->close();
    delete m_file;
    m_file = nullptr;
    m_state = DownloadThread::D_Finished;

    qDeleteAll(m_threads);
    m_threads.clear();

    int index = fileName.lastIndexOf('.');
    QFile::rename(fileName, fileName.left(index));

    emit stateChanged(tr("D_Finished"));
    emit downloadingFinished();
}

void DownloadThreadManager::pause()
{
    if(m_state != DownloadThread::D_Download)
    {
        qDebug() << "Current is not downloading";
        return;
    }

    m_state = DownloadThread::D_Pause;
    emit stateChanged(tr("D_Pause"));

    foreach(DownloadThread *thread, m_threads)
    {
        thread->stop();
    }
}

void DownloadThreadManager::restart()
{
    if(m_state != DownloadThread::D_Pause)
    {
        qDebug() << "Current is not paused";
        return;
    }

    m_state = DownloadThread::D_Download;
    emit stateChanged(tr("D_Download"));

    foreach(DownloadThread *thread, m_threads)
    {
        thread->restart();
    }
}

void DownloadThreadManager::finishedSlot(int index)
{
    m_runningCount--;
    qDebug() << "Download index of " << index << " finished";

    if(m_runningCount == 0 && m_state == DownloadThread::D_Download)
    {
        downloadingFinish();
    }
}

void DownloadThreadManager::progressChangedSlot()
{
    m_readySize = 0;
    foreach(DownloadThread *thread, m_threads)
    {
        m_readySize += thread->getReadySize();
    }

    emit progressChanged(m_readySize, m_totalSize);
}

void DownloadThreadManager::errorSlot(int index, const QString &errorString)
{
    if(index < 0 || index >= m_threads.count())
    {
        return;
    }

    m_threads[index]->stop();
    qDebug() << "Download index of " << index << " error: " << errorString;
}