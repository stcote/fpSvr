#ifndef FPDB_H
#define FPDB_H

#include <QObject>
#include <QSql>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>


//**********************************************************
//********************* Table Names ************************
//**********************************************************
const QString WeightTableName   = "tbl_Weight";

//**********************************************************
//********************* Field Names ************************
//**********************************************************
const QString ID_Field     = "Rec_Id";
const QString Fam_ID_Field = "Fam_Id";
const QString Weight_Field = "Weight";
const QString Date_Field   = "Date";
const QString Name_Field   = "Name";


//**********************************************************
//********************** Bind Names ************************
//**********************************************************
const QString ID_Bind     = ":" + ID_Field;
const QString Fam_ID_Bind = ":" + Fam_ID_Field;
const QString Weight_Bind = ":" + Weight_Field;
const QString Date_Bind   = ":" + Date_Field;
const QString Name_Bind   = ":" + Name_Field;

//***********************************************************
//********************* Column Names ************************
//***********************************************************
const int ID_Idx     = 0;
const int Fam_ID_Idx = 1;
const int Weight_Idx = 2;
const int Date_Idx   = 3;
const int Name_Idx   = 4;


//*****************************************************************************
//*****************************************************************************
/**
 * @brief The FPDB class
 */
//*****************************************************************************
class FPDB : public QObject
{
    Q_OBJECT

public:

    //*** constructor ***
    explicit FPDB( QString driver, QString dsn, QString label, bool isLocal, QObject *parent = nullptr);

    //*** destructor ***
    ~FPDB();

    //*** if TRUE, database is opened and ready ***
    bool isReady() { return isReady_; }

    //*** returns string describing the last error ***
    QString lastError() { return lastError_; }

    //*** adds a record ***
    bool addRecord( qint32 famId, float weight );
    bool addRecord( qint32 famId, float weight, qint64 date, QString name );

    //*** gets a string with the statistics for the day ***
    QString getTodaysStatistics();


private:

    //*** sets up database access ***
    void setup();

    //*** create the database tables ***
    bool createDatabase();

    bool isLocal_;
    bool isReady_;

    //*** driver and DSN for database ***
    QString driver_;
    QString dsn_;
    QString label_;

    //*** last SQL error ***
    QString lastError_;

    //*** database ***
    QSqlDatabase db_;

    //*** 'Weight' table model ***
    QSqlTableModel *weightTbl_;

    //*** 'prepared' insert query
    QSqlQuery *insertQry_;

    //*** next unique record ID ***
    int nextRecID_;
};

#endif // FPDB_H
