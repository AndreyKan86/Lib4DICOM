#include "lib4dicom.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QByteArray>
#include <QImageReader>
#include <QDebug>
#include <QRegularExpression>

// DCMTK
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/ofstd/ofstring.h>


// ---------------- Конструктор ----------------
Lib4DICOM::Lib4DICOM(QObject* parent) : QAbstractListModel(parent) {
    scanPatients();
}

// Подсчёт количества пациентов
int Lib4DICOM::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_patients.size();
}

// Возвращает данные пациента по индексу
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

// связывает таблицу с именами
QHash<int, QByteArray> Lib4DICOM::roleNames() const {
    return {
        { FullNameRole, "fullName" },
        { BirthYearRole,"birthYear"},
        { SexRole,      "sex"      }
    };
}

//Создание пациента из строк
QVariantMap Lib4DICOM::makePatientFromStrings(const QString& fullName,
    const QString& birthInput,
    const QString& sexInput,
    const QString& patientID)
{
    Patient p;
    p.fullName = fullName.trimmed().isEmpty() ? QStringLiteral("--") : fullName.trimmed();
    p.birthYear = (birthInput);
    p.sex = (sexInput);
    p.patientID = (patientID);

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

//Логгирование выбранного файла и пациента
void Lib4DICOM::TESTlogSelectedFileAndPatient(const QString& filePath,
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

//Генерация UID     
QString Lib4DICOM::generateDicomUID()
{
    char uid[128] = { 0 };
    dcmGenerateUniqueIdentifier(uid);
    return QString::fromLatin1(uid);
}

//Создание исследования для нового пациента
QVariantMap Lib4DICOM::createStudyForNewPatient(const QVariantMap& patientMap)
{
    const Patient p = patientFromMap(patientMap);

    QVariantMap out;

    // 1) Папка пациента
    const QString patientFolder = ensurePatientFolder(p.fullName, p.birthYear);
    if (patientFolder.isEmpty()) {
        qWarning().noquote() << "[Lib4DICOM] createStudyForNewPatient: patient folder not created";
        out["ok"] = false;
        out["error"] = "patient folder not created";
        return out;
    }
    out["patientFolder"] = patientFolder;

    // 2) Имя исследования = <ИмяПациента>_<Дата>_<Метка>
    const QString dateStr = QDate::currentDate().toString("yyyyMMdd");
    const QString safeName = sanitizeName(p.fullName.isEmpty() ? "Unnamed" : p.fullName);
    const QString safeLabel = sanitizeName(m_studyLabel.isEmpty() ? "Study" : m_studyLabel);
    const QString base = QString("%1_%2_%3").arg(safeName, dateStr, safeLabel);

    // 3) Создание папки исследования с авто-нумерацией
    QString studyFolder = QDir(patientFolder).absoluteFilePath(base);
    int n = 2;
    while (QDir(studyFolder).exists() && n <= 9999) {
        studyFolder = QDir(patientFolder).absoluteFilePath(base + "_" + QString::number(n++));
    }
    if (n > 9999) {
        out["ok"] = false; out["error"] = "too many duplicates for study"; return out;
    }
    if (!QDir().mkpath(studyFolder)) {
        out["ok"] = false; out["error"] = "failed to create study folder"; return out;
    }

    // 4) UID
    const QString studyUID = generateDicomUID();

    qDebug().noquote() << "[Lib4DICOM] Study created:"
        << "\n  patientFolder =" << patientFolder
        << "\n  studyFolder   =" << studyFolder
        << "\n  studyName     =" << QFileInfo(studyFolder).fileName()
        << "\n  studyUID      =" << studyUID;

    out["ok"] = true;
    out["studyFolder"] = studyFolder;
    out["studyName"] = QFileInfo(studyFolder).fileName();
    out["studyUID"] = studyUID;
    out["patientFolder"] = patientFolder;
    return out;
}


// ---------------- Загрузка изображения ----------------
QImage Lib4DICOM::TESTloadImageFromFile(const QString& localPath)
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

// ---------------- Загрузка вектор изображений (один файл - одно изображение) ----------------
QVector<QImage> Lib4DICOM::TESTloadImageVectorFromFile(const QString& localPath)
{
    QVector<QImage> result;
    QImage img = TESTloadImageFromFile(localPath);
    if (!img.isNull())
        result.push_back(img);
    return result;
}

// ---------------- Комбайн с демографией ----------------
QVariantMap Lib4DICOM::convertAndSaveImageAsDicom(const QString& imagePath,
    const QString& studyFolder,
    const QString& seriesName,
    const QString& studyUID,
    const QVariantMap& patientMap)
{
    const Patient p = patientFromMap(patientMap);
    const QString birth = p.birthRaw.isEmpty() ? p.birthYear : p.birthRaw;

    QVector<QImage> vec;
    QImage img = TESTloadImageFromFile(imagePath);
    if (img.isNull()) {
        QVariantMap r; r["ok"] = false; r["error"] = "failed to load image"; return r;
    }
    vec.push_back(img);

    // делегируем на новую saveImagesAsDicom (с patientMap)
    return saveImagesAsDicom(vec, studyFolder, seriesName, studyUID, patientMap);
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
}

//DICOM файл в корне папки пациента ----------------
QVariantMap Lib4DICOM::createPatientStubDicom(const QString& patientFolder,
    const QVariantMap& patientMap)
{
    const Patient p = patientFromMap(patientMap);
    const QString birth = p.birthRaw.isEmpty() ? p.birthYear : p.birthRaw;

    QVariantMap out;
    if (patientFolder.isEmpty() || !QDir(patientFolder).exists()) {
        out["ok"] = false; out["error"] = "patient folder does not exist"; return out;
    }

    const QByteArray baPN = p.fullName.toUtf8();
    const QByteArray baPID = p.patientID.toUtf8();
    const QByteArray baDA = birth.toUtf8();
    const QByteArray baSex = p.sex.toUtf8();

    const QString studyUID = generateDicomUID();
    const QString seriesUID = generateDicomUID();
    const QString sopUID = generateDicomUID();

    const QString studyDate = QDate::currentDate().toString("yyyyMMdd");
    const QString studyTime = QTime::currentTime().toString("HHmmss");

    const int rows = 1, cols = 1;
    QByteArray pixelData(1, char(0));
    if (pixelData.size() % 2 != 0) pixelData.append('\0');

    DcmFileFormat file;
    DcmDataset* ds = file.getDataset();

    ds->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192");
    ds->putAndInsertString(DCM_SOPClassUID, UID_SecondaryCaptureImageStorage);
    ds->putAndInsertString(DCM_SOPInstanceUID, sopUID.toLatin1().constData());
    ds->putAndInsertString(DCM_StudyInstanceUID, studyUID.toLatin1().constData());
    ds->putAndInsertString(DCM_SeriesInstanceUID, seriesUID.toLatin1().constData());

    ds->putAndInsertString(DCM_PatientID, baPID.constData());
    ds->putAndInsertString(DCM_PatientName, baPN.constData());
    ds->putAndInsertString(DCM_PatientSex, baSex.constData());
    if (!baDA.isEmpty()) ds->putAndInsertString(DCM_PatientBirthDate, baDA.constData());

    ds->putAndInsertString(DCM_StudyDate, studyDate.toLatin1().constData());
    ds->putAndInsertString(DCM_StudyTime, studyTime.toLatin1().constData());
    ds->putAndInsertString(DCM_SeriesDate, studyDate.toLatin1().constData());
    ds->putAndInsertString(DCM_SeriesTime, studyTime.toLatin1().constData());
    ds->putAndInsertUint16(DCM_SeriesNumber, 0);
    ds->putAndInsertUint16(DCM_InstanceNumber, 1);

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

    ds->putAndInsertString(DCM_ConversionType, "WSD");
    ds->putAndInsertString(DCM_SeriesDescription, "PATIENT_STUB");

    QString base = p.patientID.isEmpty() ? "--" : p.patientID;
    QString fileName = base + "_patient.dcm";
    QString absPath = QDir(patientFolder).absoluteFilePath(fileName);

    int k = 2;
    while (QFileInfo::exists(absPath) && k < 1000) {
        fileName = QString("%1_patient_%2.dcm").arg(base).arg(k++);
        absPath = QDir(patientFolder).absoluteFilePath(fileName);
    }

    const OFCondition st = file.saveFile(absPath.toLocal8Bit().constData(),
        EXS_LittleEndianExplicit, EET_ExplicitLength, EGL_recalcGL, EPD_withoutPadding);
    if (st.good()) {
        qDebug().noquote() << "[Lib4DICOM] patient stub saved:" << absPath;
        out["ok"] = true; out["path"] = absPath;
    }
    else {
        qWarning().noquote() << "[Lib4DICOM] patient stub save failed:" << st.text();
        out["ok"] = false; out["error"] = QString::fromLatin1(st.text());
    }
    return out;
}



// ---------------- Сохранение DICOM (SC) — полная версия с демографией ----------------
QVariantMap Lib4DICOM::saveImagesAsDicom(const QVector<QImage>& images,
    const QString& outFolder,
    const QString& seriesName,
    const QString& studyUIDIn,
    const QVariantMap& patientMap)
{
    const Patient p = patientFromMap(patientMap);
    const QString birth = p.birthRaw.isEmpty() ? p.birthYear : p.birthRaw;

    QVariantMap result;
    QStringList outFiles;
    int saved = 0;

    if (images.isEmpty()) { result["ok"] = false; result["error"] = "images is empty"; return result; }

    QDir dir(outFolder);
    if (outFolder.isEmpty() || !dir.exists()) {
        result["ok"] = false; result["error"] = "output folder does not exist"; return result;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const QString studyDate = now.date().toString("yyyyMMdd");
    const QString studyTime = now.time().toString("HHmmss");

    const QString studyUID = studyUIDIn.isEmpty() ? generateDicomUID() : studyUIDIn;
    const QString seriesUID = generateDicomUID();

    const QString idToken = p.patientID.isEmpty() ? QStringLiteral("--") : p.patientID;
    const QString seriesToken = seriesName.isEmpty() ? QStringLiteral("SER") : seriesName;

    const QByteArray baPN = p.fullName.toUtf8();
    const QByteArray baPID = idToken.toUtf8();
    const QByteArray baDA = birth.toUtf8();
    const QByteArray baSex = p.sex.toUtf8();

    auto makeDenseBuffer = [](const QImage& in,
        QByteArray& pixelData, int& rows, int& cols,
        int& samplesPerPixel, int& bitsAllocated, int& bitsStored, int& highBit,
        int& planarConfig, int& pixelRepr, const char*& photometric)->bool
        {
            QImage img = in;

            samplesPerPixel = 3; bitsAllocated = 8; bitsStored = 8; highBit = 7;
            planarConfig = 0; pixelRepr = 0; photometric = "RGB";

            if (img.format() == QImage::Format_Grayscale8) {
                samplesPerPixel = 1; bitsAllocated = 8; bitsStored = 8; highBit = 7; photometric = "MONOCHROME2";
            }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
            else if (img.format() == QImage::Format_Grayscale16) {
                samplesPerPixel = 1; bitsAllocated = 16; bitsStored = 16; highBit = 15; photometric = "MONOCHROME2";
            }
#endif
            else {
                img = img.convertToFormat(QImage::Format_RGB888);
            }

            rows = img.height(); cols = img.width();
            const int srcStride = img.bytesPerLine();
            const int dstStride = cols * samplesPerPixel * (bitsAllocated / 8);

            pixelData.clear();
            pixelData.resize(rows * dstStride);

            const uchar* src = img.constBits();
            uchar* dst = reinterpret_cast<uchar*>(pixelData.data());
            for (int y = 0; y < rows; ++y) memcpy(dst + y * dstStride, src + y * srcStride, dstStride);

            if (pixelData.size() % 2 != 0) pixelData.append('\0');
            return true;
        };

    for (int i = 0; i < images.size(); ++i) {
        if (images[i].isNull()) { qWarning().noquote() << "[Lib4DICOM] image" << i << "is null"; continue; }

        QByteArray pixelData; int rows = 0, cols = 0;
        int samplesPerPixel = 0, bitsAllocated = 0, bitsStored = 0, highBit = 0;
        int planarConfig = 0, pixelRepr = 0; const char* photometric = "RGB";
        if (!makeDenseBuffer(images[i], pixelData, rows, cols, samplesPerPixel, bitsAllocated, bitsStored, highBit, planarConfig, pixelRepr, photometric))
            continue;

        DcmFileFormat file; DcmDataset* ds = file.getDataset();
        ds->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192");

        const QString sopInstanceUID = generateDicomUID();
        ds->putAndInsertString(DCM_SOPClassUID, UID_SecondaryCaptureImageStorage);
        ds->putAndInsertString(DCM_SOPInstanceUID, sopInstanceUID.toLatin1().constData());
        ds->putAndInsertString(DCM_StudyInstanceUID, studyUID.toLatin1().constData());
        ds->putAndInsertString(DCM_SeriesInstanceUID, seriesUID.toLatin1().constData());

        ds->putAndInsertString(DCM_PatientID, baPID.constData());
        ds->putAndInsertString(DCM_PatientName, baPN.constData());
        ds->putAndInsertString(DCM_PatientSex, baSex.constData());
        if (!baDA.isEmpty()) ds->putAndInsertString(DCM_PatientBirthDate, baDA.constData());

        ds->putAndInsertString(DCM_StudyDate, studyDate.toLatin1().constData());
        ds->putAndInsertString(DCM_StudyTime, studyTime.toLatin1().constData());
        ds->putAndInsertString(DCM_SeriesDate, studyDate.toLatin1().constData());
        ds->putAndInsertString(DCM_SeriesTime, studyTime.toLatin1().constData());
        ds->putAndInsertUint16(DCM_SeriesNumber, 1);
        ds->putAndInsertUint16(DCM_InstanceNumber, static_cast<Uint16>(i + 1));

        ds->putAndInsertUint16(DCM_Rows, rows);
        ds->putAndInsertUint16(DCM_Columns, cols);
        ds->putAndInsertString(DCM_PhotometricInterpretation, photometric);
        ds->putAndInsertUint16(DCM_SamplesPerPixel, samplesPerPixel);
        ds->putAndInsertUint16(DCM_BitsAllocated, bitsAllocated);
        ds->putAndInsertUint16(DCM_BitsStored, bitsStored);
        ds->putAndInsertUint16(DCM_HighBit, highBit);
        ds->putAndInsertUint16(DCM_PixelRepresentation, pixelRepr);
        if (samplesPerPixel == 3) ds->putAndInsertUint16(DCM_PlanarConfiguration, 0);

        ds->putAndInsertUint8Array(DCM_PixelData,
            reinterpret_cast<const Uint8*>(pixelData.constData()),
            static_cast<unsigned long>(pixelData.size()));

        ds->putAndInsertString(DCM_ConversionType, "WSD");
        ds->putAndInsertString(DCM_SeriesDescription, seriesToken.toUtf8().constData());

        const QString fileName = QString("%1_%2_%3_%4_%5.dcm")
            .arg(idToken).arg(seriesToken).arg(studyDate).arg(studyTime)
            .arg(i + 1, 3, 10, QChar('0'));
        const QString absPath = dir.absoluteFilePath(fileName);

        const OFCondition st = file.saveFile(absPath.toLocal8Bit().constData(),
            EXS_LittleEndianExplicit, EET_ExplicitLength, EGL_recalcGL, EPD_withoutPadding);
        if (st.good()) { ++saved; outFiles << absPath; }
        else { qWarning().noquote() << "[Lib4DICOM] save failed for" << absPath << ":" << st.text(); }
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



//Получение демографии пациента по индексу
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

//создание исследования в папке существующего пациента
QVariantMap Lib4DICOM::createStudyInPatientFolder(const QString& patientFolder,
    const QString& /*patientID*/) {
    QVariantMap out;

    if (patientFolder.isEmpty() || !QDir(patientFolder).exists()) {
        out["ok"] = false;
        out["error"] = "patient folder does not exist";
        return out;
    }

    // === Извлечь "имя пациента" из имени папки пациента ===
    QString dirName = QFileInfo(patientFolder).fileName();

    dirName.remove(QRegularExpression(R"(_\d{1,4}$)"));

    int us = dirName.lastIndexOf('_');
    if (us > 0) {
        const QString tail = dirName.mid(us + 1);
        const bool isYear4 = (tail.size() == 4 &&
            tail.at(0).isDigit() && tail.at(1).isDigit() &&
            tail.at(2).isDigit() && tail.at(3).isDigit());
        if (tail == "----" || isYear4) {
            dirName = dirName.left(us);
        }
    }

    const QString safeName = sanitizeName(dirName.isEmpty() ? "Unnamed" : dirName);
    const QString dateStr = QDate::currentDate().toString("yyyyMMdd");
    const QString safeLabel = sanitizeName(m_studyLabel.isEmpty() ? "Study" : m_studyLabel);

    const QString base = QString("%1_%2_%3").arg(safeName, dateStr, safeLabel);

    QString studyFolder = QDir(patientFolder).absoluteFilePath(base);
    int n = 2;
    while (QDir(studyFolder).exists() && n <= 9999) {
        studyFolder = QDir(patientFolder).absoluteFilePath(base + "_" + QString::number(n++));
    }
    if (n > 9999) {
        out["ok"] = false;
        out["error"] = "too many duplicates for study";
        return out;
    }

    if (!QDir().mkpath(studyFolder)) {
        out["ok"] = false;
        out["error"] = "failed to create study folder";
        return out;
    }

    const QString studyUID = generateDicomUID();

    qDebug().noquote() << "[Lib4DICOM] Study created:"
        << "\n  patientFolder =" << patientFolder
        << "\n  studyFolder   =" << studyFolder
        << "\n  studyName     =" << QFileInfo(studyFolder).fileName()
        << "\n  studyUID      =" << studyUID;

    out["ok"] = true;
    out["patientFolder"] = patientFolder;
    out["studyFolder"] = studyFolder;
    out["studyName"] = QFileInfo(studyFolder).fileName();
    out["studyUID"] = studyUID;
    return out;
}


//Получение пути к DICOM-файлу-заглушке пациента
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

//Чтение демографии пациента из DICOM-файла
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

// ---------------- Создание папки пациента ----------------
QString Lib4DICOM::ensurePatientFolder(const QString& fullName,
    const QString& birthYear)
{
    const QString root = QCoreApplication::applicationDirPath() + "/patients";
    QDir rootDir(root);
    if (!rootDir.exists() && !rootDir.mkpath(".")) {
        qWarning().noquote() << "[Lib4DICOM] ensurePatientFolder: cannot create root:" << root;
        return {};
    }

    const QString namePart = sanitizeName(fullName.isEmpty() ? QStringLiteral("Unnamed") : fullName);
    const QString yearPart = sanitizeName(birthYear.isEmpty() ? QStringLiteral("----") : birthYear);

    QString base = namePart + "_" + yearPart;
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


Patient Lib4DICOM::patientFromMap(const QVariantMap& m) {
    Patient p;
    p.fullName = m.value("fullName").toString();
    p.birthYear = m.value("birthYear").toString();
    p.birthRaw = m.value("birthRaw").toString();
    p.sex = m.value("sex").toString();
    p.patientID = m.value("patientID").toString();
    p.sourceFilePath = m.value("sourceFilePath").toString();
    p.patientFolder = m.value("patientFolder").toString();
    return p;
}


QString Lib4DICOM::studyLabel() const {
    return m_studyLabel;
}

void Lib4DICOM::setStudyLabel(const QString& s) {
    QString v = s.trimmed().isEmpty() ? "Study" : s;
    if (v == m_studyLabel) return;
    m_studyLabel = v;
    emit studyLabelChanged();   // теперь это корректный вызов сигнала-члена
}

// Заменяет любые пробелы на "_", убирает опасные для файловой системы символы
QString Lib4DICOM::sanitizeName(const QString& in)
{
    QString s = in.trimmed();
    // Любые последовательности пробелов/табов/переводов строк -> "_"
    s.replace(QRegularExpression(R"(\s+)"), "_");
    // Запрещённые в Windows символы -> "_"
    s.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_");
    // Во избежание двойных подчёркиваний
    s.replace(QRegularExpression(R"(_{2,})"), "_");
    return s;
}


void Lib4DICOM::selectExistingPatient(int index)
{
    if (index < 0 || index >= m_patients.size()) {
        qWarning().noquote() << "[Lib4DICOM] selectExistingPatient: index out of range:" << index;
        return;
    }

    const Patient& p = m_patients.at(index);

    // Если уже выбран тот же — можно ничего не делать (опционально)
    if (m_selectedPatient.patientID == p.patientID &&
        m_selectedPatient.fullName == p.fullName &&
        m_selectedPatient.birthYear == p.birthYear &&
        m_selectedPatient.sex == p.sex) {
        return;
    }

    m_selectedPatient = p;
    emit selectedPatientChanged();

    qDebug().noquote() << "[Lib4DICOM] selected existing patient:"
        << m_selectedPatient.fullName
        << m_selectedPatient.birthYear
        << m_selectedPatient.sex
        << m_selectedPatient.patientID;
}

void Lib4DICOM::selectNewPatient(const QVariantMap& patient)
{
    Patient p = patientFromMap(patient);

    // Нормализуем чуть-чуть (чтобы «пустота» была однозначной)
    if (p.fullName.trimmed() == "--") p.fullName.clear();
    if (p.birthYear.trimmed() == "--") p.birthYear.clear();
    if (p.sex.trimmed() == "--") p.sex.clear();
    if (p.patientID.trimmed() == "--") p.patientID.clear();

    m_selectedPatient = std::move(p);
    emit selectedPatientChanged();

    qDebug().noquote() << "[Lib4DICOM] selected NEW patient:"
        << m_selectedPatient.fullName
        << m_selectedPatient.birthYear
        << m_selectedPatient.sex
        << m_selectedPatient.patientID;
}

void Lib4DICOM::clearSelectedPatient()
{
    // Сброс до «пустого» — значит «никто не выбран»
    m_selectedPatient = Patient{};
    emit selectedPatientChanged();
    qDebug().noquote() << "[Lib4DICOM] selected patient cleared";
}

QVariantMap Lib4DICOM::selectedPatient() const
{
    // Критерий «ничего не выбрано»: и имя, и ID пустые
    const bool none =
        m_selectedPatient.fullName.trimmed().isEmpty() &&
        m_selectedPatient.patientID.trimmed().isEmpty();

    QVariantMap map;
    if (none) {
        map["ok"] = false;
        return map;
    }

    map["ok"] = true;
    map["fullName"] = m_selectedPatient.fullName;
    map["birthYear"] = m_selectedPatient.birthYear;
    map["birthRaw"] = m_selectedPatient.birthRaw;
    map["sex"] = m_selectedPatient.sex;
    map["patientID"] = m_selectedPatient.patientID;
    map["sourceFilePath"] = m_selectedPatient.sourceFilePath;
    map["patientFolder"] = m_selectedPatient.patientFolder;
    return map;
}