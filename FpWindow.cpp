#include "FpWindow.h"
#include "ui_FpWindow.h"
#include <QLocalServer>
#include <QLocalSocket>
#include <QDate>
#include <QDebug>

const QString FP_PIPE_NAME = "FP_Pipe";

const quint16 SCALE_PORT = 29456;

const int NAME_MAX = 127;

typedef struct
{
    int key;
    char name[NAME_MAX+1];
    int  numItems;
    qint64 day;
} t_CheckIn;

const int CHECKIN_SIZE = sizeof( t_CheckIn );


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

    //*** create new checkin server ***
    svr_ = new QLocalServer( this );

    //*** connect to needed slots ***
    connect( svr_, SIGNAL(newConnection()), SLOT(handleConnection()) );


    //*** start listening for connections ***
    if ( !svr_->listen( FP_PIPE_NAME ) )
    {
        ui->textOut->append( "Error listening for connection!!!" );
    }

    goodIcon_ = QIcon(":/images/good.png");
    badIcon_  = QIcon(":/images/bad.png");

    createActions();
    createTrayIcon();

    connect( trayIcon_, &QSystemTrayIcon::activated, this, &FpWindow::iconActivated);

    showGoodIcon();
    ui->statusLbl->setPixmap( QPixmap( ":/images/good.png" ) );

    trayIcon_->show();

    showMinimized();

    setWindowTitle( "Checkin Server" );
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
