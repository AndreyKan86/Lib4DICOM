flowchart TD
    %% Внешние точки входа
    A[QML / API] --> B(scanPatients)
    A --> C(readDemographicsFromFile)
    A --> D(findPatientStubByIndex)
    A --> E(makePatientFromStrings)
    A --> F(createStudyForNewPatient)
    A --> G(createStudyInPatientFolder)
    A --> H(createPatientStubDicom)
    A --> I(convertAndSaveImageAsDicom)
    A --> J(saveImagesAsDicom)
    A --> K(loadImageFromFile)
    A --> L(loadImageVectorFromFile)
    A --> M(getPatientDemographics)
    A --> N(logSelectedFileAndPatient)

    %% Вспомогательные вызовы
    B --> O(decodeDicomText)
    C --> O
    D --> O

    F --> P(ensurePatientFolder)
    F --> Q(generateDicomUID)

    G --> R(safeNameForPath)
    G --> Q

    H --> Q

    I --> K
    I --> J

    J --> Q

    P --> R

    L --> K
