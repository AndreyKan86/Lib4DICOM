#include "lib4dicom.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QByteArray>
#include <QBuffer>
#include <QImageReader>
#include <QDebug>
#include <QRegularExpression>

// DCMTK
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmdata/dcvrda.h>
#include <dcmtk/dcmdata/dcvrtm.h>

// ---------------- Конструктор ----------------
Lib4DICOM::Lib4DICOM(QObject* parent) : QAbstractListModel(parent) {}

// ---------------- Реализация модели ----------------
int Lib4DICOM::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_patients.size();
}

QVariant Lib4DICOM::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_patients.size())
        return {};
    const auto& p = m_patients[index.row()];
    switch (role) {
    case FullNameRole:  return p.fullName;
    case BirthYearRole: return p.birthYear;
    case SexRole:       return p.sex;
    }
    return {};
}

QHash<int, QByteArray> Lib4DICOM::roleNames() const {
    return {
        { FullNameRole, "fullName" },
        { BirthYearRole,"birthYear"},
        { SexRole,      "sex"      }
    };
}

QAbstractItemModel* Lib4DICOM::patientModel() {
    return this;
}

// ---------------- Нормализация ввода ----------------
QString Lib4DICOM::normalizeBirthYear(const QString& birthInput) {
    QString digits = birthInput;
    digits.remove(QRegularExpression(QStringLiteral(R"([^0-9])")));
    if (digits.size() >= 4) return digits.left(4);
    return QStringLiteral("--");
}

QString Lib4DICOM::normalizeSex(const QString& sexInput) {
    QString s = sexInput.trimmed().toUpper();
    if (s == "M" || s == "М" || s.startsWith("МУЖ")) return "M";
    if (s == "F" || s == "Ж" || s.startsWith("ЖЕН")) return "F";
    if (s == "O" || s == "ДРУГОЕ" || s == "НЕ УКАЗАНО") return "O";
    if (s == "MALE") return "M";
    if (s == "FEMALE") return "F";
    if (s == "OTHER" || s == "UNSPECIFIED") return "O";
    return "O";
}

QString Lib4DICOM::normalizeID(const QString& idInput) {
    QString t = idInput.trimmed();
    t.replace(QRegularExpression(R"([^A-Za-z0-9_\-\.])"), "");
    if (t.isEmpty()) t = "--";
    if (t.size() > 64) t = t.left(64);
    return t;
}

QVariantMap Lib4DICOM::makePatientFromStrings(const QString& fullName,
    const QString& birthInput,
    const QString& sexInput,
    const QString& patientID)
{
    Patient p;
    p.fullName = fullName.trimmed().isEmpty() ? QStringLiteral("--") : fullName.trimmed();
    p.birthYear = normalizeBirthYear(birthInput);
    p.sex = normalizeSex(sexInput);
    p.patientID = normalizeID(patientID);

    qDebug().noquote() << "[Lib4DICOM] Converted patient:"
        << "fullName=" << p.fullName
        << "birthYear=" << p.birthYear
        << "sex=" << p.sex
        << "patientID=" << p.patientID;

    QVariantMap map;
    map["fullName"] = p.fullName;
    map["birthYear"] = p.birthYear;
    map["sex"] = p.sex;
    map["patientID"] = p.patientID;
    return map;
}

void Lib4DICOM::logSelectedFileAndPatient(const QString& filePath,
    const QString& fullName,
    const QString& birthYear,
    const QString& sex)
{
    qDebug().noquote() << "[Lib4DICOM] Selected file:" << filePath;
    qDebug().noquote() << "[Lib4DICOM] Patient:"
        << "fullName =" << (fullName.isEmpty() ? "--" : fullName) << ","
        << "birthYear =" << (birthYear.isEmpty() ? "--" : birthYear) << ","
        << "sex =" << (sex.isEmpty() ? "--" : sex);
}

// ---------------- ФС пациента/исследования ----------------
QString Lib4DICOM::makeSafeFolderName(const QString& s)
{
    QString t = s.trimmed();
    t.replace(QRegularExpression(R"([\\\/\:\*\?\"\<\>\|\n\r\t])"), "_");
    t.replace(QRegularExpression(R"(\s+)"), " ");
    t.replace(' ', '_');
    const int kMax = 80;
    if (t.size() > kMax) t = t.left(kMax);
    if (t.isEmpty()) t = "Unnamed";
    return t;
}

