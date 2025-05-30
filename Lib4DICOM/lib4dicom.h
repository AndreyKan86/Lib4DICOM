#pragma once
#include <QObject>
#include "lib4dicom_global.h"


class LIB4DICOM_EXPORT Lib4DICOM : public QObject
{
    Q_OBJECT

public:
    explicit Lib4DICOM(QObject* parent = nullptr);


    void qiToByteVector(const std::vector<QImage>& vector_image); // �������� � �������� QImage

    //�������� ������
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

    QImage loadJPEG(const QString& path); //��������� ������ � ������� ����� ���������� QImage

    //��������� ����������� UID ������������
    std::string generateStudyUID(
        const std::string* patientID,
        const std::string* studyID
    );

    //��������� ����������� UID ����������� (��������!!)
    std::string generateInstanceUID(
        const std::string* patientID,
        const std::string* studyID,
        const std::string* seriesID
    );

    //��������� ����������� UID �����
    std::string generateSeriesUID(
        const std::string* patientID,
        const std::string* studyID,
        const std::string* seriesID
    );

    //���������� �����������
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
