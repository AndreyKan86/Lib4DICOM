#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QList>
#include <QVector>     
#include <QImage>
#include <QVariant>

#include "lib4dicom_global.h"

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

    // Старая версия — оставляем для совместимости с QML
    Q_INVOKABLE QImage loadImageFromFile(const QString& localPath);

    // ✅ Новая версия — возвращает вектор, внутри которого один QImage
    Q_INVOKABLE QVector<QImage> loadImageVectorFromFile(const QString& localPath);

    Q_INVOKABLE QString ensurePatientFolder(const QString& fullName,
        const QString& birthYearNormalized);
    Q_INVOKABLE QVariantMap createStudyForNewPatient(const QString& fullName,
        const QString& birthYearNormalized,
        const QString& patientID);

    Q_INVOKABLE QVariantMap saveImagesAsDicom(const QVector<QImage>& images,
        const QString& outFolder,
        const QString& patientID,
        const QString& seriesName,
        const QString& studyUID);

signals:
    void patientModelChanged();

private:
    struct Patient {
        QString fullName;
        QString birthYear;
        QString sex;
        QString patientID;
    };

    static QString normalizeBirthYear(const QString& birthInput);
    static QString normalizeSex(const QString& sexInput);
    static QString normalizeID(const QString& idInput);
    static QString makeSafeFolderName(const QString& s);
    static QString generateDicomUID();

    static QImage  toRgb888(const QImage& src); // helper

    QList<Patient> m_patients;
};
