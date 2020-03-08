#include "FpWindow.h"
#include "ui_FpWindow.h"
#include <QLocalServer>
#include <QLocalSocket>
#include <QTcpSocket>
#include <QDate>
#include <QDebug>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>


const QString FP_PIPE_NAME = "FP_Pipe";

const quint16 SCALE_PORT = 29456;
const QString SCALE_ADDR = "10.0.1.1";
//const QString SCALE_ADDR = "127.0.0.1";

const QString DB_DSN_NAME = "FP_WEIGHTS";

const int NAME_MAX = 127;

typedef struct
{
    int key;
    char name[NAME_MAX+1];
    int  numItems;
    qint64 day;
} t_CheckIn;

const int CHECKIN_SIZE = sizeof( t_CheckIn );

const int CONNECT_TIMEOUT_MS = 1000;

//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::FpWindow
 * @param parent
 */
//*****************************************************************************
FpWindow::FpWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FpWindow)
{
    ui->setupUi(this);

    attemptingConnect_ = false;
    isConnected_       = false;
    tmOutCnt_          = 0;

    //*** create new checkin server ***
    svr_ = new QLocalServer( this );

    //*** connect to needed slots ***
    connect( svr_, SIGNAL(newConnection()), SLOT(handleConnection()) );


    //*** start listening for connections ***
    if ( !svr_->listen( FP_PIPE_NAME ) )
    {
        ui->textOut->append( "Error listening for connection!!!" );
    }

    //*** create the icons we need ***
    goodIcon_ = QIcon(":/images/good.png");
    badIcon_  = QIcon(":/images/bad.png");

    //*** set up the tray interface ***
    createActions();
    createTrayIcon();

    //*** handle activation ***
    connect( trayIcon_, &QSystemTrayIcon::activated, this, &FpWindow::iconActivated);

    //*** show bad icon until we connect ***
    showBadIcon();

    //*** show the tray (icon) ***
    trayIcon_->show();

    //*** set icon for the program ***
    setWindowIcon( goodIcon_ );

    //*** start minimized (to tray) ***
    showMinimized();

    //*** in one second, hide it - can't do right away ***
    QTimer::singleShot( 1000, [=]() { this->hide(); } );

    //*** Title for the application window ***
    setWindowTitle( "Checkin Server" );

    //*** setupDatabase ***
    setupDatabase();

    //*** start trying to connect to scale server ***
    setupNetworking();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::~FpWindow
 */
//*****************************************************************************
FpWindow::~FpWindow()
{
    delete ui;

    delete trayIcon_;
    delete trayIconMenu_;

    delete svr_;
    delete scaleSock_;

}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::iconActivated
 * @param reason
 */
//*****************************************************************************
void FpWindow::iconActivated( QSystemTrayIcon::ActivationReason reason )
{
    switch (reason)
    {
    case QSystemTrayIcon::Trigger:
        qDebug() << "Trigger";
        break;

    case QSystemTrayIcon::DoubleClick:
        qDebug() << "DoubleClick";
        break;

    case QSystemTrayIcon::MiddleClick:
        qDebug() << "MiddleClick";
        showBadIcon();
        break;

    default:
        ;
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::handleShowWeight
 */
//*****************************************************************************
void FpWindow::handleShowWeight()
{
    qDebug() << "Show weight";
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::handleConnection
 */
//*****************************************************************************
void FpWindow::handleConnection()
{
    qDebug() << "Connection...";

    //*** get the pending connection ***
    QLocalSocket *client = svr_->nextPendingConnection();

    //*** delete on disconnect ***
    connect(client, &QLocalSocket::disconnected, client, &QLocalSocket::deleteLater );

    //*** wait for incoming data ***
    connect( client, SIGNAL(readyRead()), SLOT(handleRead()) );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::handleRead
 */
//*****************************************************************************
void FpWindow::handleRead()
{
t_CheckIn ci;

    qDebug() << "Read...";

    QLocalSocket *client = (QLocalSocket*)sender();

    if ( client->bytesAvailable() == CHECKIN_SIZE )
    {
        client->read( (char*)&ci, CHECKIN_SIZE );

        QString day = QDate::fromJulianDay( ci.day ).toString( "MM/dd/yyyy" );

        QString buf = QString( "key: %1  name: %2  items: %3  day: %4" )
                .arg( ci.key ).arg( ci.name ).arg( ci.numItems ).arg( day );

        ui->textOut->append( buf );

        //*** send to scale server if connected ***
        if ( isConnected_ )
        {
            scaleSock_->write( (const char*)&ci, CHECKIN_SIZE );
        }
    }
    else
    {
        ui->textOut->append( "Data size error!!!" );
    }
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::createActions
 */
//*****************************************************************************
void FpWindow::createActions()
{
    hideAction_ = new QAction(tr("&Hide"), this);
    connect( hideAction_, &QAction::triggered, this, &QWidget::hide);

    showAction_ = new QAction(tr("&Show"), this);
    connect( showAction_, &QAction::triggered, this, &QWidget::showNormal);

    showWeightAction_ = new QAction(tr("Show &Weight Total"), this);
    connect(showWeightAction_, &QAction::triggered, this, &FpWindow::handleShowWeight );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::createTrayIcon
 */
//*****************************************************************************
void FpWindow::createTrayIcon()
{
    trayIconMenu_ = new QMenu(this);
    trayIconMenu_->addAction(showAction_);
    trayIconMenu_->addAction(hideAction_);
    trayIconMenu_->addAction(showWeightAction_);

    trayIcon_ = new QSystemTrayIcon(this);
    trayIcon_->setContextMenu( trayIconMenu_ );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::showGoodIcon
 */
//*****************************************************************************
void FpWindow::showGoodIcon()
{
    trayIcon_->setIcon( goodIcon_ );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::showBadIcon
 */
//*****************************************************************************
void FpWindow::showBadIcon()
{
    trayIcon_->setIcon( badIcon_ );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::setupNetworking
 */
//*****************************************************************************
void FpWindow::setupNetworking()
{
    scaleSock_ = new QTcpSocket( this );

    //*** set up connection endpoint ***
    scalePort_ = SCALE_PORT;
    scaleAddr_.setAddress( SCALE_ADDR );

    connect(scaleSock_, SIGNAL(connected()), SLOT(handleTcpConnected()));
    connect(scaleSock_, SIGNAL(disconnected()), SLOT(handleTcpDisconnected()));
    connect(scaleSock_, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(handleTcpError(QAbstractSocket::SocketError)));

    //*** start attempting to connect ***
    attemptReconnect();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FpWindow::setupDatabase
 */
//*****************************************************************************
void FpWindow::setupDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC3");
    db.setDatabaseName( DB_DSN_NAME );

    if(db.open())
    {
        qDebug() << "oK";
        qDebug() << db.tables();
    }
    else
      qDebug() << db.lastError().text();

    db.close();
}


//********************************************************************************
//********************************************************************************
/**
 * Closes the socket if it is open and starts an asynchronous connection attempt. On timeout, checkConnectionFailed() will be called.
 *
 * If we are already trying to connect, this function does nothing.
 */
//********************************************************************************
void FpWindow::attemptReconnect()
{
    // only continue if we're not already connecting
    if (!attemptingConnect_)
    {
       if (scaleSock_->isOpen())
       {
          scaleSock_->close();
       }

       // attempt the connect. we will check later if it succeeded.
       isConnected_ = false;
       attemptingConnect_ = true;
       scaleSock_->connectToHost( scaleAddr_, scalePort_, QAbstractSocket::ReadWrite);


       // after the timeout, check if we have connected. If the connection succeeds, connected() will handle going forward and this timeout will do nothing
       QTimer::singleShot(CONNECT_TIMEOUT_MS, this, SLOT(checkConnectionFailed()));
    }
}


//********************************************************************************
//********************************************************************************
/**
 * Called after the timeout is reached after attemptReconnect() is called. If the socket is still closed, it is assumed that an error occurred
 * and attempts a reconnect. If the socket is open, nothing more happens.
 */
//********************************************************************************
void FpWindow::checkConnectionFailed()
{
    if (!isConnected_)
    {
       // connection failed, try again
       tmOutCnt_++;
       if ( tmOutCnt_ % 10 == 0 )
       {
          ui->textOut->append( "Unable to connect, retrying..." );
       }

       attemptingConnect_ = false;

       //*** stop connect ***
       scaleSock_->abort();

       //*** try connect again ***
       attemptReconnect();
    }
}


//********************************************************************************
//********************************************************************************
/**
 * Occurs when the tcp socket connects. Sends each message that has been set.
 */
//********************************************************************************
void FpWindow::handleTcpConnected()
{
    ui->textOut->append( "Connected..." );

    ui->statusLbl->setText( "Connected" );

    isConnected_ = true;
    attemptingConnect_ = false;
    tmOutCnt_ = 0;

    showGoodIcon();
}


//********************************************************************************
//********************************************************************************
/**
 * Occurs when the tcp socket disconnects. Attempts a reconnect.
 */
//********************************************************************************
void FpWindow::handleTcpDisconnected()
{
    ui->textOut->append( "Server disconnected, reconnecting..." );

    ui->statusLbl->setText( "Disconnected" );

    attemptingConnect_ = false;
    attemptReconnect();

    showBadIcon();
}


//********************************************************************************
//********************************************************************************
/**
 * Occurs if an error is thrown by the TCP socket. Logs the error.
 *
 * This does not attempt a reconnect as many of the error types happen immediately on connectToHost and would result in an infinite loop.
 * If an error occurs and the socket disconnects, a reconnect will be attempted the next time setInfo is called.
 *
 * @param e  The type of error that occurred
 */
//********************************************************************************
void FpWindow::handleTcpError( QAbstractSocket::SocketError e )
{
    if ( e != QAbstractSocket::ConnectionRefusedError )
    {
        ui->textOut->append( "TCP Error: " + scaleSock_->errorString() );
        qDebug() << "Error (" << e << "): " << scaleSock_->errorString();
    }
}
