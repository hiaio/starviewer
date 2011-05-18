#ifndef UDGQDICOMPRINTERBASICSETTINGSWIDGETBASE_H
#define UDGQDICOMPRINTERBASICSETTINGSWIDGETBASE_H

#include "ui_qdicomprinterbasicsettingswidgetbase.h"

#include "dicomprinter.h"

namespace udg {

///Classe que mostra i permet especificar els par�metres b�sics de configuraci� d'una impressora dicom;
class QDicomPrinterBasicSettingsWidget : public QWidget, private::Ui::QDicomPrinterBasicSettingsWidgetBase {
Q_OBJECT

public:
    QDicomPrinterBasicSettingsWidget(QWidget *parent = 0);

    ///Retorna/Mostra els basic printer settings
    void getDicomPrinterBasicSettings(DicomPrinter &dicomPrinter);
    void setDicomPrinterBasicSettings(DicomPrinter &dicomPrinter);

    ///Neteja els controls
    void clear();

signals:

    ///signal que s'emet quan es canvia algun dels settings d'una impressora
    void basicDicomPrinterSettingChanged();

private slots:

    ///Slot que emet signal indicant que algunes de les propietats de la impressora DICOM ha canviat
    void settingChanged();

private :

    void createConnections();

    ///Emplena DicomPrinter amb les dades del GroupBox Film Settings
    void getFilmSettings(DicomPrinter &dicomPrinter);

    ///Emplena DicomPrinter amb les dades del GroupBox Print Settings
    void getPrintSettings(DicomPrinter &dicomPrinter);

    ///Mostra els par�metres de Film Settings
    void setFilmSettings(DicomPrinter &dicomPrinter);

    ///Mostra els par�metres del Print Settings
    void setPrintSettings(DicomPrinter &dicomPrinter);
};
} // end namespace udg.

#endif