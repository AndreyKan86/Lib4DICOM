#pragma once

#include <QAbstractListModel>
#include <QImage>
#include <QVector>
#include <QVariant>
#include <QString>

#include "lib4dicom_global.h"

// DCMTK: для OFString, используемого в decodeDicomText(...)
#include <dcmtk/ofstd/ofstring.h>

class LIB4DICOM_EXPORT Lib4DICOM : public QAbstractListModel {
    Q_OBJECT
        Q_PROPERTY(QAbstractItemModel* patientModel READ patientModel NOTIFY patientModelChanged)

public:
    enum Roles { FullNameRole = Qt::UserRole + 1, BirthYearRole, SexRole };
    explicit Lib4DICOM(QObject* parent = nullptr);

    // QAbstractListModel
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // QML API
    QAbstractItemModel* patientModel();
    Q_INVOKABLE void scanPatients();

    Q_INVOKABLE void logSelectedFileAndPatient(const QString& filePath,
        const QString& fullName,
        const QString& birthYear,
        const QString& sex);

    Q_INVOKABLE QVariantMap makePatientFromStrings(const QString& fullName,
        const QString& birthInput,
        const QString& sexInput,
        const QString& patientID);

    // Загрузка изображения
    Q_INVOKABLE QImage          loadImageFromFile(const QString& localPath);
    Q_INVOKABLE QVector<QImage> loadImageVectorFromFile(const QString& localPath);

    // ФС для пациента/исследования
    Q_INVOKABLE QString     ensurePatientFolder(const QString& fullName,
        const QString& birthYearNormalized);
    Q_INVOKABLE QVariantMap createStudyForNewPatient(const QString& fullName,
        const QString& birthYearNormalized,
        const QString& patientID);



    // ====== Полная версия: с демографией (только QVector) ======
    Q_INVOKABLE QVariantMap saveImagesAsDicom(const QVector<QImage>& images,
        const QString& outFolder,
        const QString& patientID,
        const QString& seriesName,
        const QString& studyUID,
        const QString& patientName,
        const QString& patientBirth,
        const QString& patientSex);

    // Комбайн (файл -> DICOM) с демографией
    Q_INVOKABLE QVariantMap convertAndSaveImageAsDicom(const QString& imagePath,
        const QString& studyFolder,
        const QString& patientID,
        const QString& seriesName,
        const QString& studyUID,
        const QString& patientName,
        const QString& patientBirth,
        const QString& patientSex);

    // Пустой DICOM в корне папки пациента с демографией
    Q_INVOKABLE QVariantMap createPatientStubDicom(const QString& patientFolder,
        const QString& patientID,
        const QString& patientName,
        const QString& patientBirth,
        const QString& patientSex);

    Q_INVOKABLE QVariantMap getPatientDemographics(int index) const; // fullName, birthYear, sex, patientID, patientFolder
    Q_INVOKABLE QVariantMap createStudyInPatientFolder(const QString& patientFolder,
        const QString& patientID);


    // Найти stub-файл пациента по индексу списка.
// Возвращает: { ok, patientFolder, stubPath }
    Q_INVOKABLE QVariantMap findPatientStubByIndex(int index) const;

    // Прочитать демографию из конкретного DICOM-файла.
    // Возвращает: { ok, patientName, patientBirth, patientSex, patientID }
    Q_INVOKABLE QVariantMap readDemographicsFromFile(const QString& dcmPath) const;




signals:
    void patientModelChanged();

private:
    struct Patient {
        QString fullName;
        QString birthYear;
        QString sex;
        QString patientID;
        QString sourceFilePath;
    };

    // Нормализация и хелперы
    static QString normalizeBirthYear(const QString& birthInput);
    static QString normalizeSex(const QString& sexInput);
    static QString normalizeID(const QString& idInput);
    static QString makeSafeFolderName(const QString& s);
    static QString generateDicomUID();

    static QString toDicomPN(const QString& fullName);    // "Иванов Иван" -> "Иванов^Иван"
    static QString toDicomDA(const QString& birthInput);  // "YYYY" или "YYYYMMDD"
    static QString toDicomSex(const QString& sexInput);   // "M"/"F"/"O"

    // Декодирование текстов DICOM с учётом SpecificCharacterSet
    static QString decodeDicomText(const OFString& value,
        const OFString& specificCharacterSet);

    static QImage  toRgb888(const QImage& src);

    QList<Patient> m_patients;
};
  