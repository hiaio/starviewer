/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gr�fics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGMULTQUERYSTUDY_H
#define UDGMULTQUERYSTUDY_H

#include "status.h"
#include "pacsparameters.h"
#include "qquerystudythread.h"
#include "pacslist.h"
#include "studylistsingleton.h"
#include "pacsparameters.h"
#include "studymask.h"
#include <QObject>

namespace udg {

/**
	@author Grup de Gr�fics de Girona  ( GGG ) <vismed@ima.udg.es>
*/
class MultQueryStudy : public QObject
{
Q_OBJECT
public:
    MultQueryStudy(QObject *parent = 0);
    
    ~MultQueryStudy();
   
    void setMask(StudyMask);
    void setPacsList(PacsList);
    
    Status StartQueries();
    
    StudyListSingleton* getStudyList();

signals :
    void finish();

public slots :

    void threadFinished();


private :

    StudyMask m_searchMask;
    
    StudyListSingleton* m_studyListSingleton;
    PacsList m_pacsList;
    int m_maxThreads;
    
    void QueryStudies();

};

}

#endif