QString Lib4DICOM::ensurePatientFolder(const QString& fullName,
    const QString& birthYearNormalized)
{
    const QString root = QCoreApplication::applicationDirPath() + "/patients";
    QDir rootDir(root);
    if (!rootDir.exists() && !rootDir.mkpath(".")) {
        qWarning().noquote() << "[Lib4DICOM] ensurePatientFolder: cannot create root:" << root;
        return {};
    }

    const QString safeName = makeSafeFolderName(fullName.isEmpty() ? "Unnamed" : fullName);
    const QString yearPart = (birthYearNormalized.size() == 4) ? birthYearNormalized : QStringLiteral("----");

    QString base = safeName + "_" + yearPart;
    QString candidate = root + "/" + base;

    int n = 2;
    while (QDir(candidate).exists()) {
        candidate = root + "/" + base + "_" + QString::number(n++);
        if (n > 9999) {
            qWarning() << "[Lib4DICOM] ensurePatientFolder: too many duplicates for" << base;
            return {};
        }
    }

    if (!QDir().mkpath(candidate)) {
        qWarning().noquote() << "[Lib4DICOM] ensurePatientFolder: failed to create:" << candidate;
        return {};
    }

    qDebug().noquote() << "[Lib4DICOM] ensurePatientFolder: created ->" << candidate;
    return candidate;
}

QString Lib4DICOM::generateDicomUID()
{
    char uid[128] = { 0 };
    dcmGenerateUniqueIdentifier(uid);
    return QString::fromLatin1(uid);
}

QVariantMap Lib4DICOM::createStudyForNewPatient(const QString& fullName,
    const QString& birthYearNormalized,
    const QString& patientID)
{
    QVariantMap out;

    const QString patientFolder = ensurePatientFolder(fullName, birthYearNormalized);
    if (patientFolder.isEmpty()) {
        qWarning().noquote() << "[Lib4DICOM] createStudyForNewPatient: patient folder not created";
        return out;
    }
    out["patientFolder"] = patientFolder;

    const QString safeID = normalizeID(patientID);
    const QString dateStr = QDate::currentDate().toString("yyyyMMdd");
    const QString base = safeID + "_" + dateStr;

    QString studyFolder = patientFolder + "/" + base;
    int n = 2;
    while (QDir(studyFolder).exists()) {
        studyFolder = patientFolder + "/" + base + "_" + QString::number(n++);
        if (n > 9999) {
            qWarning().noquote() << "[Lib4DICOM] createStudyForNewPatient: too many duplicates for study";
            return out;
        }
    }
    if (!QDir().mkpath(studyFolder)) {
        qWarning().noquote() << "[Lib4DICOM] createStudyForNewPatient: failed to create study folder:" << studyFolder;
        return out;
    }

    const QString studyUID = generateDicomUID();

    qDebug().noquote() << "[Lib4DICOM] Study created:"
        << "\n  patientFolder =" << patientFolder
        << "\n  studyFolder   =" << studyFolder
        << "\n  studyName     =" << QFileInfo(studyFolder).fileName()
        << "\n  studyUID      =" << studyUID;

    out["studyFolder"] = studyFolder;
    out["studyName"] = QFileInfo(studyFolder).fileName();
    out["studyUID"] = studyUID;
    out["patientFolder"] = patientFolder;
    return out;
}

// ---------------- Загрузка изображения ----------------
QImage Lib4DICOM::loadImageFromFile(const QString& localPath)
{
    QFileInfo fi(localPath);
    if (!fi.exists() || !fi.isFile()) {
        qWarning().noquote() << "[Lib4DICOM] loadImageFromFile: file does not exist:" << localPath;
        return QImage();
    }

    QImageReader reader(localPath);
    reader.setAutoTransform(true);
    const QImage img = reader.read();
    if (img.isNull()) {
        qWarning().noquote() << "[Lib4DICOM] loadImageFromFile: failed to read:"
            << localPath << " error:" << reader.errorString();
        return QImage();
    }

    qDebug().noquote() << "[Lib4DICOM] loadImageFromFile: loaded"
        << fi.fileName() << img.width() << "x" << img.height()
        << "format:" << reader.format();
    return img;
}

QVector<QImage> Lib4DICOM::loadImageVectorFromFile(const QString& localPath)
{
    QVector<QImage> result;
    QImage img = loadImageFromFile(localPath);
    if (!img.isNull())
        result.push_back(img);
    return result;
}

