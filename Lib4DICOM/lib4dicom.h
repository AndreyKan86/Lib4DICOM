#pragma once
#include <QObject>
#include "lib4dicom_global.h"


class LIB4DICOM_EXPORT Lib4DICOM : public QObject
{
    Q_OBJECT

public:
    explicit Lib4DICOM(QObject* parent = nullptr);


    Q_INVOKABLE void performAction();


    void qiToByteVector(const std::vector<QImage>& vector_image); // �������� � �������� QImage


    void sayHello();

    //�������������� � DICOM
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
};
