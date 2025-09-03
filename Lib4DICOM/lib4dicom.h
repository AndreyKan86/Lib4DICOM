#pragma once

#include <QAbstractListModel>
#include <QImage>
#include <QVector>
#include <QVariant>
#include <QString>

#include "lib4dicom_global.h"

class OFString;

struct Patient {
    QString fullName;       // "Иванов Иван"
    QString birthYear;      // "YYYY"
    QString birthRaw;       // "YYYYMMDD" или "YYYY"
    QString sex;            // "M"/"F"/"O"
    QString patientID;      // "12345"

    QString patientFolder;  // корневая папка пациента (куда пишем исследования)

    // Контекст текущего исследования/серии
    QString studyFolder;    // активная папка исследования
    QString studyUID;       // Study Instance UID
    QString seriesName;     // метка/имя серии (для SeriesDescription / имени файлов)
};

class LIB4DICOM_EXPORT Lib4DICOM : public QAbstractListModel {
    Q_OBJECT
        Q_PROPERTY(QString studyLabel READ studyLabel WRITE setStudyLabel NOTIFY studyLabelChanged)

public:
    explicit Lib4DICOM(QObject* parent = nullptr);

    // ==== Свойство, используемое в QML ====
    QString studyLabel() const;
    void setStudyLabel(const QString& s);

    // ==== Базовый интерфейс модели (используется ListView) ====
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ==== Публичный API для QML ====
    Q_INVOKABLE QVariantMap saveImagesAsDicom(const QVector<QImage>& images);
    Q_INVOKABLE QVariantMap convertAndSaveImageAsDicom(const QString& imagePath);

    Q_INVOKABLE QVariantMap createStudyForNewPatient(); // без параметров — берёт m_selectedPatient
    Q_INVOKABLE QVariantMap createStudyInPatientFolder(const QString& patientFolder,
        const QString& patientID);
    Q_INVOKABLE QVariantMap createPatientStubDicom(const QString& patientFolder);

    Q_INVOKABLE QVariantMap makePatientFromStrings(const QString& fullName,
        const QString& birthInput,
        const QString& sexInput,
        const QString& patientID);

    Q_INVOKABLE QVariantMap getPatientDemographics(int index) const;   // { ok, fullName, birthYear, sex, patientID, patientFolder }
    Q_INVOKABLE QVariantMap findPatientStubByIndex(int index) const;   // { ok, patientFolder, stubPath }
    Q_INVOKABLE QVariantMap readDemographicsFromFile(const QString& dcmPath) const; // { ok, patientName, patientBirth, patientSex, patientID }

    // Глобальный выбранный пациент
    Q_INVOKABLE void selectExistingPatient(int index);
    Q_INVOKABLE void selectNewPatient(const QVariantMap& patient);
    Q_INVOKABLE void clearSelectedPatient();

    QVariantMap selectedPatient() const;

signals:
    void patientModelChanged();
    void selectedPatientChanged();
    void studyLabelChanged();

private:
    // Роли — деталь реализации модели
    enum Roles { FullNameRole = Qt::UserRole + 1, BirthYearRole, SexRole };

    // ==== Остальной API прячем ====
    Q_INVOKABLE void scanPatients(); // сейчас не зовётся из QML

    // Тестовые/утилитные — скрываем
    Q_INVOKABLE void   TESTlogSelectedFileAndPatient(const QString& filePath,
        const QString& fullName,
        const QString& birthYear,
        const QString& sex);
    Q_INVOKABLE QImage TESTloadImageFromFile(const QString& localPath);
    Q_INVOKABLE QVector<QImage> TESTloadImageVectorFromFile(const QString& localPath);

    // Внутреннее
    QString ensurePatientFolder(const QString& fullName, const QString& birthYear);
    QString sanitizeName(const QString& in);

    // Хелперы/преобразования
    static QString generateDicomUID();
    static Patient patientFromMap(const QVariantMap& m);
    static QString decodeDicomText(const OFString& value,
        const OFString& specificCharacterSet);

    // Данные модели
    QList<Patient> m_patients;
    QString m_studyLabel = "Study";

    Patient m_selectedPatient{};
    bool m_hasSelectedPatient = false;
};
