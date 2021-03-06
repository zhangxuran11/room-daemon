#include "mytask.h"
#include <QNetworkInterface>
#include<QDateTime>
#include<QFile>
static int debug = 0;
static QString getCarIP();
static QByteArray readLineFromFile(const QString& fileName,int lineNo);
static bool recvCarrierHeart = false;
MyTask::MyTask(QObject *parent) : QObject(parent)
{
    qDebug("room-daemon v0.6");
    if(QFile::exists("/daemon.txt"))
    {
        QFile file("/daemon.txt");
        if(file.size() > 10*1024*1024)
        {
            QFile::remove("/daemon.txt");
        }
    }
    ztpm = new ZTPManager(6600,QHostAddress("224.102.228.41"));
    connect(ztpm,SIGNAL(readyRead()),this,SLOT(OnReadZTP()));
    connect(&timer,SIGNAL(timeout()),this,SLOT(OnTimeout()));
    timer.setSingleShot(true);
    timer.start(15000);

    ztpmCarrierHeart = new ZTPManager(8317,QHostAddress("224.102.228.40"));
    connect(ztpmCarrierHeart,SIGNAL(readyRead()),this,SLOT(OnRecvCarrierHeart()));
    timerCheckExist.setSingleShot(false);
    connect(&timerCheckExist,SIGNAL(timeout()),this,SLOT(OnCheckExist()));
}
void MyTask::OnReadZTP()
{
    ZTPprotocol ztp;
    ztpm->getOneZtp(ztp);
//    qDebug("daemon recv >>>");
//    ztp.print();
//    qDebug("daemon recv <<<");
    if(ztp.getPara("T") == "Heart" && ztp.getPara("CarIP") == getCarIP())
    {
        timer.stop();
        timer.start(15000);
    }
}
void MyTask::OnTimeout()
{
    char buf[50];
    memset(buf,0,50);
    sprintf(buf,"echo dev dead date : %s >> /daemon.txt",QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLatin1().data());
    system(buf);

    system("ps | grep [R]oomMedia | awk '{print $1}'|  xargs kill -9");
    //system("/appbin/RoomMedia  1>/dev/null 2>&1 &");
    system("/appbin/RoomMedia    &");
}


QString getCarIP()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach(QHostAddress address,list)
    {
        QString ip = address.toString();
        if(ip.startsWith("192.168."))
        {
            return ip;
        }
    }
    return "";
}
void MyTask::OnRecvCarrierHeart()
{
    ZTPprotocol ztp;
    QString printCmd;
    ztpmCarrierHeart->getOneZtp(ztp);
    if(ztp.getPara("T") != "CarrierHeart")
        return;
    if(recvCarrierHeart == true)
    {
        if(debug){
            printCmd = QString("echo recv CarrierHeart and recvCarrierHeart = true ") +"  : "+QDateTime::currentDateTime().toString()+" >> /carrier_record.txt";
            system(printCmd.toAscii().data());
        }
        return;
    }
    if(debug){
        printCmd = QString("echo recv CarrierHeart and recvCarrierHeart = false ") +"  : "+QDateTime::currentDateTime().toString()+" >> /carrier_record.txt";
        system(printCmd.toAscii().data());
    }
    recvCarrierHeart = true;
    timerCheckExist.start(13000);
}

void MyTask::OnCheckExist()
{
    QString printCmd;
    if(QFile::exists("/carrier_record.txt"))
    {
        QFile file("/carrier_record.txt");
        if(file.size() > 10*1024*1024)
        {
            QFile::remove("/carrier_record.txt");
        }
    }
    recvCarrierHeart = false;
    timerCheckExist.stop();
    if(QFile::exists("/tmp/DevExist"))
    {
        if(debug){
            printCmd = QString("echo OnCheckExist and has DevExist -- rm") +"  : "+QDateTime::currentDateTime().toString()+" >> /carrier_record.txt";
            system(printCmd.toAscii().data());
        }
        QFile::remove("/tmp/DevExist");
    }
    else
    {
        if(debug){
            printCmd = QString("echo OnCheckExist and has not DevExist -- restart") +"  : "+QDateTime::currentDateTime().toString()+" >> /carrier_record.txt";
            system(printCmd.toAscii().data());
        }
        char buf[50];
        memset(buf,0,50);
        sprintf(buf,"echo  err:dev offline ,date : %s >> /daemon.txt",QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLatin1().data());
        system(buf);

        system("ps | grep [R]oomMedia | awk '{print $1}'|  xargs kill -9");
        system("/appbin/RoomMedia   &");
    }
}
QByteArray readLineFromFile(const QString& fileName,int lineNo)
{
    QFile f(fileName);
    QByteArray line;
    f.open(QFile::ReadOnly);
    while(!f.atEnd()&&lineNo--)
        line = f.readLine();
    if (f.atEnd() && lineNo > 0)
        line = "";
    else
        line = line;
    f.close();
    return line;
}
int getCarID()
{
    QString ip = readLineFromFile("/etc/myip.txt",1);
    if(ip.startsWith("192.168."))
    {
        return ip.split(QChar('.'))[2].toInt();
    }
    else
        return 0;
}
