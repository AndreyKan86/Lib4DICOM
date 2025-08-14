#include "lib4dicom.h"
#include <QDebug>
#include <QImage>
#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dcuid.h>    
#include <dcmtk/dcmdata/dcfilefo.h> 
#include <dcmtk/dcmdata/dctk.h>      
#include "lib4dicom.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QUrl>
#include <QFileInfo> 

#include <filesystem>
#include <iostream>
#include <chrono>
#include <thread>
#include <ctime>
#include <cstdio>
#include <QGuiApplication>
#include <QUrl>
#include <QDebug>
#include <qquickview.h>

Lib4DICOM::Lib4DICOM(QObject* parent)
    : QObject(parent), prefix("1.2.643.10008.1.2.4.57")
{
}


void Lib4DICOM::saveQImageAsDicom(
    const QImage& image,
    const QString& patientID,
    const QString& studyID,
    const QString& seriesID,
    const QString& outputPath,
    const QString& patientName,
    const QString& sex,
    const QString& weight
) {
    if (image.isNull()) {
        qWarning() << "Invalid QImage provided!";
        return;
    }

    const std::string patientID_std = patientID.toStdString();
    const std::string studyID_std = studyID.toStdString();
    const std::string seriesID_std = seriesID.toStdString();

    // ��������� UID
    const std::string studyUID = generateStudyUID(&patientID_std, &studyID_std);
    const std::string seriesUID = generateSeriesUID(&patientID_std, &studyID_std, &seriesID_std);

    // ���������� ����� ����� ��� ����������
    QFileInfo fileInfo(outputPath);
    QString baseName = fileInfo.baseName();

    saveImageAsDicom(
        image,
        &patientID_std,
        studyUID.c_str(),
        seriesUID.c_str(),
        baseName.toUtf8().constData(),
        patientName.toUtf8().constData(),
        sex.toUtf8().constData(),
        weight.toUtf8().constData(),
        &studyID_std,
        &seriesID_std
    );
}

void Lib4DICOM::saveQImagesAsDicom(
    const std::vector<QImage>& images,
    const QString& patientID,
    const QString& studyID,
    const QString& seriesID,
    const QString& outputDir,
    const QString& patientName,
    const QString& sex,
    const QString& weight
) {
    if (images.empty()) {
        qWarning() << "No images provided!";
        return;
    }

    int counter = 1;
    for (const QImage& image : images) {
        QString outputPath = QString("%1/image_%2.dcm")
            .arg(outputDir)
            .arg(counter++, 4, 10, QLatin1Char('0'));

        saveQImageAsDicom(
            image,
            patientID,
            studyID,
            seriesID,
            outputPath,
            patientName,
            sex,
            weight
        );
    }
}


QImage Lib4DICOM::loadJPEG(const QString& path)
{
    qDebug() << "Start load JPEG from:" << path;

    QString localPath = QUrl(path).toLocalFile();
    QImage image(localPath);
    if (image.isNull()) {
        qDebug() << "Error loading image from:" << localPath;
        return QImage();
    }

    qDebug() << "Loaded image:" << image.width() << "x" << image.height();
    return image;
}

std::string Lib4DICOM::generateStudyUID(
    const std::string* patientID,
    const std::string* studyID
) {
    char studyUID[100];
    std::string seriesRoot = prefix + "." + *patientID + "." + *studyID;
    dcmGenerateUniqueIdentifier(studyUID, seriesRoot.c_str());
    return studyUID;
}

std::string Lib4DICOM::generateSeriesUID(
    const std::string* patientID,
    const std::string* studyID,
    const std::string* seriesID
) {
    char seriesUID[100];
    std::string seriesRoot = prefix + "." + *patientID + "." + *studyID + "." + *seriesID;
    dcmGenerateUniqueIdentifier(seriesUID, seriesRoot.c_str());
    return seriesUID;
}

std::string Lib4DICOM::generateInstanceUID(
    const std::string* patientID,
    const std::string* studyID,
    const std::string* seriesID
) {
    char instanceUID[100];
    std::string seriesRoot = prefix + "." + *patientID + "." + *studyID + "." + *seriesID;
    dcmGenerateUniqueIdentifier(instanceUID, seriesRoot.c_str());
    return instanceUID;
}


