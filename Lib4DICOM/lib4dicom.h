#pragma once

#include <QAbstractListModel>
#include <QImage>
#include <QVector>
#include <QVariant>
#include <QString>

#include "lib4dicom_global.h"

class OFString;

struct Patient {
    QString fullName;        // "Иванов Иван"
    QString birthYear;       // "YYYY"
    QString birthRaw;        // "YYYYMMDD" или "YYYY"
    QString sex;             // "M"/"F"/"O"
    QString patientID;       // "12345"

    QString sourceFilePath;  // источник (скан/поиск)
    QString patientFolder;   // корневая папка пациента (кеш)
};

class LIB4DICOM_EXPORT Lib4DICOM : public QAbstractListModel {
    Q_OBJECT

        // Эти свойства реально используются из QML
        Q_PROPERTY(QAbstractItemModel* patientModel READ patientModel NOTIFY patientModelChanged)
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

    Q_INVOKABLE QVariantMap saveImagesAsDicom(const QVector<QImage>& images,
        const QString& outFolder,
        const QString& seriesName,
        const QString& studyUIDIn,
        const QVariantMap& patient);

    // Функции используемые в QML
    Q_INVOKABLE QVariantMap convertAndSaveImageAsDicom(const QString& imagePath,
        const QString& studyFolder,
        const QString& seriesName,
        const QString& studyUID,
        const QVariantMap& patient);

    Q_INVOKABLE QVariantMap createStudyForNewPatient(const QVariantMap& patient);
    Q_INVOKABLE QVariantMap createStudyInPatientFolder(const QString& patientFolder,
        const QString& patientID);
    Q_INVOKABLE QVariantMap createPatientStubDicom(const QString& patientFolder,
        const QVariantMap& patient);
    Q_INVOKABLE QVariantMap makePatientFromStrings(const QString& fullName,
        const QString& birthInput,
        const QString& sexInput,
        const QString& patientID);
    Q_INVOKABLE QVariantMap getPatientDemographics(int index) const; // { fullName, birthYear, sex, patientID, patientFolder }
    Q_INVOKABLE QVariantMap findPatientStubByIndex(int index) const; // { ok, patientFolder, stubPath }
    Q_INVOKABLE QVariantMap readDemographicsFromFile(const QString& dcmPath) const; // { ok, patientName, patientBirth, patientSex, patientID }

signals:
    void patientModelChanged();
    void studyLabelChanged();

private:
    // Роли — деталь реализации модели
    enum Roles { FullNameRole = Qt::UserRole + 1, BirthYearRole, SexRole };

    // READ для Q_PROPERTY можно было бы сделать private,
    // но раз QML активно использует, оставим здесь:
    QAbstractItemModel* patientModel();

    // ==== Остальной API прячем ====
    Q_INVOKABLE void scanPatients(); // сейчас не зовётся из QML

    // Тестовые/утилитные — скрываем
    Q_INVOKABLE void   TESTlogSelectedFileAndPatient(const QString& filePath,
        const QString& fullName,
        const QString& birthYear,
        const QString& sex);
    Q_INVOKABLE QImage TESTloadImageFromFile(const QString& localPath);
    Q_INVOKABLE QVector<QImage> TESTloadImageVectorFromFile(const QString& localPath);

    // Внутреннее (QML их не вызывает напрямую)
    QString     ensurePatientFolder(const QString& fullName, const QString& birthYear);

    QString sanitizeName(const QString& in);

    // Хелперы/преобразования
    static QString     generateDicomUID();
    static Patient     patientFromMap(const QVariantMap& m);
    static QVariantMap patientToMap(const Patient& p);

    // DCMTK helper (реализация в .cpp; здесь только сигнатура)
    static QString decodeDicomText(const OFString& value,
        const OFString& specificCharacterSet);

    // Данные модели
    QList<Patient> m_patients;
    QString m_studyLabel = "Study";
};
