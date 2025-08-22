#include "lib4dicom.h"

#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QByteArray>
#include <QBuffer>
#include <QImageReader>
#include <QDebug>
#include <QRegularExpression>

// DCMTK
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcuid.h>      // dcmGenerateUniqueIdentifier
#include <dcmtk/dcmdata/dcvrda.h>
#include <dcmtk/dcmdata/dcvrtm.h>

// UID для "Secondary Capture Image Storage"
static const char* SOP_CLASS_SC = UID_SecondaryCaptureImageStorage; // 1.2.840.10008.5.1.4.1.1.7

QImage Lib4DICOM::toRgb888(const QImage& src)
{
    if (src.format() == QImage::Format_RGB888)
        return src;
    return src.convertToFormat(QImage::Format_RGB888);
}
// ---------------- Конструктор ----------------
// Просто прокидываем parent в базовый класс QAbstractListModel.
Lib4DICOM::Lib4DICOM(QObject* parent) : QAbstractListModel(parent) {}


// ---------------- Реализация модели ----------------

// Количество строк в модели (равно количеству пациентов).
// Если parent валиден (это дочерние элементы), возвращаем 0,
// т.к. модель плоская (без иерархии).
int Lib4DICOM::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_patients.size();
}

// Данные для QML: возвращаем поле структуры Patient в зависимости от роли.
// Проверяем корректность индекса и границы. Если роль неизвестна — пустое значение.
QVariant Lib4DICOM::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_patients.size())
        return {};
    const auto& p = m_patients[index.row()];
    switch (role) {
    case FullNameRole:  return p.fullName;   // ФИО пациента (PatientName)
    case BirthYearRole: return p.birthYear;  // Год рождения (из PatientBirthDate)
    case SexRole:       return p.sex;        // Пол (PatientSex)
    }
    return {};
}

// Имена ролей для QML (чтобы в QML обращаться как к p.fullName, p.birthYear, p.sex).
QHash<int, QByteArray> Lib4DICOM::roleNames() const {
    return {
        { FullNameRole, "fullName" },
        { BirthYearRole,"birthYear"},
        { SexRole,      "sex"      }
    };
}

// Удобный геттер для QML: возвращаем саму модель (this),
// чтобы можно было привязать её напрямую к ListView/TableView.
QAbstractItemModel* Lib4DICOM::patientModel() {
    return this;
}

// ---------------- Новый код: нормализация ввода ----------------
QString Lib4DICOM::normalizeBirthYear(const QString& birthInput) {
    QString digits = birthInput;
    digits.remove(QRegularExpression(QStringLiteral(R"([^0-9])")));
    if (digits.size() >= 4) return digits.left(4);
    return QStringLiteral("--");
}

QString Lib4DICOM::normalizeSex(const QString& sexInput) {
    QString s = sexInput.trimmed().toUpper();

    // Поддержка "M/F/O", русских вариантов и слов
    if (s == "M" || s == "M" || s.startsWith("M")) return "M";
    if (s == "F" || s == "F" || s.startsWith("F")) return "F";
    if (s == "O" || s == "Other" || s == "Other") return "O";

    // На случай, если приходит значение из ComboBox.value ("M","F","O")
    if (s == "MALE") return "M";
    if (s == "FEMALE") return "F";
    if (s == "OTHER" || s == "UNSPECIFIED") return "O";

    return "O";
}

