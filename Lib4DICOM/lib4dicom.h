#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QList>
#include "lib4dicom_global.h"   // подключаем макрос экспорта

class LIB4DICOM_EXPORT Lib4DICOM : public QAbstractListModel {
    Q_OBJECT
        Q_PROPERTY(QAbstractItemModel* patientModel READ patientModel NOTIFY patientModelChanged)

public:
    enum Roles { FullNameRole = Qt::UserRole + 1, BirthYearRole, SexRole };

    explicit Lib4DICOM(QObject* parent = nullptr);

    // --- QAbstractListModel ---
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // --- Доступ для QML ---
    QAbstractItemModel* patientModel();

    // --- Метод парсинга пациентов (вызывается из QML) ---
    Q_INVOKABLE void scanPatients();

signals:
    void patientModelChanged();

private:
    struct Patient {
        QString fullName;
        QString birthYear;
        QString sex;
    };
    QList<Patient> m_patients;
};
