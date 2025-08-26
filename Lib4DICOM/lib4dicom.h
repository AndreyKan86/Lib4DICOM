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
        Q_PROPERTY(QString studyLabel READ studyLabel WRITE setStudyLabel NOTIFY studyLabelChanged)

public:
    enum Roles { FullNameRole = Qt::UserRole + 1, BirthYearRole, SexRole };

    explicit Lib4DICOM(QObject* parent = nullptr);

    // ==== Настройки ====
    QString studyLabel() const { return m_studyLabel; }
    void setStudyLabel(const QString& s) {
        QString v = s;
        if (v.trimmed().isEmpty()) v = "Study";
        if (v == m_studyLabel) return;
        m_studyLabel = v;
        emit studyLabelChanged();
    }

    // ==== QAbstractListModel ====
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ==== QML API ====
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

    // ФС для пациента/исследования (БЕЗ нормализации имён/символов)
    Q_INVOKABLE QString     ensurePatientFolder(const QString& fullName,
        const QString& birthYear);
    Q_INVOKABLE QVariantMap createStudyForNewPatient(const QString& fullName,
        const QString& birthYear,
        const QString& patientID);
    Q_INVOKABLE QVariantMap createStudyInPatientFolder(const QString& patientFolder,
        const QString& patientID);

    // Полная версия: сохранение DICOM (SC) с демографией
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

    // Пустой DICOM (stub) в корне папки пациента с демографией
    Q_INVOKABLE QVariantMap createPatientStubDicom(const QString& patientFolder,
        const QString& patientID,
        const QString& patientName,
        const QString& patientBirth,
        const QString& patientSex);

    // Утилиты для UI
    Q_INVOKABLE QVariantMap getPatientDemographics(int index) const; // fullName, birthYear, sex, patientID, patientFolder
    Q_INVOKABLE QVariantMap findPatientStubByIndex(int index) const; // { ok, patientFolder, stubPath }
    Q_INVOKABLE QVariantMap readDemographicsFromFile(const QString& dcmPath) const; // { ok, patientName, patientBirth, patientSex, patientID }

signals:
    void patientModelChanged();
    void studyLabelChanged();

private:
    struct Patient {
        QString fullName;
        QString birthYear;
        QString sex;
        QString patientID;
        QString sourceFilePath;
    };

    // ===== Хелперы без «нормализации» =====
    static QString generateDicomUID();
    static QString decodeDicomText(const OFString& value, const OFString& specificCharacterSet);
    static QImage  toRgb888(const QImage& src);
    static QString safeNameForPath(const QString& s);

    QList<Patient> m_patients;
    QString m_studyLabel = "Study";
};
