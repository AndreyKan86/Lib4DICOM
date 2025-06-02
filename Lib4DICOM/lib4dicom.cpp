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

//Функция сохраняющая QImage, как DICOM
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
    }   //Защита

    int width = image.width();
    int height = image.height();
    int channels;

    Lib4DICOM obj;
    std::string uid = obj.generateInstanceUID(patientID, studyID, seriesID);  // всё по указателям

    char instanceUID[100];
    std::strncpy(instanceUID, uid.c_str(), sizeof(instanceUID));
    instanceUID[sizeof(instanceUID) - 1] = '\0';

    QImage img = image;

    // Приводим к нужному формату
    if (img.format() == QImage::Format_RGB888) {
        channels = 3;
    }
    else if (img.format() == QImage::Format_Grayscale8) {
        channels = 1;
    }
    else {
        img = img.convertToFormat(QImage::Format_RGB888); //Конвертация в RGGB
        channels = 3;
    }

    int bytesPerLine = img.bytesPerLine();
    int expectedLine = img.width() * channels;
    qDebug() << "Bytes per line:" << bytesPerLine << "Expected:" << expectedLine;

    const uchar* pixelData = img.bits(); //преобразование QImage 
    size_t pixelDataLength = static_cast<size_t>(width) * height * channels; // Получение дополнительных данных


    //Создание DICOM файла
    DcmFileFormat fileformat;
    DcmDataset* dataset = fileformat.getDataset();

    dataset->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192");
    // Пациент

    dataset->putAndInsertString(DCM_PatientName, patientName);
    dataset->putAndInsertString(DCM_PatientID, patientID->c_str());
    dataset->putAndInsertString(DCM_StudyInstanceUID, studyUID);
    dataset->putAndInsertString(DCM_SeriesInstanceUID, seriesUID);
    dataset->putAndInsertString(DCM_SOPInstanceUID, instanceUID);
    //dataset->putAndInsertString(DCM_SOPClassUID, UID_UltrasoundImageStorage); //УЗИ
    dataset->putAndInsertString(DCM_SOPClassUID, UID_SecondaryCaptureImageStorage); //Просто рисунок

    // Размер изображения
    dataset->putAndInsertUint16(DCM_Rows, static_cast<Uint16>(height));
    dataset->putAndInsertUint16(DCM_Columns, static_cast<Uint16>(width));

    dataset->putAndInsertUint16(DCM_SamplesPerPixel, static_cast<Uint16>(channels));

    if (channels == 1)
        dataset->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2");
    else if (channels == 3)
        dataset->putAndInsertString(DCM_PhotometricInterpretation, "RGB");
    else
        std::cerr << "Error: unsupported number of channels" << std::endl;

    // Параметры пикселей
    dataset->putAndInsertUint16(DCM_BitsAllocated, 8);
    dataset->putAndInsertUint16(DCM_BitsStored, 8);
    dataset->putAndInsertUint16(DCM_HighBit, 7);
    dataset->putAndInsertUint16(DCM_PixelRepresentation, 0);  // unsigned
    dataset->putAndInsertUint16(DCM_SamplesPerPixel, 1);


    // Время
    OFString dateStr, timeStr;
    DcmDate::getCurrentDate(dateStr);
    DcmTime::getCurrentTime(timeStr);

    dataset->putAndInsertString(DCM_ContentDate, dateStr.c_str());
    dataset->putAndInsertString(DCM_ContentTime, timeStr.c_str());
    dataset->putAndInsertString(DCM_AcquisitionDate, dateStr.c_str());
    dataset->putAndInsertString(DCM_AcquisitionTime, timeStr.c_str());
    dataset->putAndInsertString(DCM_StudyDate, dateStr.c_str());
    dataset->putAndInsertString(DCM_StudyTime, timeStr.c_str());

    // Доп. информация
    dataset->putAndInsertString(DCM_PatientSex, sex);
    dataset->putAndInsertString(DCM_PatientWeight, weight);

    //Создание буфера для записи
    QByteArray buffer;
    buffer.resize(width * height);
    uchar* dest = reinterpret_cast<uchar*>(buffer.data());
    for (int y = 0; y < height; ++y) {
        const uchar* line = image.constScanLine(y);
        memcpy(dest + y * width, line, width);
    }
    //

    dataset->putAndInsertUint8Array(DCM_PixelData, reinterpret_cast<Uint8*>(buffer.data()), buffer.size());

    qDebug() << pixelDataLength;

    dataset->putAndInsertString(DCM_ImageOrientationPatient, "1\\0\\0\\0\\1\\0");
    dataset->putAndInsertString(DCM_ImagePositionPatient, "70\\0\\0");



    //////////////////////////////////////////


    std::time_t now = std::time(nullptr);
    char new_fn[100];
    std::snprintf(new_fn, sizeof(new_fn), "%s_%lld.dcm", filename, static_cast<long long>(now));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // пауза 1000 мс = 1 секунда

    // Сохраняем
    OFCondition status = fileformat.saveFile(OFFilename(new_fn), EXS_LittleEndianExplicit);
    if (status.good()) {
        std::cout << "Done: " << filename << std::endl;
    }
    else {
        std::cerr << "Error: " << status.text() << std::endl;
    }

    qDebug() << "Ok";
}

