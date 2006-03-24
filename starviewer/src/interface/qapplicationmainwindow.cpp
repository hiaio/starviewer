/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gr�fics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/

// Qt
#include <QAction>
#include <QSignalMapper>
#include <QFileDialog>
#include <QSettings>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QCloseEvent>
#include <QMessageBox>
#include <QWidget>
#include <QFileInfo> //Per m_workingDirectory
#include <QCursor>
#include <QProgressDialog>

// els nostres widgets/elements de la plataforma
#include "qapplicationmainwindow.h"
#include "volumerepository.h"
#include "identifier.h"
#include "volume.h"
#include "input.h"
#include "output.h"
#include "extensionhandler.h"
#include "extensionworkspace.h"

// Mini - aplicacions
#include "qtabaxisview.h"
#include "queryscreen.h" // temporal!
#include "cacheinstallation.h"

namespace udg{

QApplicationMainWindow::QApplicationMainWindow( QWidget *parent, const char *name )
    : QMainWindow( parent )
{
    this->setAttribute( Qt::WA_DeleteOnClose );
    this->setObjectName( name );
    m_extensionWorkspace = new ExtensionWorkspace( this );
    setCentralWidget( m_extensionWorkspace );
    CacheInstallation cacheInstallation;
    m_extensionHandler = new ExtensionHandler( this );
    
    cacheInstallation.checkInstallation();
    m_queryScreen = new QueryScreen( 0 );
    connect( m_queryScreen , SIGNAL(viewStudy(StudyVolum)) , this , SLOT(viewStudy(StudyVolum)) );
    // ------------------------------------------------------------------------------------
    // aqu� creem el repositori de volums i l'objecte input per poder accedir als arxius
    // ------------------------------------------------------------------------------------
     
    m_volumeRepository = udg::VolumeRepository::getRepository();
    m_inputReader = new udg::Input;
    m_outputWriter = new udg::Output;
    // aquesta barra de progr�s �s provisional. Aix� anir� m�s lligat dins de les mini-apps
    m_progressDialog = new QProgressDialog( tr("Loading image data...") , tr("Abort") , 0 ,  100 , this );
    m_progressDialog->setMinimumDuration( 0 );
    m_progressDialog->setCaption( tr("Caption") );
    m_progressDialog->setAutoClose( TRUE );

    /** \TODO Aqu� podr�em tenir un showProgress per cada tipu d'acci�, la de carregar i la de desar
    ja que el missatge del progress dialog podria ser diferent, per exemple.
    Podr�em tenir un showLoadProgress i un showSaveProgress
    */ 
    connect( m_inputReader , SIGNAL( progress(int) ) , this , SLOT( showProgress(int) ) );
    connect( m_outputWriter , SIGNAL( progress(int) ) , this , SLOT( showProgress(int) ) );
    
    createActions();    
    createMenus();
    createToolBars();
    createStatusBar();
    
    // Llegim les configuracions de l'aplicaci�, estat de la finestra, posicio, �ltims
    // arxius oberts etc amb QSettings
    readSettings();
    // icona de l'aplicaci�
    setIcon( QPixmap(":/images/icon.png") );
    setCaption( tr("StarViewer") );
    m_openFileFilters = tr("MetaIO Images (*.mhd);;DICOM Images (*.dcm);;All Files (*)");
    m_exportFileFilters = tr("JPEG Images (*.jpg);;MetaIO Images (*.mhd);;DICOM Images (*.dcm);;All Files (*)");
    
    m_exportToJpegFilter = tr("JPEG Images (*.jpg)");
    m_exportToMetaIOFilter = tr("MetaIO Images (*.mhd)");
    m_exportToPngFilter = tr("PNG Images (*.png)");
    m_exportToTiffFilter = tr("TIFF Images (*.tiff)");
    m_exportToBmpFilter = tr("BMP Images (*.bmp)");
    m_exportToDicomFilter = tr("DICOM Images (*.dcm)");

    m_modified = false;
    m_self = this;                                    
    
    emit containsVolume( FALSE );
}

QApplicationMainWindow::~QApplicationMainWindow()
{
    if( !m_volumeID.isNull() )
    {
        m_volumeRepository->removeVolume( m_volumeID );
    }
}

void QApplicationMainWindow::viewStudy( StudyVolum study )
{
    Input *input = new Input;
    SeriesVolum serie;
    
    this->setCursor( QCursor(Qt::WaitCursor) );
    study.firstSerie();
    while ( !study.end() )
    {
        if ( study.getDefaultSeriesUID() == study.getSeriesVolum().getSeriesUID() )
        {
            break;
        }
        study.nextSerie();
    }
    if ( study.end() )
    { 
        //si no l'hem trobat per defecte mostrarem la primera serie
        study.firstSerie();
    }
    
    serie = study.getSeriesVolum();

    input->readSeries( serie.getSeriesPath().c_str() );
    
    udg::Volume *dummyVolume = input->getData();
    m_volumeID = m_volumeRepository->addVolume( dummyVolume );
    m_extensionHandler->setVolumeID( m_volumeID );    
    m_extensionHandler->request( 2 );

    this->setCursor( QCursor(Qt::ArrowCursor) );    

}

void QApplicationMainWindow::createActions()
{
    QSignalMapper* signalMapper = new QSignalMapper( this );
    connect( signalMapper, SIGNAL( mapped(int) ), m_extensionHandler , SLOT( request(int) ) );
    connect( signalMapper, SIGNAL( mapped( const QString) ), m_extensionHandler , SLOT( request(const QString) ) );
    
    m_newAction = new QAction( this );
    m_newAction->setText( tr("&New") );
    m_newAction->setShortcut( tr("Ctrl+N") );
    m_newAction->setStatusTip(tr("Open a new working window") );
    m_newAction->setIcon( QIcon(":/images/new.png") );
    connect( m_newAction , SIGNAL( activated() ), this, SLOT( newFile() ) );

    m_openAction = new QAction( this );
    m_openAction->setText( tr("&Open...") );
    m_openAction->setShortcut( tr("Ctrl+O") );
    m_openAction->setStatusTip(tr("Open an existing volume file"));
    m_openAction->setIcon( QIcon(":/images/open.png") );
    signalMapper->setMapping( m_openAction , 1 );
    signalMapper->setMapping( m_openAction , "Open File" );
    connect( m_openAction , SIGNAL( activated() ) , signalMapper , SLOT( map() ) );
    
    m_pacsAction = new QAction( this );
    m_pacsAction->setText(tr("&PACS...") );
    m_pacsAction->setShortcut( tr("Ctrl+P") );
    m_pacsAction->setStatusTip( tr("Open PACS Query Screen") );
    m_pacsAction->setIcon( QIcon(":/images/find.png") );
    connect( m_pacsAction, SIGNAL( activated() ) , this , SLOT( pacsQueryScreen() ) );

    m_mpr2DAction = new QAction( this );
    m_mpr2DAction->setText( tr("2D &MPR Viewer") );
    m_mpr2DAction->setShortcut( tr("Ctrl+M") );
    m_mpr2DAction->setStatusTip( tr("Open the 2D MPR Application Viewer") );
    signalMapper->setMapping( m_mpr2DAction , 2 );
    signalMapper->setMapping( m_mpr2DAction , "2D MPR" );
    connect( m_mpr2DAction , SIGNAL( activated() ) , signalMapper , SLOT( map() ) );

    m_mpr3DAction = new QAction( this );
    m_mpr3DAction->setText( tr("3D M&PR Viewer") );
    m_mpr3DAction->setShortcut( tr("Ctrl+P") );
    m_mpr3DAction->setStatusTip( tr("Open the 3D MPR Application Viewer") );
    signalMapper->setMapping( m_mpr3DAction , 3 );
    signalMapper->setMapping( m_mpr3DAction , "3D MPR" );
    connect( m_mpr3DAction , SIGNAL( activated() ) , signalMapper , SLOT( map() ) );

    m_mpr3D2DAction = new QAction( this );
    m_mpr3D2DAction->setText( tr("3D-2D MP&R Viewer") );
    m_mpr3D2DAction->setShortcut( tr("Ctrl+R") );
    m_mpr3D2DAction->setStatusTip( tr("Open the 3D-2D MPR Application Viewer") );
    signalMapper->setMapping( m_mpr3D2DAction , 4 );
    signalMapper->setMapping( m_mpr3D2DAction , "3D-2D MPR" );
    connect( m_mpr3D2DAction , SIGNAL( activated() ) , signalMapper , SLOT( map() ) );
    
    m_exportToJpegAction = new QAction( this );
    m_exportToJpegAction->setText(tr("Export to JPEG"));
    m_exportToJpegAction->setShortcut( 0 );
    m_exportToJpegAction->setStatusTip( tr("Export the volume to jpeg format") );
    connect( m_exportToJpegAction , SIGNAL( activated() ) , this , SLOT( exportToJpeg() ) );
    connect( this , SIGNAL( containsVolume(bool) ), m_exportToJpegAction, SLOT( setEnabled(bool) ) );
    
    m_exportToMetaIOAction = new QAction( this );
    m_exportToMetaIOAction->setText(tr("Export to MetaIO"));
    m_exportToMetaIOAction->setShortcut( 0 );
    m_exportToMetaIOAction->setStatusTip( tr("Export the volume to MetaIO format") );
    connect( m_exportToMetaIOAction , SIGNAL( activated() ) , this , SLOT( exportToMetaIO() ) );
    connect( this , SIGNAL( containsVolume(bool) ), m_exportToMetaIOAction, SLOT( setEnabled(bool) ) );
    
    m_exportToPngAction = new QAction( this );
    m_exportToPngAction->setText(tr("Export to PNG"));
    m_exportToPngAction->setShortcut( 0 );
    m_exportToPngAction->setStatusTip( tr("Export the volume to png format") );
    connect( m_exportToPngAction , SIGNAL( activated() ) , this , SLOT( exportToPng() ) );
    connect( this , SIGNAL( containsVolume(bool) ), m_exportToPngAction, SLOT( setEnabled(bool) ) );
    
    m_exportToTiffAction = new QAction( this );
    m_exportToTiffAction->setText(tr("Export to TIFF"));
    m_exportToTiffAction->setShortcut( 0 );
    m_exportToTiffAction->setStatusTip( tr("Export the volume to tiff format") );
    connect( m_exportToTiffAction , SIGNAL( activated() ) , this , SLOT( exportToTiff() ) );
    connect( this , SIGNAL( containsVolume(bool) ), m_exportToTiffAction, SLOT( setEnabled(bool) ) );
    
    m_exportToBmpAction = new QAction( this );
    m_exportToBmpAction->setText(tr("Export to BMP"));
    m_exportToBmpAction->setShortcut( 0 );
    m_exportToBmpAction->setStatusTip( tr("Export the volume to bmp format") );
    connect( m_exportToBmpAction , SIGNAL( activated() ) , this , SLOT( exportToBmp() ) );
    connect( this , SIGNAL( containsVolume(bool) ), m_exportToBmpAction, SLOT( setEnabled(bool) ) );
    
    m_aboutAction = new QAction( this );
    m_aboutAction->setText(tr("&About") );
    m_aboutAction->setShortcut( 0 );
    m_aboutAction->setStatusTip(tr("Show the application's About box"));
    connect(m_aboutAction, SIGNAL(activated()), this, SLOT(about()));

    m_closeAction = new QAction( this );
    m_closeAction->setText( tr("&Close") );
    m_closeAction->setShortcut( tr("Ctrl+W") );
    m_closeAction->setStatusTip(tr("Close the current volume"));
    m_closeAction->setIcon( QIcon(":/images/fileclose.png"));
    connect( m_closeAction, SIGNAL( activated() ), this, SLOT( close() ) );            
    connect( this , SIGNAL( containsVolume(bool) ), m_closeAction, SLOT( setEnabled(bool) ) );
    
    m_exitAction = new QAction( this );
    m_exitAction->setText( tr("E&xit") );
    m_exitAction->setShortcut(tr("Ctrl+Q") );
    m_exitAction->setStatusTip(tr("Exit the application"));
    m_exitAction->setIcon( QIcon(":/images/exit.png") );
    connect(m_exitAction, SIGNAL(activated()), qApp, SLOT(closeAllWindows()));

    for (int i = 0; i < MaxRecentFiles; ++i)
    {
        m_recentFileActions[i] = new QAction( this );
        m_recentFileActions[i]->setVisible( false );
        connect( m_recentFileActions[i], SIGNAL( triggered() ), this, SLOT( openRecentFile() ) );
    }
}

void QApplicationMainWindow::insertApplicationAction( QAction *action , OperationsType operation , bool toToolBar )
{
    // pre: s'han creat els men�s sin� petar�!
    switch( operation )
    {
        case Segmentation:
            action->addTo( m_segmentationMenu );
            if( toToolBar ) m_segmentationToolBar->addAction( action );
        break;
        case Registration:
            action->addTo( m_registrationMenu );
            if( toToolBar ) action->addTo( m_registrationToolBar );
        break;
        case Clustering:
            action->addTo( m_clusteringMenu );
            if( toToolBar ) action->addTo( m_clusteringToolBar );
        break;
        case Color:
            action->addTo( m_colorMenu );
            if( toToolBar ) action->addTo( m_colorToolBar );
        break;
    }
}

void QApplicationMainWindow::exportFile( int type )
{
    switch( type )
    {
    case QApplicationMainWindow::JpegExport:
        exportToJpeg();
    break;
    case QApplicationMainWindow::MetaIOExport:
        exportToMetaIO();
    break;
    case QApplicationMainWindow::PngExport:
        exportToPng();
    break;
    case QApplicationMainWindow::TiffExport:
        exportToTiff();
    break;
    case QApplicationMainWindow::BmpExport:
        exportToBmp();
    break;
    }
}

void QApplicationMainWindow::exportToJpeg( )
{
    QString fileName = QFileDialog::getSaveFileName( this , tr("Choose an image filename") , m_exportWorkingDirectory, m_exportToJpegFilter );
    if ( !fileName.isEmpty() )
    {
//         std::cout << "Extension::" << QFileInfo( fileName ).suffix() << std::endl;
        if( QFileInfo( fileName ).suffix() != "jpg" )
        {
            fileName += ".jpg";
        }
        
        Output *out = new Output();
        // aqu� cladria rec�rrer les llesques per guardar per separat en un fitxer cadascuna
        out->setInput( m_volumeRepository->getVolume( this->getVolumeID() ) );
        out->saveSeries( fileName.toLatin1() );
        m_exportWorkingDirectory = QFileInfo( fileName ).dirPath();
    }

}

void QApplicationMainWindow::exportToPng( )
{
    QString fileName = QFileDialog::getSaveFileName( this , tr("Choose an image filename") , m_exportWorkingDirectory, m_exportToPngFilter );
    if ( !fileName.isEmpty() )
    {
        if( QFileInfo( fileName ).suffix() != "png" )
        {
            fileName += ".png";
        }      
        Output *out = new Output();
        // aqu� cladria rec�rrer les llesques per guardar per separat en un fitxer cadascuna
        out->setInput( m_volumeRepository->getVolume( this->getVolumeID() ) );
        out->saveSeries( fileName.toLatin1() );
        m_exportWorkingDirectory = QFileInfo( fileName ).dirPath();
    }
}

void QApplicationMainWindow::exportToTiff( )
{
    QString fileName = QFileDialog::getSaveFileName( this , tr("Choose an image filename") , m_exportWorkingDirectory, m_exportToTiffFilter );
    
    if ( !fileName.isEmpty() )
    {
        if( QFileInfo( fileName ).suffix() != "tiff" )
        {
            fileName += ".tiff";
        }
        
        Output *out = new Output();
        // aqu� cladria rec�rrer les llesques per guardar per separat en un fitxer cadascuna
        out->setInput( m_volumeRepository->getVolume( this->getVolumeID() ) );
        out->saveSeries( fileName.toLatin1() );
        m_exportWorkingDirectory = QFileInfo( fileName ).dirPath();
    }
}

void QApplicationMainWindow::exportToBmp( )
{
    QString fileName = QFileDialog::getSaveFileName( this , tr("Choose an image filename") , m_exportWorkingDirectory, m_exportToBmpFilter );
            
    if ( !fileName.isEmpty() )
    {
        if( QFileInfo( fileName ).suffix() != "bmp" )
        {
            fileName += ".bmp";
        }
        
        Output *out = new Output();
        // aqu� caldria rec�rrer les llesques per guardar per separat en un fitxer cadascuna
        out->setInput( m_volumeRepository->getVolume( this->getVolumeID() ) );
        out->saveSeries( fileName.toLatin1() );
        m_exportWorkingDirectory = QFileInfo( fileName ).dirPath();
    }
}

void QApplicationMainWindow::exportToMetaIO( )
{
    QString fileName = QFileDialog::getSaveFileName( this , tr("Choose an image filename") , m_exportWorkingDirectory, m_exportToMetaIOFilter );
            
    if (!fileName.isEmpty())
    {
        if( QFileInfo( fileName ).suffix() != "mhd" )
        {
            fileName += ".mhd";
        }
        Output *out = new Output();
        out->setInput( m_volumeRepository->getVolume( this->getVolumeID() ) );
        out->saveFile( fileName.toLatin1() );
        m_exportWorkingDirectory = QFileInfo( fileName ).dirPath();
    }
}

void QApplicationMainWindow::createMenus()
{
    // Men� d'arxiu: aquest es correspondr� a l'acc�s directe al sistema de fitxers per adquirir un volum, com pot ser un arxiu *.mhd
    m_fileMenu = menuBar()->addMenu( tr("&File") );
    m_fileMenu->addAction( m_newAction );
    m_fileMenu->addAction( m_openAction );
    m_fileMenu->addAction( m_pacsAction );
    
    m_fileMenu->addSeparator();
    
    m_importFilesMenu = m_fileMenu->addMenu( tr("&Import") );

    m_exportFilesMenu = m_fileMenu->addMenu( tr("&Export"));
    
    m_exportFilesMenu->addAction( m_exportToJpegAction );
    m_exportFilesMenu->addAction( m_exportToMetaIOAction );
    m_exportFilesMenu->addAction( m_exportToPngAction );
    m_exportFilesMenu->addAction( m_exportToBmpAction );
    // \TODO l'export al tipus Tiff falla, pot ser cosa de les itk o del suport del sistema a aquest tipu de fitxer
    m_exportFilesMenu->addAction( m_exportToTiffAction );
    
    m_fileMenu->addSeparator();
    
    m_recentFilesMenu = m_fileMenu->addMenu( tr("&Recent files") );
    for (int i = 0; i < MaxRecentFiles; ++i)
        m_recentFilesMenu->addAction( m_recentFileActions[i]);
        
    m_fileMenu->addSeparator();
    m_fileMenu->addAction( m_closeAction );
    m_fileMenu->addAction( m_exitAction );

    // aquest men� es correspondr� amb la connexi� al pacs
    m_databaseMenu = menuBar()->addMenu( tr("&Database") );

    // accions relacionades amb la segmentaci�
    m_segmentationMenu = menuBar()->addMenu( tr("&Segmentation") );

    // accions relacionades amb clustering
    m_clusteringMenu = menuBar()->addMenu( tr("&Clustering") );

    // accions relacionades amb el registre
    m_registrationMenu = menuBar()->addMenu( tr("&Registration") );;

    // accions relacionades amb la visualitzaci�
    m_visualizationMenu = menuBar()->addMenu( tr("&Visualization") );
    m_visualizationMenu->addAction( m_mpr2DAction );
    m_visualizationMenu->addAction( m_mpr3DAction );
    m_visualizationMenu->addAction( m_mpr3D2DAction );

    // accions relacionades amb tractament de color, funcions de transfer�ncia
    m_colorMenu = menuBar()->addMenu( tr("Co&lor") );

    // accions relacionades amb tools
    m_toolsMenu = menuBar()->addMenu( tr("&Tools") );

    // men� finestra, controla organitzaci� de la workingarea i permet canviar a altres finestres obertes amb els pacients
    m_windowMenu = menuBar()->addMenu( tr("&Window") );

    // men� d'opcions del programa
    m_optionsMenu = menuBar()->addMenu( tr("&Options") );

    // men� per escollir idioma
    m_languageMenu = menuBar()->addMenu( tr("&Language") );

    menuBar()->insertSeparator();
    
    // men� d'ajuda, ara nom�s hi ha els t�pic abouts  
    m_helpMenu = menuBar()->addMenu(tr("&Help") );
    m_helpMenu->addAction( m_aboutAction );
}

void QApplicationMainWindow::createToolBars()
{
    m_fileToolBar = addToolBar( tr("File") );
    m_fileToolBar->addAction( m_newAction );
    m_fileToolBar->addAction( m_openAction );
    m_fileToolBar->addAction( m_pacsAction );
    
    m_databaseToolBar = addToolBar( tr("Database") );
    
    m_optionsToolBar = addToolBar( tr("Options") );

    m_extensionsToolBar = addToolBar( tr("Extensions") );
}


void QApplicationMainWindow::showProgress( int value )
{
    if( value == -1 )
    {
    // hi ha hagut una excepci� en la lectura del fitxer, mostrar alerta
        m_progressDialog->cancel();
        QMessageBox::critical( this , tr("Error") , tr("Exception while reading file. Cannot open the specified file.") );
    }
    else
    {
        m_progressDialog->setValue( value );
    }
}

void QApplicationMainWindow::newFile()
{
    QString windowName;    
    QApplicationMainWindow *newMainWindow = new QApplicationMainWindow( 0, windowName.sprintf( "NewWindow[%d]" ,getCountQApplicationMainWindow() + 1 ) );
    newMainWindow->show();
}

void QApplicationMainWindow::pacsQueryScreen()
{
    m_queryScreen->show();
}

void QApplicationMainWindow::close()
{
    if( !m_volumeID.isNull() )
    {
        // tancar el volum, �s a dir treure'l del repositori, matar les mini-aplicacions que el fan servir o almenys alertar de si realment es vol fer ja que hi ha mini-apps que el faran servir conjuntament amb altres com el registre. Bona part d'aix� ser� feina del mini-app handler
    }
    else
    {
        // error que no s'hauria de donar
    }
}

unsigned int QApplicationMainWindow::getCountQApplicationMainWindow()
{
    QWidgetList list( QApplication::topLevelWidgets() );
    QWidget *widget;
    unsigned int count = 0;
    for ( int i = 0; i < list.size(); ++i )
    {
        if ( QWidget *mainWin = qobject_cast<QWidget *>( list.at(i) ) )
        {
            if( mainWin->isA("udg::QApplicationMainWindow") )
                count++;
                
        }
    }
    return count;
}

void QApplicationMainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

void QApplicationMainWindow::setCurrentFile(const QString &fileName)
{
    m_currentFile = fileName;
    if ( m_currentFile.isEmpty() )
        setWindowTitle(tr("StarViewer"));
    else
        setWindowTitle(tr("%1 - %2").arg( strippedName( m_currentFile ) )
                                    .arg( tr("Starviewer") ) );

    m_recentFiles.removeAll( fileName );
    m_recentFiles.prepend(fileName);
    while ( m_recentFiles.size() > MaxRecentFiles )
        m_recentFiles.removeLast();

    foreach ( QWidget *widget, QApplication::topLevelWidgets() )
    {
        QApplicationMainWindow *mainWin = qobject_cast<QApplicationMainWindow *>(widget);
        if (mainWin)
            mainWin->updateRecentFileActions();
    }
}

QString QApplicationMainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void QApplicationMainWindow::updateRecentFileActions()
{
    int numRecentFiles = qMin(m_recentFiles.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i)
    {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(m_recentFiles[i]));
        m_recentFileActions[i]->setText(text);
        m_recentFileActions[i]->setData(m_recentFiles[i]);
        m_recentFileActions[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        m_recentFileActions[j]->setVisible(false);

}

void QApplicationMainWindow::openRecentFile( int param )
{
//     if (maybeSave())
//         loadFile(m_recentFiles[param]);
}

void QApplicationMainWindow::createStatusBar()
{
}

void QApplicationMainWindow::about()
{
    QMessageBox::about(this, tr("About StarViewer"),
            tr("<h2>StarViewer 2006 �Beta�</h2>"
               "<p>Copyright &copy; 2004 Universitat de Girona"
               "<p>StarViewer is a small application that "
               "lets you view <b>DICOM</b>, <b>MHD's</b>,... "
               "files and manipulate them."
               "<p>Last Redisign Version : in early development")
               );
}

void QApplicationMainWindow::writeSettings()
{
    QSettings settings("software-inc.com", "StarViewer");

    settings.beginGroup("StarViewer");

    settings.setValue( "position", pos() );
    settings.setValue( "size", size() );
    settings.setValue( "recentFiles" , m_recentFiles );
    settings.setValue( "workingDirectory" , m_workingDirectory );
    settings.setValue( "exportWorkingDirectory" , m_exportWorkingDirectory );
    settings.setValue( "defaultLocale" , m_defaultLocale );
    
    settings.endGroup();
}

void QApplicationMainWindow::readSettings()
{
    QSettings settings("software-inc.com", "StarViewer");
    settings.beginGroup("StarViewer");

    move( settings.value("position", QPoint(200, 200)).toPoint());
    resize( settings.value("size", QSize(400, 400)).toSize());
    
    m_recentFiles = settings.value("recentFiles").toStringList();
    updateRecentFileActions();

    m_workingDirectory = settings.value("workingDirectory", ".").toString();
    m_exportWorkingDirectory = settings.value("exportWorkingDirectory", ".").toString();
    m_defaultLocale = settings.value("defaultLocale", "en_GB" ).toString();
    
    settings.endGroup();
}

}; // end namespace udg
