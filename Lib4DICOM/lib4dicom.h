#pragma once

#include <QAbstractListModel>
#include <QImage>
#include <QVector>
#include <QVariant>
#include <QString>

#include "lib4dicom_global.h"

class OFString;

struct Patient {
    QString fullName;     // "Иванов Иван"
    QString birthYear;    // "YYYY"
    QString birthDA;      // "YYYYMMDD" или пусто
    QString sex;          // "M"/"F"/"O"
    QString patientID;    // "12345"

    QString patientFolder; // корневая папка пациента
    QString studyFolder;   // текущая папка исследования
    QString studyUID;      // UID текущего исследования
    QString seriesName;    // предпочтительное имя серии
};

class LIB4DICOM_EXPORT Lib4DICOM : public QAbstractListModel {
    Q_OBJECT
        Q_PROPERTY(QString studyLabel READ studyLabel WRITE setStudyLabel NOTIFY studyLabelChanged)

public:
    explicit Lib4DICOM(QObject* parent = nullptr);

    // ==== Свойство, используемое в QML ====
    QString studyLabel() const;
    void setStudyLabel(const QString& s);

    // ==== Модель ====
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void saveImagesAsDicom(const QVector<QImage>& images);

    // ==== API для QML ====
    Q_INVOKABLE QVariantMap makePatientFromStrings(const QString& fullName,
        const QString& birthInput,
        const QString& sexInput,
        const QString& patientID);

    Q_INVOKABLE QVariantMap createStudyForNewPatient();
    Q_INVOKABLE QVariantMap createStudyInPatientFolder(const QString& patientFolder,
        const QString& patientID);
    Q_INVOKABLE QVariantMap createPatientStubDicom(const QString& patientFolder);
    Q_INVOKABLE QVariantMap getPatientDemographics(int index) const;
    Q_INVOKABLE QVariantMap findPatientStubByIndex(int index) const;
    Q_INVOKABLE QVariantMap readDemographicsFromFile(const QString& dcmPath) const;

    Q_INVOKABLE void convertAndSaveImageAsDicom(const QString& imagePath);

    // выбор пациента (глобальный state)
    Q_INVOKABLE void selectExistingPatient(int index);
    Q_INVOKABLE void selectNewPatient(const QVariantMap& patient);
    Q_INVOKABLE void clearSelectedPatient();
    QVariantMap selectedPatient() const;

    // ==== НОВОЕ: передача полной даты рождения YYYYMMDD из QML ====
    Q_INVOKABLE void setSelectedBirthDA(const QString& birthDA);
    Q_INVOKABLE void   scanPatients();

signals:
    void selectedPatientChanged();
    void studyLabelChanged();

private:
    enum Roles { FullNameRole = Qt::UserRole + 1, BirthYearRole, SexRole };


    Q_INVOKABLE void   TESTlogSelectedFileAndPatient(const QString& filePath,
        const QString& fullName,
        const QString& birthYear,
        const QString& sex);
    Q_INVOKABLE QImage TESTloadImageFromFile(const QString& localPath);
    Q_INVOKABLE QVector<QImage> TESTloadImageVectorFromFile(const QString& localPath);

    QString  ensurePatientFolder(const QString& fullName, const QString& birthYear);
    QString  sanitizeName(const QString& in);

    static QString generateDicomUID();
    static Patient patientFromMap(const QVariantMap& m);
    static QString decodeDicomText(const OFString& value,
        const OFString& specificCharacterSet);

    QList<Patient> m_patients;
    QString        m_studyLabel = "Study";

    Patient m_selectedPatient{};
};
