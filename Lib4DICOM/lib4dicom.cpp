#include "lib4dicom.h"

#include <QCoreApplication>
#include <QDir>
#include <QDate>

// DCMTK
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcdeftag.h>

// ---------------- ����������� ----------------
Lib4DICOM::Lib4DICOM(QObject* parent) : QAbstractListModel(parent) {}


// ---------------- ���������� ������ ----------------

// ���������� ����� � ������ (����� ���������� ���������)
int Lib4DICOM::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_patients.size();
}

// ������ � ������ ��� QML: ���������� ���� �������� �� ����
QVariant Lib4DICOM::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_patients.size())
        return {};
    const auto& p = m_patients[index.row()];
    switch (role) {
    case FullNameRole:  return p.fullName;   // ���
    case BirthYearRole: return p.birthYear;  // ��� ��������
    case SexRole:       return p.sex;        // ���
    }
    return {};
}

// ����������� ��� ����� ��� QML (����� � QML ������ fullName, birthYear, sex)
QHash<int, QByteArray> Lib4DICOM::roleNames() const {
    return {
        { FullNameRole, "fullName" },
        { BirthYearRole,"birthYear"},
        { SexRole,      "sex"      }
    };
}

// ��� QML: ���������� ���� ������
QAbstractItemModel* Lib4DICOM::patientModel() {
    return this;
}


// ---------------- ������� ��������� ----------------

// �����, ������� ���������� �� QML: ��������� ����� /patients ����� � exe
void Lib4DICOM::scanPatients() {
    beginResetModel();
    m_patients.clear();

    QDir root(QCoreApplication::applicationDirPath() + "/patients");
    if (!root.exists())
        root.mkpath(".");

    // ����� � ����� (����� �������� ������ �� ����������)
    const QStringList nameFilters = { "*.dcm", "*.DCM" };
    const QFileInfoList files = root.entryInfoList(
        nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    // �������� (���� �������)
    const QFileInfoList dirs = root.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    auto processFile = [&](const QString& path) {
        // �����: �� Windows ������������ �������� ���������
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
        } /* else: �� DICOM/�� �������� � ���������� */
        };

    // ����� � �����
    for (const QFileInfo& f : files)
        processFile(f.absoluteFilePath());

    // ����� � ��������� (������ ���� �������)
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