//Принимается на вход вектор с QImage. Публичная
void Lib4DICOM::qiToByteVector(const std::vector<QImage>& vector_image) {

    if (vector_image.empty()) {
        std::cout << "Empty vector ";
    }

    for (QImage image : vector_image) {

        ///////////////ВРЕМЕННО//////////////////
        std::string pID = "1";
        std::string studyID = "1";
        std::string seriesID = "3";

        //QImage image = loadJPEG(path);

        saveImageAsDicom(
            image,
            &pID,
            "0",
            "1",
            "C:\\my_files\\DICOM\\output",
            "Tom Andreson",
            "M",
            "90.0",
            &studyID,
            &seriesID
        );
    }
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
    // Преобразование в std::string
    const std::string patientID_std = patientID.toStdString();
    const std::string studyID_std = studyID.toStdString();
    const std::string seriesID_std = seriesID.toStdString();
    const std::string filename_std = filename.toStdString();

    // Генерация UID'ов
    const std::string studyUID = generateStudyUID(&patientID_std, &studyID_std);
    const std::string seriesUID = generateSeriesUID(&patientID_std, &studyID_std, &seriesID_std);
    const char* studyUID_chr = studyUID.c_str();
    const char* seriesUID_chr = seriesUID.c_str();

    // Загрузка изображения
    QImage img = loadJPEG(filename);
    if (img.isNull()) {
        qDebug() << "Image is null, aborting.";
        return;
    }

    // Имя файла без расширения
    size_t slashPos = filename_std.find_last_of("/\\");
    std::string filenameWithExt = filename_std.substr(slashPos + 1);
    size_t dotPos = filenameWithExt.find_last_of('.');
    std::string nameFile = filenameWithExt.substr(0, dotPos);
    QByteArray nameFileUtf8 = QByteArray::fromStdString(nameFile);
    const char* nameFile_chr = nameFileUtf8.constData();

    // Полное имя пациента: Фамилия^Имя^Отчество
    QString fullName = QString("%1^%2^%3").arg(patientFamily, patientName, patientFatherName);
    static QByteArray fullNameUtf8;
    fullNameUtf8 = fullName.toUtf8();
    const char* patientName_chr = fullNameUtf8.constData();

    // Нормализация пола до 'M' / 'F' / 'O'
    QString normalizedSex = sex.trimmed().left(1).toUpper();
    static QByteArray sexUtf8;
    sexUtf8 = normalizedSex.toUtf8();
    const char* patientSex_chr = sexUtf8.constData();

    // Вес как строка с точкой (пример: "75.5")
    bool ok1 = false, ok2 = false;
    double kg = weightKG.toDouble(&ok1);
    double g = weightG.toDouble(&ok2);
    double totalWeight = (ok1 ? kg : 0.0) + (ok2 ? g : 0.0) / 1000.0;
    QString weightStr = QString::number(totalWeight, 'f', 1);
    static QByteArray weightUtf8;
    weightUtf8 = weightStr.toUtf8();
    const char* patientWeightKG_chr = weightUtf8.constData();


    qDebug() << sex;

    // Вызов сохранения (без передачи возраста — он не нужен, если функция его не принимает)
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



