/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gr�fics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/

#include <QProgressDialog>
#include <QDate>

#include "cachelayer.h"
#include "cachepacs.h"
#include "status.h"
#include "studymask.h"
#include "cachepool.h"
#include "starviewersettings.h"
#include "logging.h"
#include "cachestudydal.h"

namespace udg {

CacheLayer::CacheLayer( QObject *parent )
 : QObject( parent )
{
}

Status CacheLayer::clearCache()
{
    CacheStudyDAL cacheStudyDAL;
    StudyMask studyMask;
    StudyList studyList;
    Study study;
    Status state;
    CachePool pool;
    unsigned usedSpaceInit , usedSpace;
    int deletedSpace = 0;
    
    pool.getPoolUsedSpace( usedSpaceInit );
    QProgressDialog *progress;
    progress = new QProgressDialog( tr( "Clearing cache..." ) , "" , 0 , usedSpaceInit );
    progress->setMinimumDuration( 0 );
    progress->setCancelButton( 0 );

    state = cacheStudyDAL.queryStudy( studyMask , studyList );
    
    studyList.firstStudy();
    while ( !studyList.end() && state.good() )
    {
        study = studyList.getStudy();
        state = cacheStudyDAL.delStudy( study.getStudyUID() ); //indiquem l'estudi a esborrar
        
        if ( !state.good() ) state = pool.getPoolUsedSpace( usedSpace ); 
        deletedSpace = usedSpaceInit - usedSpace;//calculem l'espai esborrat
        progress->setValue( deletedSpace );
        progress->repaint();
        studyList.nextStudy();
    }
    
    progress->close();
    
    if ( !state.good() )
    {
        return state;
    }
    else
    {        
        return state.setStatus( CORRECT );
    }
}

Status CacheLayer::deleteOldStudies()
{
    QDate today,lastTimeViewedMinimum;
    StarviewerSettings settings;
    StudyList studyList;
    Status state;
    Study study;
    int comptador = 0;
    QString logMessage , numberOfDeletedStudies;    
    CacheStudyDAL cacheStudyDAL;
   
    today = today.currentDate();
    //calculem fins a quin dia conservarem els estudis
    //de la data del dia restem el par�metre definit per l'usuari, que estableix quants dies pot estar un estudi sense ser visualitzat
    lastTimeViewedMinimum = today.addDays( - settings.getMaximumDaysNotViewedStudy().toInt( NULL , 10 ) );
    
    //cerquem els estudis que no han estat visualitzats, en una data inferior a la passada per par�metre
    state = cacheStudyDAL.queryOldStudies( lastTimeViewedMinimum.toString( "yyyyMMdd" ).toAscii().constData() , studyList );
    studyList.firstStudy();
    
    QProgressDialog *progress;
    progress = new QProgressDialog( tr( "Clearing old studies..." ) , "" , 0 , studyList.count() );
    progress->setMinimumDuration( 0 );
	progress->setCancelButton( 0 );
    
    while ( state.good() && !studyList.end() )
    {
        study = studyList.getStudy();
        state = cacheStudyDAL.delStudy( study.getStudyUID() ); //indiquem l'estudi a esborrar   
        studyList.nextStudy();
        comptador++;
        progress->setValue( comptador );
        progress->repaint();
    }
   
	logMessage = "S'han esborrat " + numberOfDeletedStudies.setNum( comptador , 10 ) + " estudis vells";
	INFO_LOG( logMessage.toAscii().constData() );
    progress->close();
    
    if ( !state.good() )
    {
        return state;
    }
    else return state.setStatus( CORRECT );
}

Status CacheLayer::deleteOldStudies( int MbytesToErase )
{
    QDate maxDate;
    StarviewerSettings settings;
    StudyList studyList;
    Status state;
    Study study;
    CachePool pool;
    unsigned int usedSpaceInit = 0 , usedSpace = 0;
    int deletedSpace = 0;
    QString logMessage , numberOfDeletedStudies;  
    CacheStudyDAL cacheStudyDAL;  
         
    maxDate = maxDate.currentDate();
    maxDate = maxDate.addDays( 1 ); //com que la funcio queryOldStudies, retorna els que no ha estat visualitzats en una data inferior a la passada per parametre, per incloure els del mateix dia a la llista que retorna, hi sumem un dia
    
    state  = pool.getPoolUsedSpace( usedSpaceInit );
    
    if ( !state.good() ) return state;
    
    //cerquem els estudis que no han estat visualitzats, en una data inferior a la passada per par�metre, retorna la llista ordenada per data i hora de l'ultima visualitzacio, ordenada ascendentment 
    state = cacheStudyDAL.queryOldStudies( maxDate.toString("yyyyMMdd").toAscii().constData() , studyList );
    studyList.firstStudy();
    
    //esborrem estudis fins que la llista estigui buida o haguem alliberat l'espai en Mb passat per parametre
    while ( state.good() && !studyList.end() && MbytesToErase >= deletedSpace )
    {
        study = studyList.getStudy();
        state = cacheStudyDAL.delStudy( study.getStudyUID() ); //indiquem l'estudi a esborrar   
        pool.getPoolUsedSpace( usedSpace );//calculem l'espai esborrat
        deletedSpace = usedSpaceInit - usedSpace;
        studyList.nextStudy();
    }

    if ( !state.good() )
    {
        return state;
    }
    else return state.setStatus( CORRECT );
}

CacheLayer::~CacheLayer()
{
}

}
