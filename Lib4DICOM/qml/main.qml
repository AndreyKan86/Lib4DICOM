import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import Qt.labs.qmlmodels 1.0


ApplicationWindow {
    id: win
    width: 720
    height: 480
    visible: true
    title: "DICOM — пациенты"

    // Пользовательский сигнал: «идём дальше» с полезной нагрузкой
    signal proceed(string payload)

    Connections {
        target: win
        onProceed: {
            console.log("[QML] proceed:", payload)
        }
    }

    // Состояние выбора
    property bool  selectedIsNew: false      // выбрана ли строка «Новый пациент»
    property int   selectedIndex: -1         // индекс выбранного пациента в модели (если не «новый»)

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: startPage
    }

    Component {
        id: startPage
        Page {
            Component.onCompleted: {
                if (appLogic && appLogic.scanPatients) {
                    console.log("[QML] auto-scan on page start")
                    appLogic.scanPatients()
                }
            }
            header: ToolBar {
                RowLayout {
                    anchors.fill: parent
                    spacing: 12

                    Label {
                        Layout.alignment: Qt.AlignVCenter
                        text: {
                            const base = (patientsList.count === 0
                                          ? "Пациенты (нет пациентов)"
                                          : "Пациенты (" + patientsList.count + " найдено)")
                            const filtered = (tableFrame.filterName || tableFrame.filterBirth || tableFrame.filterSex !== "ALL")
                            return base + (filtered ? " — фильтр" : "")
                        }
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true } // растяжка
                }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                // === Таблица ===
                Rectangle {
                    id: tableFrame
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 6
                    border.width: 1
                    border.color: "#bfc3c8"
                    color: "white"
                    clip: true

                    // размеры колонок/сетки
                    property int yearW: 140
                    property int sexW: 90
                    property int sepW: 1
                    property color gridColor: "#c7cbd1"
                    property int nameW: width - yearW - sexW - 2*sepW

                    // --- фильтры ---
                    property string filterName: ""
                    property string filterBirth: ""
                    property string filterSex: "ALL"   // ALL/M/F/O

                    Column {
                        width: parent.width
                        height: parent.height
                        spacing: 0

                        // --- Шапка ---
                        Rectangle {
                            id: headerRow
                            height: 36
                            width: parent.width
                            color: "#efefef"

                            Row {
                                anchors.fill: parent
                                spacing: 0

                                Rectangle {
                                    width: tableFrame.nameW
                                    height: parent.height
                                    color: "transparent"
                                    Label {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        text: "ФИО"
                                        verticalAlignment: Qt.AlignVCenter
                                        color: "black"
                                    }
                                }
                                Rectangle { width: tableFrame.sepW; height: parent.height; color: tableFrame.gridColor }
                                Rectangle {
                                    width: tableFrame.yearW
                                    height: parent.height
                                    color: "transparent"
                                    Label {
                                        anchors.fill: parent
                                        text: "Год рождения"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        color: "black"
                                    }
                                }
                                Rectangle { width: tableFrame.sepW; height: parent.height; color: tableFrame.gridColor }
                                Rectangle {
                                    width: tableFrame.sexW
                                    height: parent.height
                                    color: "transparent"
                                    Label {
                                        anchors.fill: parent
                                        text: "Пол"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        color: "black"
                                    }
                                }
                            }

                            // одна нижняя линия под шапкой
                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: tableFrame.gridColor
                            }
                        }

                        // --- Данные (скроллируемая часть) ---
                        ScrollView {
                            id: sv
                            width: parent.width
                            height: parent.height - headerRow.height - footerRow.height
                            clip: true
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded




                            ListView {
                                id: patientsList
                                width: parent.width
                                model: appLogic ? appLogic.patientModel : null
                                boundsBehavior: Flickable.StopAtBounds
                                clip: true
                                interactive: true


                                header: Rectangle {
                                    width: patientsList.width
                                    height: 36
                                    color: selectedIsNew ? "#e6f2ff" : "white"

                                    Text {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        text: "Новый пациент"
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                        color: "black"
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        height: 1
                                        color: tableFrame.gridColor
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        acceptedButtons: Qt.LeftButton
                                        onClicked: {
                                            selectedIsNew = true
                                            selectedIndex = -1
                                            patientsList.currentIndex = -1
                                        }
                                        onDoubleClicked: {
                                            selectedIsNew = true
                                            selectedIndex = -1
                                            patientsList.currentIndex = -1
                                            stack.push(nextPage, { existingMode: false, existingIndex: -1 })
                                        }
                                        Keys.onEnterPressed: onDoubleClicked()
                                        Keys.onReturnPressed: onDoubleClicked()
                                    }
                                }



                                // Делегат обычных пациентов
                                delegate: Rectangle {
                                    width: patientsList.width
                                    // подсветка текущего элемента (если выбрана не строка «Новый пациент»)
                                    color: (!selectedIsNew && ListView.isCurrentItem) ? "#e6f2ff" : "white"

                                    // фильтрация
                                    function matchesFilter() {
                                        const name  = (fullName || "").toLowerCase()
                                        const birth = (birthYear || "").toString()
                                        const sx    = (sex || "")

                                        const fName  = tableFrame.filterName.toLowerCase()
                                        const fBirth = tableFrame.filterBirth
                                        const fSex   = tableFrame.filterSex

                                        const nameOk  = !fName  || name.indexOf(fName) !== -1
                                        const birthOk = !fBirth || birth.indexOf(fBirth) !== -1   // можно заменить на startsWith
                                        const sexOk   = (fSex === "ALL") || (sx === fSex)

                                        return nameOk && birthOk && sexOk
                                    }

                                    visible: matchesFilter()
                                    height: visible ? 36 : 0

                                    Row {
                                        anchors.fill: parent
                                        spacing: 0

                                        // ФИО
                                        Rectangle {
                                            width: tableFrame.nameW
                                            height: parent.height
                                            color: "transparent"
                                            Text {
                                                anchors.fill: parent
                                                anchors.margins: 8
                                                text: fullName
                                                elide: Text.ElideRight
                                                verticalAlignment: Text.AlignVCenter
                                                color: "black"
                                            }
                                        }
                                        Rectangle { width: tableFrame.sepW; height: parent.height; color: tableFrame.gridColor }

                                        // Год рождения
                                        Rectangle {
                                            width: tableFrame.yearW
                                            height: parent.height
                                            color: "transparent"
                                            Text {
                                                anchors.fill: parent
                                                text: birthYear
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                                color: "black"
                                            }
                                        }
                                        Rectangle { width: tableFrame.sepW; height: parent.height; color: tableFrame.gridColor }

                                        // Пол
                                        Rectangle {
                                            width: tableFrame.sexW
                                            height: parent.height
                                            color: "transparent"
                                            Text {
                                                anchors.fill: parent
                                                text: sex
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                                color: "black"
                                            }
                                        }
                                    }

                                    // Горизонтальная линия
                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        height: 1
                                        color: tableFrame.gridColor
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        acceptedButtons: Qt.LeftButton
                                        onClicked: {
                                            // обычный одинарный клик — просто выделяем строку
                                            selectedIsNew = false
                                            patientsList.currentIndex = index
                                            selectedIndex = index
                                        }

                                        onDoubleClicked: {
                                            selectedIsNew = false
                                            patientsList.currentIndex = index
                                            selectedIndex = index
                                            stack.push(nextPage, { existingMode: true, existingIndex: index })
                                        }
                                    }
                                }
                            }
                        }

// --- Футер с фильтрами (фиксированная строка под списком) ---
Rectangle {
    id: footerRow
    height: 36
    width: parent.width
    color: tableFrame.uiPanelBg

    Row {
        anchors.fill: parent
        spacing: 0

        // ФИО — фильтр по подстроке
        Rectangle {
            width: tableFrame.nameW
            height: parent.height
            color: "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6

                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Поиск по ФИО…"
                    text: tableFrame.filterName
                    onTextChanged: tableFrame.filterName = text
                    color: tableFrame.uiText
                    placeholderTextColor: tableFrame.uiTextHint

                    background: Rectangle {
                        radius: 6
                        color: tableFrame.uiFieldBg         // фон поля совпадает с popup
                        border.color: parent.focus ? tableFrame.uiFieldBorderFocus
                                                   : tableFrame.uiFieldBorder
                        border.width: 1
                    }
                }

                ToolButton {
                    text: "×"
                    visible: tableFrame.filterName.length > 0
                    onClicked: tableFrame.filterName = ""
                    // делаем кнопку аккуратной
                    background: Rectangle { radius: 6; color: "transparent" }
                }
            }
        }

        // Вертикальный разделитель
        Rectangle { width: tableFrame.sepW; height: parent.height; color: tableFrame.gridColor }

        // Год рождения — фильтр
        Rectangle {
            width: tableFrame.yearW
            height: parent.height
            color: "transparent"
            TextField {
                anchors.fill: parent
                anchors.margins: 6
                placeholderText: "Год…"
                inputMethodHints: Qt.ImhPreferNumbers
                text: tableFrame.filterBirth
                onTextChanged: tableFrame.filterBirth = text
                horizontalAlignment: Text.AlignHCenter
                color: tableFrame.uiText
                placeholderTextColor: tableFrame.uiTextHint

                background: Rectangle {
                    radius: 6
                    color: tableFrame.uiFieldBg
                    border.color: parent.focus ? tableFrame.uiFieldBorderFocus
                                               : tableFrame.uiFieldBorder
                    border.width: 1
                }
            }
        }

        // Вертикальный разделитель
        Rectangle { width: tableFrame.sepW; height: parent.height; color: tableFrame.gridColor }

        // Пол — выпадающий список
        Rectangle {
            width: tableFrame.sexW
            height: parent.height
            color: "transparent"

            ComboBox {
                id: sexFilter
                anchors.fill: parent
                anchors.margins: 4
                textRole: "text"
                valueRole: "value"
                model: [
                    { text: "Все", value: "ALL" },
                    { text: "М",   value: "M"   },
                    { text: "Ж",   value: "F"   },
                    { text: "Др.", value: "O"   }
                ]
                onCurrentIndexChanged: tableFrame.filterSex = currentValue

                // Текст в закрытом состоянии
                contentItem: Text {
                    text: sexFilter.displayText
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 8; rightPadding: 24
                    color: tableFrame.uiText
                    elide: Text.ElideRight
                }

                // Стрелка
                indicator: Text {
                    text: "\u25BE"  // ▾
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    color: tableFrame.uiTextHint
                }

                // Фон комбобокса (совпадает с полями)
                background: Rectangle {
                    radius: 6
                    color: tableFrame.uiFieldBg
                    border.color: sexFilter.activeFocus ? tableFrame.uiFieldBorderFocus : tableFrame.uiFieldBorder
                    border.width: 1
                }

                // === ВАЖНО: свой popup с корректной моделью ===
                popup: Popup {
                    y: sexFilter.height
                    width: sexFilter.width
                    padding: 0
                    implicitHeight: Math.min(list.implicitHeight, 240)

                    background: Rectangle {
                        radius: 6
                        color: tableFrame.uiFieldBg   // тот же фон, что у полей
                        border.color: tableFrame.uiFieldBorder
                        border.width: 1
                    }

                    contentItem: ListView {
                        id: list
                        clip: true
                        implicitHeight: contentHeight
                        model: sexFilter.model                // <-- не delegateModel!
                        currentIndex: sexFilter.highlightedIndex

                        delegate: ItemDelegate {
                            width: list.width
                            text: (typeof modelData === "object")
                                    ? modelData[sexFilter.textRole]
                                    : String(modelData)
                            background: Rectangle {
                                color: pressed ? tableFrame.uiItemPressed
                                               : (hovered ? tableFrame.uiItemHover
                                                          : tableFrame.uiFieldBg)
                            }
                            contentItem: Text {
                                text: parent.text
                                color: tableFrame.uiText
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: 8; rightPadding: 8
                            }
                            onClicked: {
                                sexFilter.currentIndex = index
                                sexFilter.popup.close()
                            }
                        }
                    }
                }
            }


        }
    }

    // верхняя разделительная линия футера
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: tableFrame.gridColor
    }
}





                    }
                }

                // нижняя панель с серой линией сверху и кнопкой
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#d9dce1" // светло-серая линия
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignRight
                        Layout.topMargin: 8

                        Button {
                            text: "Далее"
                            onClicked: {
                                if (selectedIsNew) {
                                    // Открываем страницу создания/ввода нового пациента
                                    stack.push(nextPage, { existingMode: false, existingIndex: -1 })
                                } else if (selectedIndex >= 0) {
                                    // Переходим к существующему пациенту
                                    stack.push(nextPage, { existingMode: true, existingIndex: selectedIndex })
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: nextPage
        Page {
            id: pageNew

            header: ToolBar {
                RowLayout {
                    anchors.fill: parent
                    ToolButton { text: "←"; onClicked: stack.pop() }
                    Label {
                        text: pageNew.existingMode ? "Добавить снимок к существующему пациенту" : "Создать нового пациента"
                        Layout.alignment: Qt.AlignVCenter
                    }
                }
            }

            // локальные данные пациента (ВСЕ объявлены на Page!)
            property bool existingMode: false
            property int  existingIndex: -1

            property string pName: ""
            property string pBirth: ""
            property string pSex: ""
            property string pPatientID: ""
            property string pPatientFolder: ""
            property string pFile: ""  // выбранное изображение

Component.onCompleted: {
    if (existingMode && existingIndex >= 0 && appLogic) {
        // Пытаемся через stub
        const stubInfo = appLogic.findPatientStubByIndex(existingIndex)
        if (stubInfo && stubInfo.ok) {
            pPatientFolder = stubInfo.patientFolder
            const demo = appLogic.readDemographicsFromFile(stubInfo.stubPath)
            if (demo && demo.ok) {
                pName      = demo.patientName  || "--"
                pBirth     = demo.patientBirth || "--"
                pSex       = demo.patientSex   || "O"
                pPatientID = demo.patientID    || "--"
            }
        } else {
            // ФОЛБЭК БЕЗ СТАБА
            const g = appLogic.getPatientDemographics(existingIndex)
            if (g && g.ok) {
                pName         = g.fullName    || pName
                pBirth        = g.birthYear   || pBirth
                pSex          = g.sex         || pSex
                pPatientID    = g.patientID   || pPatientID
                pPatientFolder= g.patientFolder || pPatientFolder
            }
        }
    }
}


            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                GroupBox {
                    title: "Данные пациента"
                    Layout.fillWidth: true

                    GridLayout {
                        columns: 2
                        rowSpacing: 8
                        columnSpacing: 12
                        Layout.fillWidth: true

                        Label { text: "Имя (ФИО):" }
                        TextField {
                            id: nameField
                            Layout.fillWidth: true
                            placeholderText: "Иванов Иван Иванович"
                            text: pageNew.pName
                            onTextChanged: if (!pageNew.existingMode) pageNew.pName = text
                            enabled: !pageNew.existingMode
                            readOnly: pageNew.existingMode
                        }

                        Label { text: "Дата рождения:" }
                        TextField {
                            id: birthField
                            Layout.fillWidth: true
                            placeholderText: "YYYY или YYYYMMDD"
                            inputMethodHints: Qt.ImhPreferNumbers
                            text: pageNew.pBirth
                            onTextChanged: if (!pageNew.existingMode) pageNew.pBirth = text
                            enabled: !pageNew.existingMode
                            readOnly: pageNew.existingMode
                        }

                        Label { text: "Пол:" }
                        ComboBox {
                            id: sexCombo
                            Layout.fillWidth: true
                            textRole: "text"
                            valueRole: "value"
                            model: [
                                { text: "Мужской", value: "M" },
                                { text: "Женский", value: "F" },
                                { text: "Другое/не указано", value: "O" }
                            ]
                            onCurrentIndexChanged: if (!pageNew.existingMode) pageNew.pSex = currentValue
                            Component.onCompleted: {
                                if (pageNew.existingMode) {
                                    const v = pageNew.pSex || "O"
                                    const idx = model.findIndex(m => m.value === v)
                                    if (idx >= 0) currentIndex = idx
                                } else {
                                    pageNew.pSex = currentValue
                                }
                            }
                            enabled: !pageNew.existingMode
                        }

                        // --- Новые поля ---
                        Label { text: "ID пациента:" }
                        TextField {
                            id: patientIdField
                            Layout.fillWidth: true
                            placeholderText: "Напр.: P12345"
                            text: pageNew.pPatientID
                            onTextChanged: if (!pageNew.existingMode) pageNew.pPatientID = text
                            enabled: !pageNew.existingMode
                            readOnly: pageNew.existingMode
                        }
                    }
                }

                // ===== Выбор файла =====
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: filePathField
                        Layout.fillWidth: true
                        readOnly: true
                        placeholderText: "Файл не выбран"
                        text: pageNew.pFile
                    }

                    Button {
                        text: "Обзор…"
                        onClicked: fileDialog.open()
                    }
                }

                FileDialog {
                    id: fileDialog
                    title: "Выберите файл изображения"
                    fileMode: FileDialog.OpenFile
                    nameFilters: [ "Изображения (*.bmp *.jpeg *.jpg *.png)", "Все файлы (*)" ]

                    onAccepted: {
                        // 1) Берём URL из любого доступного свойства
                        var url = null

                        if (fileDialog.selectedFile)                 // Qt 6.x
                            url = fileDialog.selectedFile
                        else if (fileDialog.fileUrl)                 // Qt 5.15.x
                            url = fileDialog.fileUrl
                        else if (fileDialog.currentFile)             // некоторые сборки/плагины
                            url = fileDialog.currentFile
                        else if (fileDialog.selectedFiles && fileDialog.selectedFiles.length > 0)
                            url = fileDialog.selectedFiles[0]
                        else if (fileDialog.fileUrls && fileDialog.fileUrls.length > 0)
                            url = fileDialog.fileUrls[0]

                        // 2) Превращаем в локальный путь
                        var localPath = ""
                        if (url && url.toLocalFile) {
                            localPath = url.toLocalFile()
                        } else if (url && url.toString) {
                            // fallback: отрезаем file:/// вручную
                            var s = url.toString()
                            if (s.startsWith("file:///"))      localPath = s.substr(8)
                            else if (s.startsWith("file://"))  localPath = s.substr(7)
                            else                                localPath = s
                        }

                        if (!localPath || localPath.length === 0) {
                            console.warn("[QML] cannot extract localPath from FileDialog:", url)
                            return
                        }

                        pageNew.pFile = localPath
                        filePathField.text = localPath
                        console.log("[QML] chosen image:", localPath)

                        // ===== дальше твоя логика для существующего пациента =====
                        if (!pageNew.existingMode)
                            return

                        if (!appLogic || !appLogic.createStudyInPatientFolder) {
                            console.warn("[QML] createStudyInPatientFolder not found")
                            return
                        }

                        // если папка/ID ещё не заполнены — возьмём из модели
                        if ((!pageNew.pPatientFolder || !pageNew.pPatientID) &&
                            appLogic && appLogic.getPatientDemographics && pageNew.existingIndex >= 0) {
                            const g = appLogic.getPatientDemographics(pageNew.existingIndex)
                            if (g && g.ok) {
                                pageNew.pPatientFolder = pageNew.pPatientFolder || g.patientFolder
                                pageNew.pPatientID     = pageNew.pPatientID     || g.patientID
                                pageNew.pName          = pageNew.pName          || g.fullName
                                pageNew.pBirth         = pageNew.pBirth         || g.birthYear
                                pageNew.pSex           = pageNew.pSex           || g.sex
                            }
                        }

                        if (!pageNew.pPatientFolder || !pageNew.pPatientID) {
                            console.warn("[QML] patientFolder or patientID is empty")
                            return
                        }

                        const study = appLogic.createStudyInPatientFolder(pageNew.pPatientFolder, pageNew.pPatientID)
                        if (!study || !study.ok) {
                            console.warn("[QML] failed to create study:", study ? study.error : "undefined")
                            return
                        }

                        if (!appLogic || !appLogic.convertAndSaveImageAsDicom) {
                            console.warn("[QML] convertAndSaveImageAsDicom not found")
                            return
                        }

                        const res = appLogic.convertAndSaveImageAsDicom(
                            pageNew.pFile,
                            study.studyFolder,
                            pageNew.pPatientID,
                            "SER01",
                            study.studyUID,
                            pageNew.pName,
                            pageNew.pBirth,
                            pageNew.pSex
                        )
                        console.log("[QML] Save existing:", JSON.stringify(res))
                    }

                    onRejected: console.log("[QML] FileDialog canceled")
                }




                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    Layout.topMargin: 8

                    Button {
                        text: "Далее"
                        onClicked: {
                            // Сценарий НОВОГО пациента (существующий уже сохраняется при выборе файла)
                            // 1) Собираем данные пациента
                            let patient = null
                            if (appLogic && appLogic.makePatientFromStrings) {
                                patient = appLogic.makePatientFromStrings(
                                    pageNew.pName, pageNew.pBirth, pageNew.pSex, pageNew.pPatientID
                                )
                            } else {
                                console.warn("appLogic.makePatientFromStrings не найден")
                            }

                            // 2) Создаём исследование для нового пациента
                            let study = null
                            if (!pageNew.existingMode && patient && appLogic && appLogic.createStudyForNewPatient) {
                                study = appLogic.createStudyForNewPatient(
                                    patient.fullName, patient.birthYear, patient.patientID
                                )
                                if (!study || !study.studyFolder) {
                                    console.warn("[QML] Failed to create study folder")
                                    return
                                }

                                // 2.1) Stub DICOM в корне папки пациента (демография)
                                if (appLogic && appLogic.createPatientStubDicom && study.patientFolder) {
                                    const stub = appLogic.createPatientStubDicom(
                                        study.patientFolder,
                                        patient.patientID,
                                        patient.fullName,
                                        patient.birthYear,
                                        patient.sex
                                    )
                                    if (!stub.ok) console.warn("[QML] Stub DICOM failed:", stub.error)
                                    else          console.log("[QML] Stub DICOM created:", stub.path)
                                } else {
                                    console.warn("[QML] createPatientStubDicom не найден или нет patientFolder")
                                }
                            }
        
                            let res = null

                            console.log("[QML] pageNew.existingMode:", pageNew.existingMode)
                            console.log("[QML] pageNew.pFile:", pageNew.pFile)
                            console.log("[QML] pageNew.pFile.length > 0:", (pageNew.pFile.length > 0))
                            console.log("[QML] study:", study)
                            console.log("[QML] appLogic", appLogic.convertAndSaveImageAsDicom)

                            // 3) Файл -> QVector<QImage> -> DICOM
                            if (!pageNew.existingMode &&
                                pageNew.pFile && pageNew.pFile.length > 0 && study &&
                                appLogic && appLogic.convertAndSaveImageAsDicom) {

                                const res = appLogic.convertAndSaveImageAsDicom(
                                  pageNew.pFile,
                                  study.studyFolder,
                                  patient.patientID,
                                  "SER01",
                                  study.studyUID,
                                  patient.fullName,
                                  patient.birthYear,
                                  patient.sex
                                )
                                console.log("[QML] DICOM save (new patient):", JSON.stringify(res))
                            } else {
                                    console.log("[QML] DICOM not save (new patient):", JSON.stringify(res))
                                    }

                          

                            // 4) Очистка и закрытие
                            pageNew.pName = ""
                            pageNew.pBirth = ""
                            pageNew.pSex = sexCombo.currentValue
                            pageNew.pFile = ""
                            pageNew.pPatientID = ""
                            win.close()
                        }
                    }
                }
            }
        }
    }
}