// ---------------- Хелпер пиксельного формата ----------------
QImage Lib4DICOM::toRgb888(const QImage& src)
{
    if (src.format() == QImage::Format_RGB888) return src;
    return src.convertToFormat(QImage::Format_RGB888);
}


// ---------------- Хелперы преобразования к DICOM-форматам ----------------
QString Lib4DICOM::toDicomPN(const QString& fullName)
{
    QString s = fullName.trimmed();
    s.replace(QRegularExpression(R"(\s+)"), " ");
    s.replace(' ', '^');              // Иванов Иван -> Иванов^Иван
    if (s.isEmpty()) s = "--";
    return s;
}

QString Lib4DICOM::toDicomDA(const QString& birthInput)
{
    QString d = birthInput;
    d.remove(QRegularExpression(R"([^0-9])"));
    if (d.size() == 4 || d.size() == 8) return d;   // "YYYY" или "YYYYMMDD"
    return {};
}

QString Lib4DICOM::toDicomSex(const QString& sexInput)
{
    const QString s = normalizeSex(sexInput);
    if (s == "M" || s == "F" || s == "O") return s;
    return "O";
}

// ---------------- Комбайн с демографией ----------------
QVariantMap Lib4DICOM::convertAndSaveImageAsDicom(const QString& imagePath,
    const QString& studyFolder,
    const QString& patientID,
    const QString& seriesName,
    const QString& studyUID,
    const QString& patientName,
    const QString& patientBirth,
    const QString& patientSex)
{
    QVector<QImage> vec;
    QImage img = loadImageFromFile(imagePath);
    if (img.isNull()) {
        QVariantMap r; r["ok"] = false; r["error"] = "failed to load image"; return r;
    }
    vec.push_back(img);
    return saveImagesAsDicom(vec, studyFolder, patientID, seriesName, studyUID,
        patientName, patientBirth, patientSex);
}

// ---------------- Декодер строк из DICOM с учётом кодировки ----------------
QString Lib4DICOM::decodeDicomText(const OFString& value, const OFString& specificCharacterSet)
{
    if (value.empty())
        return QString();

    QByteArray bytes(value.c_str(), int(value.length()));
    const QString charset = QString::fromLatin1(specificCharacterSet.c_str());

    if (charset == "ISO_IR 192") {             // UTF-8
        return QString::fromUtf8(bytes);
    }
    else if (charset.isEmpty() || charset == "ISO_IR 100") {
        return QString::fromLatin1(bytes);     // Latin-1 (по умолчанию)
    }
    else {
        // На всякий случай — пробуем как UTF-8
        return QString::fromUtf8(bytes);
    }
}

