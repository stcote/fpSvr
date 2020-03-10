#include "FPDB.h"

#include <QSqlError>
#include <QDate>
#include <QSqlRecord>

//*****************************************************************************
//*****************************************************************************
/**
 * @brief FPDB::FPDB
 * @param parent
 */
//*****************************************************************************
FPDB::FPDB( QString driver, QString dsn, QString label, bool isLocal, QObject *parent ) : QObject(parent)
{
    driver_    = driver;
    dsn_       = dsn;
    isLocal_   = isLocal;
    isReady_   = false;
    lastError_ = "No error";
    weightTbl_ = Q_NULLPTR;

    nextRecID_ = 1;

    //*** setup access to the database ***
    setup();
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FPDB::setup
 */
//*****************************************************************************
void FPDB::setup()
{
    //*** add database connection ***
    db_ = QSqlDatabase::addDatabase( driver_, label_ );

    //*** set name (DSN) ***
    db_.setDatabaseName( dsn_ );

    //*** open the database ***
    if ( !db_.open() )
    {
        //*** error opening database ***
        lastError_ = db_.lastError().text();
        return;
    }
    isReady_ = true;

    //*** if local, check if we need to set up the database ***
    if ( isLocal_ && db_.tables().size() == 0 )
    {
        isReady_ = createDatabase();
    }

    if ( !isReady_ ) return;

    //*** create the table model ***
    weightTbl_ = new QSqlTableModel( this, db_ );
    weightTbl_->setTable( WeightTableName );
    weightTbl_->setEditStrategy( QSqlTableModel::OnManualSubmit );

    weightTbl_->select();

    //*** determine next unique record ID ***
    nextRecID_ = weightTbl_->rowCount() + 1;

    //*** prepare the insert query ***
    QString insertStr;
    if ( isLocal_ )
    {
        //*** for local SQLITE database ***
        insertStr = QString( "INSERT INTO %1 ( %2, %3, %4, %5 ) " )
                .arg(WeightTableName)
                .arg(ID_Field)
                .arg(Fam_ID_Field)
                .arg(Weight_Field)
                .arg(Date_Field);
        insertStr += QString( "VALUES ( %1, %2, %3, %4 )" )
                .arg(ID_Bind)
                .arg(Fam_ID_Bind)
                .arg(Weight_Bind)
                .arg(Date_Bind);
    }
    else
    {
        //*** for Access weights database ***
        insertStr = QString( "INSERT INTO %1 ( %2, %3, %4 ) " )
                .arg(WeightTableName)
                .arg(ID_Field)
                .arg(Fam_ID_Field)
                .arg(Weight_Field);
        insertStr += QString( "VALUES ( %1, %2, %3 )" )
                .arg(ID_Bind)
                .arg(Fam_ID_Bind)
                .arg(Weight_Bind);
    }

    //*** create a 'permanent' insert query ***
    insertQry_ = new QSqlQuery( db_ );

    //*** prepare the insert query ***
    insertQry_->prepare( insertStr );
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FPDB::createDatabase
 */
//*****************************************************************************
bool FPDB::createDatabase()
{
QSqlQuery query(db_);       // query associated with this database
QString createStr;

    //*** sanity check ***
    if ( !db_.isOpen() ) return false;

    //*** build string to create the weights table ***
    createStr = QString( "create table %1 ( %2 integer primary key, %3 integer, %4 integer, %5 integer ) ")
                .arg(WeightTableName)
                .arg(ID_Field)
                .arg(Fam_ID_Field)
                .arg(Weight_Field)
                .arg(Date_Field);

    //*** create table that holds weights ***
    if ( !query.exec( createStr ) )
    {
        lastError_ = query.lastError().text();
        return false;
    }

    return true;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FPDB::addRecord
 * @param id
 * @param weight
 * @param data
 * @return
 */
//*****************************************************************************
bool FPDB::addRecord( qint32 famId, float weight, qint64 date )
{
bool rtn = true;

    //*** ID for this record ***
    qint32 id = nextRecID_++;

    //*** bind values to query ***
    insertQry_->bindValue( ID_Bind,    id );
    insertQry_->bindValue( Fam_ID_Bind,  famId );
    insertQry_->bindValue( Weight_Bind,  weight );
    if ( isLocal_ )
    {
        insertQry_->bindValue( Date_Bind, date );
    }

    //*** execute the query ***
    if ( !insertQry_->exec() )
    {
        lastError_ = insertQry_->lastError().text();
        rtn = false;
    }

    return rtn;
}


//*****************************************************************************
//*****************************************************************************
/**
 * @brief FPDB::getTodaysStatistics
 * @return
 */
//*****************************************************************************
QString FPDB::getTodaysStatistics()
{
float totalWeight = 0;
int   numEntries = 0;
QString rtn;

    //*** get todays date ***
    qint64 date = QDate::currentDate().toJulianDay();

    //*** set filter ***
    weightTbl_->setFilter( QString("date = %1" ).arg(date) );

    //*** do the query ***
    weightTbl_->select();

    numEntries = weightTbl_->rowCount();

    if ( numEntries > 0 )
    {
        //*** process all records ***
        for ( int row=0; row<numEntries; row++ )
        {
            totalWeight += weightTbl_->record( row ).value( Weight_Idx ).toFloat();
        }

        rtn.sprintf( "Count: %d   Weight %.1f lbs" );
    }
    else
    {
        rtn = "No entries today!!!";
    }

    return rtn;
}
