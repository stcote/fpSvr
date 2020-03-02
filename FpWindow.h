#ifndef FPWINDOW_H
#define FPWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

namespace Ui {
class FpWindow;
}

class QLocalServer;



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


private:

    void createActions();
    void createTrayIcon();

    void showGoodIcon();
    void showBadIcon();


    Ui::FpWindow *ui;

    QLocalServer *svr_;

    QAction *showWeightAction_;
    QAction *showAction_;
    QAction *hideAction_;

    QSystemTrayIcon *trayIcon_;
    QMenu *trayIconMenu_;

    QIcon goodIcon_;
    QIcon badIcon_;

};

#endif // FPWINDOW_H
