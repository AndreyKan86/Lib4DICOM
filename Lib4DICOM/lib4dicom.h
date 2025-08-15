#pragma once

#define DCMTK_DLL  // Для корректного импорта/экспорта символов DCMTK
#include <QObject>
#include "lib4dicom_global.h"


class LIB4DICOM_EXPORT Lib4DICOM : public QObject
{
    Q_OBJECT

public:
    explicit Lib4DICOM(QObject* parent = nullptr);


    void qiToByteVector(
        const std::vector<QImage>& images,
        const QString& patientID,
        const QString& studyID,
        const QString& seriesID,
        const QString& outputDir,
        const QString& patientName,
        const QString& sex,
        const QString& weight
    );

    void saveQImageAsDicom(
        const QImage& image,
        const QString& patientID,
        const QString& studyID,
        const QString& seriesID,
        const QString& outputPath,
        const QString& patientName,
        const QString& sex,
        const QString& weight
    );

    void saveQImagesAsDicom(
        const std::vector<QImage>& images,
        const QString& patientID,
        const QString& studyID,
        const QString& seriesID,
        const QString& outputDir,
        const QString& patientName,
        const QString& sex,
        const QString& weight
    );

    Q_INVOKABLE void dataTransfer(
        const QString& patientID,
        const QString& studyID,
        const QString& seriesID,
        const QString& filename,
        const QString& patientName,
        const QString& patientFamily,
        const QString& patientFatherName,
        const QString& sex,
        const QString& weightKG,
        const QString& weightG,
        const QString& patinentAgeYear,
        const QString& patientAgeMonth,
        const QString& patinentAgeDay,
        const QString& patientBirthday
    );

private:
    const std::string prefix;

    QImage loadJPEG(const QString& path); 



    std::string generateStudyUID(
        const std::string* patientID,
        const std::string* studyID
    );


    std::string generateInstanceUID(
        const std::string* patientID,
        const std::string* studyID,
        const std::string* seriesID
    );


    std::string generateSeriesUID(
        const std::string* patientID,
        const std::string* studyID,
        const std::string* seriesID
    );


    void saveImageAsDicom(
        const QImage& image,
        const std::string* patientID,
        const char* studyUID,
        const char* seriesUID,
        const char* filename,
        const char* patientName,
        const char* sex,
        const char* weight,
        const std::string* studyID,
        const std::string* seriesID
    );
};