QString Lib4DICOM::normalizeID(const QString& idInput) {
    // Разрешим буквы/цифры/подчёркивание/дефис/точку, остальное уберём; ограничим длину.
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

// ---------------- Парсинг пациентов ----------------

// Публичный слот/метод для QML: сканируем папку "/patients" рядом с .exe,
// ищем DICOM-файлы (*.dcm / *.DCM), читаем атрибуты пациента и наполняем модель.
void Lib4DICOM::scanPatients() {
    // Объявляем полную перезагрузку данных модели (оповещение представлений).
    beginResetModel();
    m_patients.clear();

    // Корневая папка с пациентами: <папка_приложения>/patients.
    QDir root(QCoreApplication::applicationDirPath() + "/patients");
    if (!root.exists())
        root.mkpath("."); // Создаём, если её нет.

    // Фильтр для DICOM-файлов в корне (можно сузить/расширить при необходимости).
    const QStringList nameFilters = { "*.dcm", "*.DCM" };
    const QFileInfoList files = root.entryInfoList(
        nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    // Список подпапок (один уровень вложенности).
    const QFileInfoList dirs = root.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    // Лямбда для обработки одного файла: открыть как DICOM и извлечь нужные теги.
    auto processFile = [&](const QString& path) {
        // ВАЖНО для Windows: использовать «нативную» кодировку пути.
        const QByteArray native = QFile::encodeName(path);

        DcmFileFormat ff; // Обёртка DCMTK для файла + dataset.
        if (ff.loadFile(native.constData()).good()) { // Успешно открыли DICOM?
            auto* ds = ff.getDataset(); // Сам dataset (набор тегов).
            Patient p;

            OFString v; // Строковый буфер DCMTK.

            // PatientName (PN). В DICOM разделители компонент — '^', меняем на пробел.
            if (ds->findAndGetOFString(DCM_PatientName, v).good())
                p.fullName = QString::fromLatin1(v.c_str()).replace("^", " ");
            else
                p.fullName = "--"; // Нет значения — ставим плейсхолдер.

            // PatientBirthDate (DA) формата YYYYMMDD. Берём только YYYY, если хватает длины.
            if (ds->findAndGetOFString(DCM_PatientBirthDate, v).good() && v.length() >= 4)
                p.birthYear = QString::fromLatin1(v.c_str()).left(4);
            else
                p.birthYear = "--";

            // PatientSex (CS): обычно "M", "F" или "O".
            if (ds->findAndGetOFString(DCM_PatientSex, v).good())
                p.sex = QString::fromLatin1(v.c_str());
            else
                p.sex = "--";

            // Добавляем пациента в буфер.
            m_patients.push_back(p);
        }
        // Иначе: файл не открылся как DICOM — пропускаем без ошибки.
        };

    // Проходим файлы в корне.
    for (const QFileInfo& f : files)
        processFile(f.absoluteFilePath());

    // Проходим файлы в подпапках (только один уровень).
    for (const QFileInfo& d : dirs) {
        QDir sub(d.absoluteFilePath());
        const QFileInfoList subFiles = sub.entryInfoList(
            nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& f : subFiles)
            processFile(f.absoluteFilePath());
    }

    // Завершаем перезагрузку модели и уведомляем QML о смене ссылки (если бинджен).
    endResetModel();
    emit patientModelChanged();
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

QString Lib4DICOM::makeSafeFolderName(const QString& s)
{
    QString t = s.trimmed();

    // заменить недопустимые символы на подчёркивание
    t.replace(QRegularExpression(R"([\\\/\:\*\?\"\<\>\|\n\r\t])"), "_");

    // нормализовать пробелы -> один пробел
    t.replace(QRegularExpression(R"(\s+)"), " ");

    // пробелы -> подчёркивания
    t.replace(' ', '_');

    // обрезать до разумной длины
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

    // если уже существует — добавляем суффикс _2, _3, ...
    int n = 2;
    while (QDir(candidate).exists()) {
        candidate = root + "/" + base + "_" + QString::number(n++);
        if (n > 9999) { // на всякий случай
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
    // DCMTK сам сгенерирует уникальный UID на основе системных источников энтропии
    char uid[128] = { 0 };
    dcmGenerateUniqueIdentifier(uid); // root по умолчанию (SITE_INSTANCE_UID_ROOT)
    return QString::fromLatin1(uid);
}

QVariantMap Lib4DICOM::createStudyForNewPatient(const QString& fullName,
    const QString& birthYearNormalized,
    const QString& patientID)
{
    QVariantMap out;

    // 1) Папка пациента
    const QString patientFolder = ensurePatientFolder(fullName, birthYearNormalized);
    if (patientFolder.isEmpty()) {
        qWarning().noquote() << "[Lib4DICOM] createStudyForNewPatient: patient folder not created";
        return out; // пусто
    }
    out["patientFolder"] = patientFolder;

    // 2) Название исследования: <PatientID>_<YYYYMMDD>
    const QString safeID = normalizeID(patientID);
    const QString dateStr = QDate::currentDate().toString("yyyyMMdd");
    const QString base = safeID + "_" + dateStr;

    // 3) Уникальное имя папки исследования
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

    // 4) Генерация StudyInstanceUID
    const QString studyUID = generateDicomUID();

    qDebug().noquote() << "[Lib4DICOM] Study created:"
        << "\n  patientFolder =" << patientFolder
        << "\n  studyFolder   =" << studyFolder
        << "\n  studyName     =" << QFileInfo(studyFolder).fileName()
        << "\n  studyUID      =" << studyUID;

    out["studyFolder"] = studyFolder;
    out["studyName"] = QFileInfo(studyFolder).fileName();
    out["studyUID"] = studyUID;
    return out;
}

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
            << localPath << "error:" << reader.errorString();
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

    QFileInfo fi(localPath);
    if (!fi.exists() || !fi.isFile()) {
        qWarning().noquote() << "[Lib4DICOM] loadImageVectorFromFile: file does not exist:" << localPath;
        return result;
    }

    QImageReader reader(localPath);
    reader.setAutoTransform(true);
    QImage img = reader.read();
    if (img.isNull()) {
        qWarning().noquote() << "[Lib4DICOM] loadImageVectorFromFile: failed to read:"
            << localPath << "error:" << reader.errorString();
        return result;
    }

    result.push_back(img);

    qDebug().noquote() << "[Lib4DICOM] loadImageVectorFromFile: loaded 1 image:"
        << fi.fileName() << img.width() << "x" << img.height();
    return result;
}

QVariantMap Lib4DICOM::saveImagesAsDicom(const QVector<QImage>& images,
    const QString& outFolder,
    const QString& patientID,
    const QString& seriesName,
    const QString& studyUIDIn)
{
    QVariantMap result;
    QStringList outFiles;
    int saved = 0;

    if (images.isEmpty()) {
        result["ok"] = false;
        result["error"] = "images is empty";
        return result;
    }

    QDir dir(outFolder);
    if (outFolder.isEmpty() || !dir.exists()) {
        result["ok"] = false;
        result["error"] = "output folder does not exist";
        return result;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const QString studyDate = now.date().toString("yyyyMMdd");
    const QString studyTime = now.time().toString("HHmmss");

    // UID'ы исследования/серии
    const QString studyUID = studyUIDIn.isEmpty() ? generateDicomUID() : studyUIDIn;
    const QString seriesUID = generateDicomUID();

    const QString idToken = patientID.isEmpty() ? QStringLiteral("--") : normalizeID(patientID);
    const QString seriesToken = seriesName.isEmpty() ? QStringLiteral("SER") : normalizeID(seriesName);

    for (int i = 0; i < images.size(); ++i) {
        // подготовка пикселей
        QImage rgb = toRgb888(images[i]);
        if (rgb.isNull()) {
            qWarning().noquote() << "[Lib4DICOM] saveImagesAsDicom: image" << i << "is null";
            continue;
        }

        const int rows = rgb.height();
        const int cols = rgb.width();
        const int samplesPerPixel = 3;    // RGB
        const int bitsAllocated = 8;
        const int bitsStored = 8;
        const int highBit = 7;
        const int pixelRepr = 0;    // unsigned
        const int planarConfig = 0;    // interleaved RGB

        const int frameBytes = rows * cols * samplesPerPixel;
        QByteArray pixelData(reinterpret_cast<const char*>(rgb.constBits()), frameBytes);

        DcmFileFormat file;
        DcmDataset* ds = file.getDataset();

        // Идентификаторы
        const QString sopInstanceUID = generateDicomUID();
        ds->putAndInsertString(DCM_SOPClassUID, SOP_CLASS_SC);
        ds->putAndInsertString(DCM_SOPInstanceUID, sopInstanceUID.toLatin1().constData());
        ds->putAndInsertString(DCM_StudyInstanceUID, studyUID.toLatin1().constData());
        ds->putAndInsertString(DCM_SeriesInstanceUID, seriesUID.toLatin1().constData());

        // Пациент
        ds->putAndInsertString(DCM_PatientID, idToken.toLatin1().constData());

        // Исследование/серия/инстанс — базовые теги
        ds->putAndInsertString(DCM_StudyDate, studyDate.toLatin1().constData());
        ds->putAndInsertString(DCM_StudyTime, studyTime.toLatin1().constData());
        ds->putAndInsertString(DCM_SeriesDate, studyDate.toLatin1().constData());
        ds->putAndInsertString(DCM_SeriesTime, studyTime.toLatin1().constData());
        ds->putAndInsertUint16(DCM_SeriesNumber, 1);
        ds->putAndInsertUint16(DCM_InstanceNumber, static_cast<Uint16>(i + 1));

        // Image Pixel Module
        ds->putAndInsertUint16(DCM_Rows, rows);
        ds->putAndInsertUint16(DCM_Columns, cols);
        ds->putAndInsertString(DCM_PhotometricInterpretation, "RGB");
        ds->putAndInsertUint16(DCM_SamplesPerPixel, samplesPerPixel);
        ds->putAndInsertUint16(DCM_BitsAllocated, bitsAllocated);
        ds->putAndInsertUint16(DCM_BitsStored, bitsStored);
        ds->putAndInsertUint16(DCM_HighBit, highBit);
        ds->putAndInsertUint16(DCM_PixelRepresentation, pixelRepr);
        ds->putAndInsertUint16(DCM_PlanarConfiguration, planarConfig);

        // Пиксели
        ds->putAndInsertUint8Array(DCM_PixelData,
            reinterpret_cast<const Uint8*>(pixelData.constData()),
            static_cast<unsigned long>(pixelData.size()));

        // Необязательные, но полезные поля
        ds->putAndInsertString(DCM_ConversionType, "WSD"); // Workstation

        // Имя файла: <PatientID>_<SeriesName>_<YYYYMMDD>_<HHMMSS>_<NNN>.dcm
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
            qDebug().noquote() << "[Lib4DICOM] saved:" << absPath;
        }
        else {
            qWarning().noquote() << "[Lib4DICOM] save failed for" << absPath << ":" << st.text();
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

