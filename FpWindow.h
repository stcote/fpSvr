#ifndef FPWINDOW_H
#define FPWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QHostAddress>
#include <QTimer>

namespace Ui {
class FpWindow;
}

class QLocalServer;
class QTcpSocket;
class FPDB;

const int NAME_MAX = 127;

typedef struct
{
    int key;
    char name[NAME_MAX+1];
    int  numItems;
    qint64 day;
} t_CheckIn;

const int CHECKIN_SIZE = sizeof( t_CheckIn );


typedef struct
{
    quint32 magic;
    quint32 size;
    quint32 type;
    int     key;
    float   weight;
    qint64  day;
} t_WeightReport;

const int WEIGHT_SIZE = sizeof( t_WeightReport );

const int MAGIC_VAL = 0x3e3e3e3e;

const int WEIGHT_REPORT_SIZE = sizeof( t_WeightReport );
const int WEIGHT_SIZE_FIELD = WEIGHT_REPORT_SIZE - ( 2 * sizeof(quint32) );
const int WEIGHT_REPORT_TYPE = 0x0001;



//*****************************************************************************
//*****************************************************************************
/**
 * @brief The FpWindow class
 */
//*****************************************************************************
class FpWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FpWindow(QWidget *parent = nullptr);
    ~FpWindow();

private slots:

    void iconActivated( QSystemTrayIcon::ActivationReason reason );
    void handleShowWeight();

    void handleConnection();
    void handleRead();

    //********************************************************************************
    //********************************************************************************
    /**
     * Closes the socket if it is open and starts an asynchronous connection attempt. On timeout, checkConnectionFailed() will be called.
     *
     * If we are already trying to connect, this function does nothing.
     */
    //********************************************************************************
    void attemptReconnect();


    //********************************************************************************
    //********************************************************************************
    /**
     * Called after the timeout is reached after attemptReconnect() is called. If the socket is still closed, it is assumed that an error occurred
     * and attempts a reconnect. If the socket is open, nothing more happens.
     */
    //********************************************************************************
    void checkConnectionFailed();


    //********************************************************************************
    //********************************************************************************
    /**
     * Occurs when the tcp socket connects. Sends each message that has been set.
     */
    //********************************************************************************
    void handleTcpConnected();


    //********************************************************************************
    //********************************************************************************
    /**
     * Occurs when the tcp socket disconnects. Attempts a reconnect.
     */
    //********************************************************************************
    void handleTcpDisconnected();


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
    void handleTcpError( QAbstractSocket::SocketError e );


    void handleDataIn();

private:

    void createActions();
    void createTrayIcon();

    void showGoodIcon();
    void showBadIcon();

    void setupNetworking();

    void setupDatabase();



    Ui::FpWindow *ui;

    //*** Windows 'Named Pipe' server ***
    QLocalServer *svr_;

    //*** client socket to talk to scale server ***
    QTcpSocket *scaleSock_;

    QHostAddress scaleAddr_;
    quint16      scalePort_;

    bool attemptingConnect_;   // True if we are attempting to connect, false if we are connected or disconnected
    bool isConnected_;         // True if we are connected to the TCP server
    int tmOutCnt_;

    //*** menu actions ***
    QAction *showWeightAction_;
    QAction *showAction_;
    QAction *hideAction_;

    //*** tray icon ***
    QSystemTrayIcon *trayIcon_;

    //*** menu for tray icon ***
    QMenu *trayIconMenu_;

    //*** Tray icons ***
    QIcon goodIcon_;
    QIcon badIcon_;

    QHash<int,QString> keyToName_;

    FPDB *fpDB_;
    FPDB *localDB_;
};

#endif // FPWINDOW_H
