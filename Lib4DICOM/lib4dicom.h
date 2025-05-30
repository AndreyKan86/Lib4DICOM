#pragma once
#include <QObject>
#include "lib4dicom_global.h"


class LIB4DICOM_EXPORT Lib4DICOM : public QObject
{
    Q_OBJECT

public:
    explicit Lib4DICOM(QObject* parent = nullptr);


    void qiToByteVector(const std::vector<QImage>& vector_image); // Работает с вектором QImage

    //Передача данных
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

    //Сохранение изображения
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
