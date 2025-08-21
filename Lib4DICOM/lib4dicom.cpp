#include "lib4dicom.h"

#include <QCoreApplication>
#include <QDir>
#include <QDate>

// DCMTK
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcdeftag.h>

// ---------------- Конструктор ----------------
Lib4DICOM::Lib4DICOM(QObject* parent) : QAbstractListModel(parent) {}


// ---------------- Реализация модели ----------------

// Количество строк в модели (равно количеству пациентов)
int Lib4DICOM::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_patients.size();
}

// Доступ к данным для QML: возвращаем поле пациента по роли
QVariant Lib4DICOM::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_patients.size())
        return {};
    const auto& p = m_patients[index.row()];
    switch (role) {
    case FullNameRole:  return p.fullName;   // ФИО
    case BirthYearRole: return p.birthYear;  // Год рождения
    case SexRole:       return p.sex;        // Пол
    }
    return {};
}

// Определение имён ролей для QML (чтобы в QML писать fullName, birthYear, sex)
QHash<int, QByteArray> Lib4DICOM::roleNames() const {
    return {
        { FullNameRole, "fullName" },
        { BirthYearRole,"birthYear"},
        { SexRole,      "sex"      }
    };
}

// Для QML: возвращаем саму модель
QAbstractItemModel* Lib4DICOM::patientModel() {
    return this;
}


// ---------------- Парсинг пациентов ----------------

// Метод, который вызывается из QML: сканируем папку /patients рядом с exe
void Lib4DICOM::scanPatients() {
    beginResetModel();
    m_patients.clear();

    QDir root(QCoreApplication::applicationDirPath() + "/patients");
    if (!root.exists())
        root.mkpath(".");

    // Файлы в корне (можно добавить фильтр по расширению)
    const QStringList nameFilters = { "*.dcm", "*.DCM" };
    const QFileInfoList files = root.entryInfoList(
        nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    // Подпапки (один уровень)
    const QFileInfoList dirs = root.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    auto processFile = [&](const QString& path) {
        // ВАЖНО: на Windows использовать нативную кодировку
        const QByteArray native = QFile::encodeName(path);
        DcmFileFormat ff;
        if (ff.loadFile(native.constData()).good()) {
            auto* ds = ff.getDataset();
            Patient p;

            OFString v;
            if (ds->findAndGetOFString(DCM_PatientName, v).good())
                p.fullName = QString::fromLatin1(v.c_str()).replace("^", " ");
            else
                p.fullName = "--";

            if (ds->findAndGetOFString(DCM_PatientBirthDate, v).good() && v.length() >= 4)
                p.birthYear = QString::fromLatin1(v.c_str()).left(4);
            else
                p.birthYear = "--";

            if (ds->findAndGetOFString(DCM_PatientSex, v).good())
                p.sex = QString::fromLatin1(v.c_str());
            else
                p.sex = "--";

            m_patients.push_back(p);
        } /* else: не DICOM/не открылся — пропускаем */
        };

    // Файлы в корне
    for (const QFileInfo& f : files)
        processFile(f.absoluteFilePath());

    // Файлы в подпапках (только один уровень)
    for (const QFileInfo& d : dirs) {
        QDir sub(d.absoluteFilePath());
        const QFileInfoList subFiles = sub.entryInfoList(
            nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& f : subFiles)
            processFile(f.absoluteFilePath());
    }

    endResetModel();
    emit patientModelChanged();
}
