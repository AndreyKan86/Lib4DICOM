#pragma once
#include <QObject>
#include "lib4dicom_global.h"


class LIB4DICOM_EXPORT Lib4DICOM : public QObject
{
    Q_OBJECT

public:
    explicit Lib4DICOM(QObject* parent = nullptr);


    Q_INVOKABLE void performAction();


    void qiToByteVector(const std::vector<QImage>& vector_image); // Работает с вектором QImage


    void sayHello();

    //Преобразование в DICOM
    Q_INVOKABLE void saveImageAsDicom(
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

private:
    const std::string prefix;


    QImage loadJPEG(const QString& path); //Принимает строку с адресом файла возвращает QImage

    //Генерация уникального UID исследования
    std::string generateStudyUID(
        const std::string* patientID,
        const std::string* studyID
    );

    //Генерация уникального UID изображения (ВРЕМЕННО!!)
    std::string generateInstanceUID(
        const std::string* patientID,
        const std::string* studyID,
        const std::string* seriesID
    );

    //Генерация уникального UID серии
    std::string generateSeriesUID(
        const std::string* patientID,
        const std::string* studyID,
        const std::string* seriesID
    );
};