void Lib4DICOM::saveImageAsDicom(
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
)
{
    if (image.isNull()) {
        std::cerr << "Error: QImage is null!" << std::endl;
        return;
    }

    int width = image.width();
    int height = image.height();
    int channels;
    int bitsAllocated;

    // ��������� Instance UID
    std::string uid = generateInstanceUID(patientID, studyID, seriesID);
    char instanceUID[100];
    std::strncpy(instanceUID, uid.c_str(), sizeof(instanceUID));
    instanceUID[sizeof(instanceUID) - 1] = '\0';

    // ����������� � ���������� ������
    QImage img;
    if (image.format() == QImage::Format_Grayscale8) {
        // 8-bit grayscale
        img = image;
        channels = 1;
        bitsAllocated = 8;
    }
    else if (image.format() == QImage::Format_RGB888 ||
             image.format() == QImage::Format_RGB32) {
        // 24-bit RGB
        img = image.convertToFormat(QImage::Format_RGB888);
        channels = 3;
        bitsAllocated = 24;
    }
    else if (image.format() == QImage::Format_ARGB32) {
        // 32-bit ARGB (������������ � RGB, ��������� �����-�����)
        img = image.convertToFormat(QImage::Format_RGB888);
        channels = 3;
        bitsAllocated = 24;
    }
    else {
        // ��� ��������� ������� ������������ � RGB
        img = image.convertToFormat(QImage::Format_RGB888);
        channels = 3;
        bitsAllocated = 24;
    }

    // �������� DICOM �����
    DcmFileFormat fileformat;
    DcmDataset* dataset = fileformat.getDataset();

    // ��������� ���������
    dataset->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192");

    // �������� DICOM ����
    dataset->putAndInsertString(DCM_PatientName, patientName);
    dataset->putAndInsertString(DCM_PatientID, patientID->c_str());
    dataset->putAndInsertString(DCM_StudyInstanceUID, studyUID);
    dataset->putAndInsertString(DCM_SeriesInstanceUID, seriesUID);
    dataset->putAndInsertString(DCM_SOPInstanceUID, instanceUID);
    dataset->putAndInsertString(DCM_SOPClassUID, UID_SecondaryCaptureImageStorage);

    // ��������� �����������
    dataset->putAndInsertUint16(DCM_Rows, static_cast<Uint16>(height));
    dataset->putAndInsertUint16(DCM_Columns, static_cast<Uint16>(width));
    dataset->putAndInsertUint16(DCM_SamplesPerPixel, static_cast<Uint16>(channels));

    // ��������� ������� �������
    if (channels == 1) {
        // 8-bit grayscale
        dataset->putAndInsertUint16(DCM_BitsAllocated, 8);
        dataset->putAndInsertUint16(DCM_BitsStored, 8);
        dataset->putAndInsertUint16(DCM_HighBit, 7);
        dataset->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2");
    }
    else {
        // 24-bit RGB
        dataset->putAndInsertUint16(DCM_BitsAllocated, 8);  // 8 ��� �� �����
        dataset->putAndInsertUint16(DCM_BitsStored, 8);
        dataset->putAndInsertUint16(DCM_HighBit, 7);
        dataset->putAndInsertString(DCM_PhotometricInterpretation, "RGB");
        dataset->putAndInsertUint16(DCM_PlanarConfiguration, 0);  // 0 = interleaved (R1G1B1 R2G2B2...)
    }

    dataset->putAndInsertUint16(DCM_PixelRepresentation, 0);  // unsigned

    // ��������� �����
    OFString dateStr, timeStr;
    DcmDate::getCurrentDate(dateStr);
    DcmTime::getCurrentTime(timeStr);

    dataset->putAndInsertString(DCM_ContentDate, dateStr.c_str());
    dataset->putAndInsertString(DCM_ContentTime, timeStr.c_str());
    dataset->putAndInsertString(DCM_AcquisitionDate, dateStr.c_str());
    dataset->putAndInsertString(DCM_AcquisitionTime, timeStr.c_str());
    dataset->putAndInsertString(DCM_StudyDate, dateStr.c_str());
    dataset->putAndInsertString(DCM_StudyTime, timeStr.c_str());

    // ��������������� ������
    dataset->putAndInsertString(DCM_PatientSex, sex);
    dataset->putAndInsertString(DCM_PatientWeight, weight);

    // ������� ����������� (����������� ��������)
    dataset->putAndInsertString(DCM_ImageOrientationPatient, "1\\0\\0\\0\\1\\0");
    dataset->putAndInsertString(DCM_ImagePositionPatient, "0\\0\\0");

    // ���������� ���������� ������ � ������ ������������ �����
    QByteArray buffer;
    const int bytesPerLine = img.bytesPerLine(); // �������� �������� ����� ������ � ������ ������������
    
    if (channels == 1) {
        // ��� 8-bit grayscale
        buffer.resize(width * height);
        for (int y = 0; y < height; ++y) {
            const uchar* srcLine = img.constScanLine(y);
            uchar* dstLine = reinterpret_cast<uchar*>(buffer.data()) + y * width;
            memcpy(dstLine, srcLine, width);
        }
    }
    else {
        // ��� RGB (3 ������)
        buffer.resize(width * height * 3);
        for (int y = 0; y < height; ++y) {
            const uchar* srcLine = img.constScanLine(y);
            uchar* dstLine = reinterpret_cast<uchar*>(buffer.data()) + y * width * 3;
            memcpy(dstLine, srcLine, width * 3);
        }
    }

    // ������ ���������� ������
    dataset->putAndInsertUint8Array(
        DCM_PixelData,
        reinterpret_cast<Uint8*>(buffer.data()),
        buffer.size()
    );

    // ��������� ����� ����� � ��������� ������
    std::time_t now = std::time(nullptr);
    char new_fn[100];
    std::snprintf(new_fn, sizeof(new_fn), "%s_%lld.dcm", filename, static_cast<long long>(now));

    // ���������� DICOM �����
    OFCondition status = fileformat.saveFile(OFFilename(new_fn), EXS_LittleEndianExplicit);
    if (status.good()) {
        qDebug() << "DICOM file saved successfully:" << new_fn;
    } 
    else {
        qDebug() << "Error saving DICOM file:" << status.text();
    }
}


