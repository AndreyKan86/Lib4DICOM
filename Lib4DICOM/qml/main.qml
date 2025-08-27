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

    signal proceed(string payload)

    Connections {
        target: win
        onProceed: console.log("[QML] proceed:", payload)
    }

    // состояние выбора
    property bool  selectedIsNew: false
    property int   selectedIndex: -1

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

                    Item { Layout.fillWidth: true }
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

                    // --- недостающая палитра для футера/полей (чтоб не было ReferenceError) ---
                    property color uiPanelBg:       "#f7f8fa"
                    property color uiText:          "#1f2328"
                    property color uiTextHint:      "#6b7280"
                    property color uiFieldBg:       "#ffffff"
                    property color uiFieldBorder:   "#d9dce1"
                    property color uiFieldBorderFocus: "#2563eb"
                    property color uiItemHover:     "#edf2ff"
                    property color uiItemPressed:   "#e6f2ff"

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

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: tableFrame.gridColor
                            }
                        }

                        // --- Данные ---
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

                                // строка "Новый пациент" — на всю ширину; подсветка только при выборе
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
                                    color: (!selectedIsNew && ListView.isCurrentItem) ? "#e6f2ff" : "white"

                                    function matchesFilter() {
                                        const name  = (fullName || "").toLowerCase()
                                        const birth = (birthYear || "").toString()
                                        const sx    = (sex || "")

                                        const fName  = tableFrame.filterName.toLowerCase()
                                        const fBirth = tableFrame.filterBirth
                                        const fSex   = tableFrame.filterSex

                                        const nameOk  = !fName  || name.indexOf(fName) !== -1
                                        const birthOk = !fBirth || birth.indexOf(fBirth) !== -1
                                        const sexOk   = (fSex === "ALL") || (sx === fSex)

                                        return nameOk && birthOk && sexOk
                                    }

                                    visible: matchesFilter()
                                    height: visible ? 36 : 0

                                    Row {
                                        anchors.fill: parent
                                        spacing: 0

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

                        // --- Футер с фильтрами ---
                        Rectangle {
                            id: footerRow
                            height: 36
                            width: parent.width
                            color: tableFrame.uiPanelBg

                            Row {
                                anchors.fill: parent
                                spacing: 0

                                // ФИО
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
                                                color: tableFrame.uiFieldBg
                                                border.color: parent.focus ? tableFrame.uiFieldBorderFocus
                                                                           : tableFrame.uiFieldBorder
                                                border.width: 1
                                            }
                                        }

                                        ToolButton {
                                            text: "×"
                                            visible: tableFrame.filterName.length > 0
                                            onClicked: tableFrame.filterName = ""
                                            background: Rectangle { radius: 6; color: "transparent" }
                                        }
                                    }
                                }

                                Rectangle { width: tableFrame.sepW; height: parent.height; color: tableFrame.gridColor }

                                // Год
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

                                Rectangle { width: tableFrame.sepW; height: parent.height; color: tableFrame.gridColor }

                                // Пол
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

                                        contentItem: Text {
                                            text: sexFilter.displayText
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: 8; rightPadding: 24
                                            color: tableFrame.uiText
                                            elide: Text.ElideRight
                                        }

                                        indicator: Text {
                                            text: "\u25BE"
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.right: parent.right
                                            anchors.rightMargin: 8
                                            color: tableFrame.uiTextHint
                                        }

                                        background: Rectangle {
                                            radius: 6
                                            color: tableFrame.uiFieldBg
                                            border.color: sexFilter.activeFocus ? tableFrame.uiFieldBorderFocus : tableFrame.uiFieldBorder
                                            border.width: 1
                                        }

                                        popup: Popup {
                                            y: sexFilter.height
                                            width: sexFilter.width
                                            padding: 0
                                            implicitHeight: Math.min(list.implicitHeight, 240)

                                            background: Rectangle {
                                                radius: 6
                                                color: tableFrame.uiFieldBg
                                                border.color: tableFrame.uiFieldBorder
                                                border.width: 1
                                            }

                                            contentItem: ListView {
                                                id: list
                                                clip: true
                                                implicitHeight: contentHeight
                                                model: sexFilter.model
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

                // нижняя панель с кнопкой
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#d9dce1" }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignRight
                        Layout.topMargin: 8

                        Button {
                            text: "Далее"
                            onClicked: {
                                if (selectedIsNew) {
                                    stack.push(nextPage, { existingMode: false, existingIndex: -1 })
                                } else if (selectedIndex >= 0) {
                                    stack.push(nextPage, { existingMode: true, existingIndex: selectedIndex })
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ===== Страница "Далее" =====
    Component {
        id: nextPage
        Page {
            id: pageNew

            header: ToolBar {
                RowLayout {
                    anchors.fill: parent
                    ToolButton { text: "←"; onClicked: stack.pop() }
                    Label {
                        text: pageNew.existingMode
                                ? "Пациент: " + (pageNew.pName || "—")
                                : "Создать нового пациента"
                        Layout.alignment: Qt.AlignVCenter
                    }
                }
            }

            // локальные данные
            property bool   existingMode: false
            property int    existingIndex: -1

            property string pName: ""
            property string pBirth: ""
            property string pSex: ""
            property string pPatientID: ""
            property string pPatientFolder: ""
            property string pFile: ""
            property string pStudyLabel: ""

            Component.onCompleted: {
                if (existingMode && existingIndex >= 0 && appLogic) {
                    // 1) пробуем через stub
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
                        // 2) фолбэк напрямую из модели
                        const g = appLogic.getPatientDemographics(existingIndex)
                        if (g && g.ok) {
                            pName          = g.fullName       || pName
                            pBirth         = g.birthYear      || pBirth
                            pSex           = g.sex            || pSex
                            pPatientID     = g.patientID      || pPatientID
                            pPatientFolder = g.patientFolder  || pPatientFolder
                        }
                    }
                }
            }

            ColumnLayout {
                //anchors.fill: parent
                anchors.left: parent.left
                anchors.right: parent.right  
                anchors.margins: 16
                spacing: 12
                Layout.alignment: Qt.AlignTop

                GroupBox {
                    title: "Данные пациента"
                    Layout.fillWidth: true

                    GridLayout {
                        columns: 2
                        rowSpacing: 8
                        columnSpacing: 12
                        Layout.fillWidth: true

                        Label { text: "ФИО: " }
                        TextField {
                            id: nameField
                            Layout.fillWidth: true
                            placeholderText: "ФИО"
                            text: pageNew.pName
                            onTextChanged: if (!pageNew.existingMode) pageNew.pName = text
                            enabled: !pageNew.existingMode
                            readOnly: pageNew.existingMode
                        }

                        Label { text: "Год рождения:" }
                        TextField {
                            id: birthField
                            Layout.fillWidth: true
                            placeholderText: "YYYY"
                            text: pageNew.pBirth
                            enabled: !pageNew.existingMode
                            readOnly: pageNew.existingMode

                            // ограничение длины
                            maximumLength: 4

                            // разрешаем только цифры
                            validator: IntValidator { bottom: 0; top: 9999 }

                            onTextChanged: if (!pageNew.existingMode) pageNew.pBirth = text
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

                        Label { text: "ID пациента:" }
                        TextField {
                            id: patientIdField
                            Layout.fillWidth: true
                            placeholderText: "ID"
                            text: pageNew.pPatientID
                            onTextChanged: if (!pageNew.existingMode) pageNew.pPatientID = text
                            enabled: !pageNew.existingMode
                            readOnly: pageNew.existingMode
                            validator: IntValidator { bottom: 0; top: 9999 }
                        }

                        Label { text: "Исследование:" }
                        TextField {
                            Layout.fillWidth: true
                            placeholderText: "Исследование"
                            text: pageNew.pStudyLabel

                            onTextChanged: {
                                // Оставляем только буквы, цифры и пробелы
                                let clean = text.replace(/[^a-zA-Zа-яА-Я0-9 ]/g, "")
                                if (clean !== text) {
                                    text = clean
                                }
                                pageNew.pStudyLabel = text
                            }
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
                        // 1) URL из доступного свойства (Qt 5/6)
                        var url = null
                        if (fileDialog.selectedFile)                 url = fileDialog.selectedFile         // Qt 6
                        else if (fileDialog.fileUrl)                 url = fileDialog.fileUrl              // Qt 5.15
                        else if (fileDialog.currentFile)             url = fileDialog.currentFile
                        else if (fileDialog.selectedFiles && fileDialog.selectedFiles.length > 0)
                            url = fileDialog.selectedFiles[0]
                        else if (fileDialog.fileUrls && fileDialog.fileUrls.length > 0)
                            url = fileDialog.fileUrls[0]

                        // 2) URL -> локальный путь
                        var localPath = ""
                        if (url && url.toLocalFile) {
                            localPath = url.toLocalFile()
                        } else if (url && url.toString) {
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

                        // ВАЖНО: больше здесь НИЧЕГО не делаем — исследование создадим по кнопке "Далее"
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
                        // >>> Перед любыми действиями задаём метку исследования для C++
                        if (appLogic && appLogic.studyLabel !== undefined)
                            appLogic.studyLabel = pageNew.pStudyLabel || "Study"

                        if (pageNew.existingMode) {
                            // ====== Существующий пациент ======
                            // Убедимся, что знаем папку пациента и его ID/демографию
                            if ((!pageNew.pPatientFolder || !pageNew.pPatientID) &&
                                appLogic && appLogic.getPatientDemographics && pageNew.existingIndex >= 0) {
                                const g = appLogic.getPatientDemographics(pageNew.existingIndex)
                                if (g && g.ok) {
                                    pageNew.pPatientFolder = pageNew.pPatientFolder || g.patientFolder
                                    pageNew.pPatientID     = pageNew.pPatientID     || g.patientID
                                    pageNew.pName          = pageNew.pName          || g.fullName
                                    pageNew.pBirth         = pageNew.pBirth         || g.birthYear
                                    pageNew.pSex           = pageNew.pSex           || g.sex
                                } else {
                                    console.warn("[QML] getPatientDemographics failed:", g ? g.error : "undefined")
                                    return
                                }
                            }

                            if (!pageNew.pPatientFolder || !pageNew.pPatientID) {
                                console.warn("[QML] patientFolder or patientID is empty")
                                return
                            }

                            // 1) Создаём исследование ТОЛЬКО сейчас
                            const study = appLogic.createStudyInPatientFolder(pageNew.pPatientFolder, pageNew.pPatientID)
                            if (!study || !study.ok) {
                                console.warn("[QML] Failed to create study:", study ? study.error : "undefined")
                                return
                            }

                            // 2) Если выбран файл — конвертируем в DICOM
                            if (pageNew.pFile && pageNew.pFile.length > 0 &&
                                appLogic && appLogic.convertAndSaveImageAsDicom) {

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
                                console.log("[QML] DICOM save (existing):", JSON.stringify(res))
                            } else {
                                console.log("[QML] No file selected; study folder created:", study.studyFolder)
                            }

                        } else {
                            // ====== Новый пациент ======
                            let patient = null
                            if (appLogic && appLogic.makePatientFromStrings) {
                                patient = appLogic.makePatientFromStrings(
                                    pageNew.pName, pageNew.pBirth, pageNew.pSex, pageNew.pPatientID
                                )
                            } else {
                                console.warn("appLogic.makePatientFromStrings не найден")
                                return
                            }

                            let study = null
                            if (patient && appLogic && appLogic.createStudyForNewPatient) {
                                study = appLogic.createStudyForNewPatient(
                                    patient.fullName, patient.birthYear, patient.patientID
                                )
                                if (!study || !study.studyFolder) {
                                    console.warn("[QML] Failed to create study folder")
                                    return
                                }

                                // Stub DICOM (демография)
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
                            } else {
                                return
                            }

                            // Сохраняем файл, если выбран
                            if (pageNew.pFile && pageNew.pFile.length > 0 &&
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
                                console.log("[QML] No file selected; study folder created:", study.studyFolder)
                            }
                        }

                        // Очистка и закрытие
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