// ---------------- Сканирование пациентов (чтение DICOM, поддержка UTF-8) ----------------
void Lib4DICOM::scanPatients() {
    beginResetModel();
    m_patients.clear();

    QDir root(QCoreApplication::applicationDirPath() + "/patients");
    if (!root.exists())
        root.mkpath(".");

    const QStringList nameFilters = { "*.dcm", "*.DCM" };

    // Соберём все .dcm в /patients и всех подкаталогах первого уровня
    QFileInfoList files = root.entryInfoList(nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    const QFileInfoList dirs = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& d : dirs) {
        QDir sub(d.absoluteFilePath());
        const QFileInfoList subFiles = sub.entryInfoList(nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        files.append(subFiles);
    }

    auto patientFolderOf = [](const QString& filePath) -> QString {
        QFileInfo fi(filePath);
        QDir d = fi.dir();
        // эвристика: если текущая папка похожа на study (ID_YYYYMMDD...), берём родителя
        if (d.dirName().contains('_'))
            d.cdUp();
        return d.absolutePath();
        };

    QHash<QString, Patient> uniq;  // ключ -> Patient
    auto makeKey = [](const Patient& p, const QString& fallbackFolder) -> QString {
        const QString pid = p.patientID.trimmed();
        if (!pid.isEmpty() && pid != "--")
            return "pid:" + pid;
        // если нет ID — склеим по демографии
        return QString("pn:%1|by:%2|sx:%3|pf:%4")
            .arg(p.fullName.isEmpty() ? "--" : p.fullName)
            .arg(p.birthYear.isEmpty() ? "--" : p.birthYear)
            .arg(p.sex.isEmpty() ? "--" : p.sex)
            .arg(fallbackFolder);
        };

    for (const QFileInfo& fi : files) {
        const QString path = fi.absoluteFilePath();

        DcmFileFormat ff;
        if (!ff.loadFile(QFile::encodeName(path).constData()).good())
            continue;

        DcmDataset* ds = ff.getDataset();
        OFString v, cs;
        ds->findAndGetOFString(DCM_SpecificCharacterSet, cs);

        Patient p;
        // Name
        if (ds->findAndGetOFString(DCM_PatientName, v).good())
            p.fullName = decodeDicomText(v, cs).replace("^", " ");
        else
            p.fullName = "--";
        // Birth (год)
        if (ds->findAndGetOFString(DCM_PatientBirthDate, v).good() && v.length() >= 4)
            p.birthYear = decodeDicomText(v, cs).left(4);
        else
            p.birthYear = "--";
        // Sex
        if (ds->findAndGetOFString(DCM_PatientSex, v).good())
            p.sex = decodeDicomText(v, cs);
        else
            p.sex = "--";
        // PatientID
        if (ds->findAndGetOFString(DCM_PatientID, v).good())
            p.patientID = decodeDicomText(v, cs);
        else
            p.patientID = "--";

        p.sourceFilePath = path;
        const QString pf = patientFolderOf(path);
        const QString key = makeKey(p, pf);

        // добавляем только первый раз; при желании можно обновлять p.dcmPath на stub, если встретился
        if (!uniq.contains(key)) {
            uniq.insert(key, p);
        }
    }

    // Переносим в модель
    m_patients = uniq.values();

    endResetModel();
    emit patientModelChanged();
}


// ---------------- Stub DICOM в корне папки пациента ----------------
QVariantMap Lib4DICOM::createPatientStubDicom(const QString& patientFolder,
    const QString& patientID,
    const QString& patientName,
    const QString& patientBirth,
    const QString& patientSex)
{
    QVariantMap out;
    if (patientFolder.isEmpty() || !QDir(patientFolder).exists()) {
        out["ok"] = false; out["error"] = "patient folder does not exist"; return out;
    }

    // Нормализованные строки (UTF-8)
    const QByteArray baPN = toDicomPN(patientName).toUtf8();
    const QByteArray baPID = normalizeID(patientID).toUtf8();
    const QString    da = toDicomDA(patientBirth);
    const QByteArray baDA = da.toUtf8();
    const QByteArray baSex = toDicomSex(patientSex).toUtf8();

    // UID'ы и время/дата
    const QString studyUID = generateDicomUID();
    const QString seriesUID = generateDicomUID();
    const QString sopUID = generateDicomUID();

    const QString studyDate = QDate::currentDate().toString("yyyyMMdd");
    const QString studyTime = QTime::currentTime().toString("HHmmss");

    // Минимальный 1×1 кадр
    const int rows = 1, cols = 1;
    QByteArray pixelData(1, char(0));
    if (pixelData.size() % 2 != 0) pixelData.append('\0'); // чётная VL

    DcmFileFormat file;
    DcmDataset* ds = file.getDataset();

    // ВАЖНО: объявляем кодировку строк (UTF-8)
    ds->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192");

    // Идентификаторы (используем константу UID напрямую)
    ds->putAndInsertString(DCM_SOPClassUID, UID_SecondaryCaptureImageStorage);
    ds->putAndInsertString(DCM_SOPInstanceUID, sopUID.toLatin1().constData());
    ds->putAndInsertString(DCM_StudyInstanceUID, studyUID.toLatin1().constData());
    ds->putAndInsertString(DCM_SeriesInstanceUID, seriesUID.toLatin1().constData());

    // Демография
    ds->putAndInsertString(DCM_PatientID, baPID.constData());
    ds->putAndInsertString(DCM_PatientName, baPN.constData());
    ds->putAndInsertString(DCM_PatientSex, baSex.constData());
    if (!baDA.isEmpty())
        ds->putAndInsertString(DCM_PatientBirthDate, baDA.constData());

    // Даты/время и нумерация
    ds->putAndInsertString(DCM_StudyDate, studyDate.toLatin1().constData());
    ds->putAndInsertString(DCM_StudyTime, studyTime.toLatin1().constData());
    ds->putAndInsertString(DCM_SeriesDate, studyDate.toLatin1().constData());
    ds->putAndInsertString(DCM_SeriesTime, studyTime.toLatin1().constData());
    ds->putAndInsertUint16(DCM_SeriesNumber, 0);
    ds->putAndInsertUint16(DCM_InstanceNumber, 1);

    // Image Pixel Module (MONOCHROME2, 8-бит)
    ds->putAndInsertUint16(DCM_Rows, rows);
    ds->putAndInsertUint16(DCM_Columns, cols);
    ds->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2");
    ds->putAndInsertUint16(DCM_SamplesPerPixel, 1);
    ds->putAndInsertUint16(DCM_BitsAllocated, 8);
    ds->putAndInsertUint16(DCM_BitsStored, 8);
    ds->putAndInsertUint16(DCM_HighBit, 7);
    ds->putAndInsertUint16(DCM_PixelRepresentation, 0);

    ds->putAndInsertUint8Array(DCM_PixelData,
        reinterpret_cast<const Uint8*>(pixelData.constData()),
        static_cast<unsigned long>(pixelData.size()));

    // Служебные поля
    ds->putAndInsertString(DCM_ConversionType, "WSD");
    ds->putAndInsertString(DCM_SeriesDescription, "PATIENT_STUB");

    // Имя файла с защитой от коллизий
    QString base = normalizeID(patientID);
    if (base.isEmpty()) base = "--";
    QString fileName = base + "_patient.dcm";
    QString absPath = QDir(patientFolder).absoluteFilePath(fileName);

    int k = 2;
    while (QFileInfo::exists(absPath) && k < 1000) {
        fileName = QString("%1_patient_%2.dcm").arg(base).arg(k++);
        absPath = QDir(patientFolder).absoluteFilePath(fileName);
    }

    const OFCondition st = file.saveFile(absPath.toLocal8Bit().constData(),
        EXS_LittleEndianExplicit,
        EET_ExplicitLength,
        EGL_recalcGL,
        EPD_withoutPadding);
    if (st.good()) {
        qDebug().noquote() << "[Lib4DICOM] patient stub saved:" << absPath;
        out["ok"] = true;
        out["path"] = absPath;
    }
    else {
        qWarning().noquote() << "[Lib4DICOM] patient stub save failed:" << st.text();
        out["ok"] = false;
        out["error"] = QString::fromLatin1(st.text());
    }
    return out;
}



// ---------------- Сохранение DICOM (SC) — полная версия с демографией ----------------
QVariantMap Lib4DICOM::saveImagesAsDicom(const QVector<QImage>& images,
    const QString& outFolder,
    const QString& patientID,
    const QString& seriesName,
    const QString& studyUIDIn,
    const QString& patientName,
    const QString& patientBirth,
    const QString& patientSex)
{
    QVariantMap result;
    QStringList outFiles;
    int saved = 0;

    if (images.isEmpty()) {
        result["ok"] = false; result["error"] = "images is empty"; return result;
    }

    QDir dir(outFolder);
    if (outFolder.isEmpty() || !dir.exists()) {
        result["ok"] = false; result["error"] = "output folder does not exist"; return result;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const QString   studyDate = now.date().toString("yyyyMMdd");
    const QString   studyTime = now.time().toString("HHmmss");

    const QString studyUID = studyUIDIn.isEmpty() ? generateDicomUID() : studyUIDIn;
    const QString seriesUID = generateDicomUID();

    const QString idToken = patientID.isEmpty() ? QStringLiteral("--") : normalizeID(patientID);
    const QString seriesToken = seriesName.isEmpty() ? QStringLiteral("SER") : normalizeID(seriesName);

    // Демография: используем UTF-8 и объявляем SpecificCharacterSet
    const QByteArray baPN = toDicomPN(patientName).toUtf8();
    const QByteArray baPID = idToken.toUtf8();
    const QString    da = toDicomDA(patientBirth);
    const QByteArray baDA = da.toUtf8();               // может быть пустым
    const QByteArray baSex = toDicomSex(patientSex).toUtf8();

    // Хелпер подготовки «плотного» буфера
    auto makeDenseBuffer = [](const QImage& in,
        QByteArray& pixelData,
        int& rows, int& cols,
        int& samplesPerPixel,
        int& bitsAllocated, int& bitsStored, int& highBit,
        int& planarConfig,
        int& pixelRepr,
        const char*& photometric) -> bool
        {
            QImage img = in;

            samplesPerPixel = 3;
            bitsAllocated = 8;
            bitsStored = 8;
            highBit = 7;
            planarConfig = 0; // interleaved RGB
            pixelRepr = 0; // unsigned
            photometric = "RGB";

            if (img.format() == QImage::Format_Grayscale8) {
                samplesPerPixel = 1;
                bitsAllocated = 8;
                bitsStored = 8;
                highBit = 7;
                photometric = "MONOCHROME2";
            }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
            else if (img.format() == QImage::Format_Grayscale16) {
                samplesPerPixel = 1;
                bitsAllocated = 16;
                bitsStored = 16;
                highBit = 15;
                photometric = "MONOCHROME2";
            }
#endif
            else {
                img = img.convertToFormat(QImage::Format_RGB888);
            }

            rows = img.height();
            cols = img.width();

            const int srcStride = img.bytesPerLine();
            const int dstStride = cols * samplesPerPixel * (bitsAllocated / 8);

            pixelData.clear();
            pixelData.resize(rows * dstStride);

            const uchar* src = img.constBits();
            uchar* dst = reinterpret_cast<uchar*>(pixelData.data());

            for (int y = 0; y < rows; ++y) {
                memcpy(dst + y * dstStride, src + y * srcStride, dstStride);
            }

            // DICOM: VL должна быть чётной
            if (pixelData.size() % 2 != 0) pixelData.append('\0');
            return true;
        };

    for (int i = 0; i < images.size(); ++i) {
        if (images[i].isNull()) {
            qWarning().noquote() << "[Lib4DICOM] saveImagesAsDicom(full): image" << i << "is null";
            continue;
        }

        QByteArray pixelData;
        int rows = 0, cols = 0;
        int samplesPerPixel = 0;
        int bitsAllocated = 0, bitsStored = 0, highBit = 0;
        int planarConfig = 0;
        int pixelRepr = 0;
        const char* photometric = "RGB";

        if (!makeDenseBuffer(images[i], pixelData, rows, cols,
            samplesPerPixel, bitsAllocated, bitsStored, highBit,
            planarConfig, pixelRepr, photometric)) {
            qWarning().noquote() << "[Lib4DICOM] saveImagesAsDicom(full): failed to build pixel buffer for image" << i;
            continue;
        }

        DcmFileFormat file;
        DcmDataset* ds = file.getDataset();

        // Говорим, что все текстовые поля в UTF-8
        ds->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192");

        const QString sopInstanceUID = generateDicomUID();
        ds->putAndInsertString(DCM_SOPClassUID, UID_SecondaryCaptureImageStorage);
        ds->putAndInsertString(DCM_SOPInstanceUID, sopInstanceUID.toLatin1().constData());
        ds->putAndInsertString(DCM_StudyInstanceUID, studyUID.toLatin1().constData());
        ds->putAndInsertString(DCM_SeriesInstanceUID, seriesUID.toLatin1().constData());

        // Пациент
        ds->putAndInsertString(DCM_PatientID, baPID.constData());
        ds->putAndInsertString(DCM_PatientName, baPN.constData());
        ds->putAndInsertString(DCM_PatientSex, baSex.constData());
        if (!baDA.isEmpty()) ds->putAndInsertString(DCM_PatientBirthDate, baDA.constData());

        // Даты/время и номера
        ds->putAndInsertString(DCM_StudyDate, studyDate.toLatin1().constData());
        ds->putAndInsertString(DCM_StudyTime, studyTime.toLatin1().constData());
        ds->putAndInsertString(DCM_SeriesDate, studyDate.toLatin1().constData());
        ds->putAndInsertString(DCM_SeriesTime, studyTime.toLatin1().constData());
        ds->putAndInsertUint16(DCM_SeriesNumber, 1);
        ds->putAndInsertUint16(DCM_InstanceNumber, static_cast<Uint16>(i + 1));

        // Пиксельный модуль
        ds->putAndInsertUint16(DCM_Rows, rows);
        ds->putAndInsertUint16(DCM_Columns, cols);
        ds->putAndInsertString(DCM_PhotometricInterpretation, photometric);
        ds->putAndInsertUint16(DCM_SamplesPerPixel, samplesPerPixel);
        ds->putAndInsertUint16(DCM_BitsAllocated, bitsAllocated);
        ds->putAndInsertUint16(DCM_BitsStored, bitsStored);
        ds->putAndInsertUint16(DCM_HighBit, highBit);
        ds->putAndInsertUint16(DCM_PixelRepresentation, pixelRepr);
        if (samplesPerPixel == 3)
            ds->putAndInsertUint16(DCM_PlanarConfiguration, 0);

        // Пиксели
        ds->putAndInsertUint8Array(DCM_PixelData,
            reinterpret_cast<const Uint8*>(pixelData.constData()),
            static_cast<unsigned long>(pixelData.size()));

        // Служебное
        ds->putAndInsertString(DCM_ConversionType, "WSD");
        ds->putAndInsertString(DCM_SeriesDescription, seriesToken.toUtf8().constData());

        // Имя файла
        const QString fileName = QString("%1_%2_%3_%4_%5.dcm")
            .arg(idToken)
            .arg(seriesToken)
            .arg(studyDate)
            .arg(studyTime)
            .arg(i + 1, 3, 10, QChar('0'));
        const QString absPath = dir.absoluteFilePath(fileName);

        const OFCondition st = file.saveFile(absPath.toLocal8Bit().constData(),
            EXS_LittleEndianExplicit,
            EET_ExplicitLength,
            EGL_recalcGL,
            EPD_withoutPadding);
        if (st.good()) {
            ++saved;
            outFiles << absPath;
            qDebug().noquote() << "[Lib4DICOM] saved(full):" << absPath;
        }
        else {
            qWarning().noquote() << "[Lib4DICOM] save failed(full) for" << absPath << ":" << st.text();
        }
    }

    result["ok"] = (saved == images.size());
    result["saved"] = saved;
    result["studyUID"] = studyUID;
    result["seriesUID"] = seriesUID;
    result["files"] = outFiles;
    if (saved != images.size())
        result["error"] = QString("saved %1 of %2").arg(saved).arg(images.size());
    return result;
}

QVariantMap Lib4DICOM::getPatientDemographics(int index) const {
    QVariantMap out;
    if (index < 0 || index >= m_patients.size()) {
        out["ok"] = false; out["error"] = "index out of range"; return out;
    }
    const auto& p = m_patients.at(index);
    out["ok"] = true;
    out["fullName"] = p.fullName;
    out["birthYear"] = p.birthYear;
    out["sex"] = p.sex;
    out["patientID"] = p.patientID;

    // Определяем папку пациента: если файл лежит в подкаталоге исследования — поднимаемся на уровень выше
    QFileInfo fi(p.sourceFilePath);
    QDir d = fi.dir();
    QString patientFolder = d.absolutePath();
    // Если в текущей папке есть много DICOMов и имя файла не похоже на «root-level», можно при желании cdUp()
    // Простая и надёжная эвристика:
    if (d.dirName().contains('_')) { // чаще study-папка типа P1234_YYYYMMDD
        d.cdUp();
        patientFolder = d.absolutePath();
    }
    out["patientFolder"] = patientFolder;
    return out;
}


QVariantMap Lib4DICOM::createStudyInPatientFolder(const QString& patientFolder,
    const QString& patientID) {
    QVariantMap out;
    if (patientFolder.isEmpty() || !QDir(patientFolder).exists()) {
        out["ok"] = false; out["error"] = "patient folder does not exist"; return out;
    }

    const QString safeID = normalizeID(patientID);
    const QString dateStr = QDate::currentDate().toString("yyyyMMdd");
    const QString base = safeID + "_" + dateStr;

    QString studyFolder = QDir(patientFolder).absoluteFilePath(base);
    int n = 2;
    while (QDir(studyFolder).exists() && n <= 9999) {
        studyFolder = QDir(patientFolder).absoluteFilePath(base + "_" + QString::number(n++));
    }
    if (!QDir().mkpath(studyFolder)) {
        out["ok"] = false; out["error"] = "failed to create study folder"; return out;
    }

    out["ok"] = true;
    out["patientFolder"] = patientFolder;
    out["studyFolder"] = studyFolder;
    out["studyName"] = QFileInfo(studyFolder).fileName();
    out["studyUID"] = generateDicomUID();
    return out;
}


QVariantMap Lib4DICOM::findPatientStubByIndex(int index) const {
    QVariantMap out; out["ok"] = false;
    if (index < 0 || index >= m_patients.size()) { out["error"] = "index out of range"; return out; }

    const QString anyPath = m_patients.at(index).sourceFilePath;
    const QString wantedPID = m_patients.at(index).patientID.trimmed();
    if (anyPath.isEmpty() || !QFileInfo::exists(anyPath)) { out["error"] = "no source file path"; return out; }

    const QString root = QCoreApplication::applicationDirPath() + "/patients";

    // 1) Базовая папка пациента из пути файла
    QDir d = QFileInfo(anyPath).dir();
    if (d.dirName().contains('_')) d.cdUp(); // подняться из папки исследования
    QString patientFolder = d.absolutePath();

    auto tryFindStubIn = [&](const QString& folder)->QString {
        const QStringList nameFilters = { "*_patient*.dcm", "*_PATIENT*.dcm", "*_Patient*.dcm" };
        QFileInfoList stubs = QDir(folder).entryInfoList(nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        if (!stubs.isEmpty()) return stubs.first().absoluteFilePath();

        // Fallback: ищем по SeriesDescription == PATIENT_STUB
        const QFileInfoList all = QDir(folder).entryInfoList(QStringList() << "*.dcm" << "*.DCM",
            QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& fi : all) {
            DcmFileFormat ff;
            if (!ff.loadFile(QFile::encodeName(fi.absoluteFilePath()).constData()).good()) continue;
            DcmDataset* ds = ff.getDataset();
            OFString v;
            if (ds->findAndGetOFString(DCM_SeriesDescription, v).good()) {
                const QString desc = QString::fromLatin1(v.c_str());
                if (desc.compare(QStringLiteral("PATIENT_STUB"), Qt::CaseInsensitive) == 0)
                    return fi.absoluteFilePath();
            }
        }
        return QString();
        };

    // 2) Сначала пытаемся в вычисленной папке
    QString stubPath = tryFindStubIn(patientFolder);

    // 3) Если это оказался корень /patients — пробегаемся по подпапкам и ищем по PatientID
    if (stubPath.isEmpty() && QDir::cleanPath(patientFolder) == QDir::cleanPath(root)) {
        const QFileInfoList subdirs = QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& sdi : subdirs) {
            const QString candidate = sdi.absoluteFilePath();

            // Быстрая проверка: есть ли в этой папке DCM с тем же PatientID
            bool pidMatch = false;
            const QFileInfoList all = QDir(candidate).entryInfoList(QStringList() << "*.dcm" << "*.DCM",
                QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
            for (const QFileInfo& fi : all) {
                DcmFileFormat ff;
                if (!ff.loadFile(QFile::encodeName(fi.absoluteFilePath()).constData()).good()) continue;

                DcmDataset* ds = ff.getDataset();
                OFString v, cs;
                ds->findAndGetOFString(DCM_SpecificCharacterSet, cs);
                if (ds->findAndGetOFString(DCM_PatientID, v).good()) {
                    const QString pid = decodeDicomText(v, cs);
                    if (!wantedPID.isEmpty() && wantedPID != "--" && pid.trimmed() == wantedPID) {
                        pidMatch = true;
                        break;
                    }
                }
            }

            if (pidMatch) {
                // Нашли папку пациента — ищем в ней stub
                stubPath = tryFindStubIn(candidate);
                if (!stubPath.isEmpty()) {
                    patientFolder = candidate;
                    break;
                }
            }
        }
    }

    if (stubPath.isEmpty()) {
        out["error"] = "stub not found";
        out["patientFolder"] = patientFolder; // для диагностики
        return out;
    }

    out["ok"] = true;
    out["patientFolder"] = patientFolder;
    out["stubPath"] = stubPath;
    return out;
}


QVariantMap Lib4DICOM::readDemographicsFromFile(const QString& dcmPath) const {
    QVariantMap out; out["ok"] = false;
    if (dcmPath.isEmpty() || !QFileInfo::exists(dcmPath)) { out["error"] = "file not found"; return out; }

    DcmFileFormat ff;
    if (!ff.loadFile(QFile::encodeName(dcmPath).constData()).good()) { out["error"] = "load failed"; return out; }
    DcmDataset* ds = ff.getDataset();

    OFString v, cs;
    ds->findAndGetOFString(DCM_SpecificCharacterSet, cs);

    auto q = [&](const DcmTagKey& tag) -> QString {
        if (ds->findAndGetOFString(tag, v).good()) return decodeDicomText(v, cs);
        return QString();
        };

    out["patientName"] = q(DCM_PatientName).replace("^", " ");
    const QString birth = q(DCM_PatientBirthDate);
    out["patientBirth"] = birth;                 // "YYYY" или "YYYYMMDD" — как есть
    out["patientSex"] = q(DCM_PatientSex);     // "M"/"F"/"O"
    out["patientID"] = q(DCM_PatientID);
    out["ok"] = true;
    return out;
}