//����������� �� ���� ������ � QImage. ���������
void Lib4DICOM::qiToByteVector(
    const std::vector<QImage>& images,
    const QString& patientID,
    const QString& studyID,
    const QString& seriesID,
    const QString& outputDir,
    const QString& patientName,
    const QString& sex,
    const QString& weight
) {
    if (images.empty()) {
        qWarning() << "No images provided!";
        return;
    }

    // ���������� ��� ������������ ����� saveQImagesAsDicom
    saveQImagesAsDicom(
        images,
        patientID,
        studyID,
        seriesID,
        outputDir,
        patientName,
        sex,
        weight
    );
}

void Lib4DICOM::dataTransfer(
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
    const QString& patientAgeYear,
    const QString& patientAgeMonth,
    const QString& patientAgeDay,
    const QString& patientBirthday
)
{
    // �������������� � std::string
    const std::string patientID_std = patientID.toStdString();
    const std::string studyID_std = studyID.toStdString();
    const std::string seriesID_std = seriesID.toStdString();
    const std::string filename_std = filename.toStdString();

    // ��������� UID'��
    const std::string studyUID = generateStudyUID(&patientID_std, &studyID_std);
    const std::string seriesUID = generateSeriesUID(&patientID_std, &studyID_std, &seriesID_std);
    const char* studyUID_chr = studyUID.c_str();
    const char* seriesUID_chr = seriesUID.c_str();

    // �������� �����������
    QImage img = loadJPEG(filename);
    if (img.isNull()) {
        qDebug() << "Image is null, aborting.";
        return;
    }

    // ��� ����� ��� ����������
    size_t slashPos = filename_std.find_last_of("/\\");
    std::string filenameWithExt = filename_std.substr(slashPos + 1);
    size_t dotPos = filenameWithExt.find_last_of('.');
    std::string nameFile = filenameWithExt.substr(0, dotPos);
    QByteArray nameFileUtf8 = QByteArray::fromStdString(nameFile);
    const char* nameFile_chr = nameFileUtf8.constData();

    // ������ ��� ��������: �������^���^��������
    QString fullName = QString("%1^%2^%3").arg(patientFamily, patientName, patientFatherName);
    static QByteArray fullNameUtf8;
    fullNameUtf8 = fullName.toUtf8();
    const char* patientName_chr = fullNameUtf8.constData();

    // ������������ ���� �� 'M' / 'F' / 'O'
    QString normalizedSex = sex.trimmed().left(1).toUpper();
    static QByteArray sexUtf8;
    sexUtf8 = normalizedSex.toUtf8();
    const char* patientSex_chr = sexUtf8.constData();

    // ��� ��� ������ � ������ (������: "75.5")
    bool ok1 = false, ok2 = false;
    double kg = weightKG.toDouble(&ok1);
    double g = weightG.toDouble(&ok2);
    double totalWeight = (ok1 ? kg : 0.0) + (ok2 ? g : 0.0) / 1000.0;
    QString weightStr = QString::number(totalWeight, 'f', 1);
    static QByteArray weightUtf8;
    weightUtf8 = weightStr.toUtf8();
    const char* patientWeightKG_chr = weightUtf8.constData();


    qDebug() << sex;

    // ����� ���������� (��� �������� �������� � �� �� �����, ���� ������� ��� �� ���������)
    saveImageAsDicom(
        img,
        &patientID_std,
        studyUID_chr,
        seriesUID_chr,
        nameFile_chr,
        patientName_chr,
        patientSex_chr,
        patientWeightKG_chr,
        &studyID_std,
        &seriesID_std
    );

}



