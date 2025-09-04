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

                    // --- палитра для футера/полей ---
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
                                model: appLogic ? appLogic : null
                                boundsBehavior: Flickable.StopAtBounds
                                clip: true
                                interactive: true

                                // строка "Новый пациент"
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
                                            if (appLogic && appLogic.clearSelectedPatient)
                                                appLogic.clearSelectedPatient()
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
                                                text: fullName||""
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
                                                text: birthYear||""
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
                                                text: sex||""
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

                                            if (appLogic && appLogic.selectExistingPatient)
                                                appLogic.selectExistingPatient(index)
                                        }

                                        onDoubleClicked: {
                                            selectedIsNew = false
                                            patientsList.currentIndex = index
                                            selectedIndex = index

                                            if (appLogic && appLogic.selectExistingPatient)
                                                appLogic.selectExistingPatient(index)

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

                                // ОДНА строка поиска по ФИО на всю ширину
                                Rectangle {
                                    width: parent.width
                                    height: parent.height
                                    color: "transparent"

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 6

                                        TextField {
                                            id: searchName
                                            Layout.fillWidth: true
                                            placeholderText: "Поиск по ФИО…"
                                            text: tableFrame.filterName
                                            onTextChanged: tableFrame.filterName = text
                                            color: tableFrame.uiText
                                            placeholderTextColor: tableFrame.uiTextHint
                                            background: Rectangle {
                                                radius: 6
                                                color: tableFrame.uiFieldBg
                                                border.color: parent.focus
                                                             ? tableFrame.uiFieldBorderFocus
                                                             : tableFrame.uiFieldBorder
                                                border.width: 1
                                            }
                                        }

                                        ToolButton {
                                            text: "×"
                                            visible: searchName.text.length > 0
                                            onClicked: {
                                                searchName.clear()
                                                tableFrame.filterName = ""
                                            }
                                            background: Rectangle { radius: 6; color: "transparent" }
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
            property string pBirthYear: ""
            property string pBirthDA: ""
            property string pSex: ""
            property string pPatientID: ""
            property string pPatientFolder: ""
            property string pFile: ""
            property string pStudyLabel: ""

            function daysInMonth(y, m) {
                function leap(yy){ return (yy%4===0 && yy%100!==0) || (yy%400===0) }
                if ([1,3,5,7,8,10,12].indexOf(m) !== -1) return 31
                if ([4,6,9,11].indexOf(m) !== -1) return 30
                return leap(y) ? 29 : 28
            }
            function to2(n){ return n<10 ? "0"+n : ""+n }

            function syncBirthFromControls() {
                const y = yearSpin.value
                const m = monthCombo.currentValue || (monthCombo.currentIndex + 1)
                const d = daySpin.value
                pBirthYear = (""+y).slice(0,4)
                pBirthDA   = pBirthYear.length===4 ? (pBirthYear + to2(m) + to2(d)) : ""
            }

            function setControlsFromString(b) { // b: "YYYY" или "YYYYMMDD"
                if (!b) return
                const yy = b.slice(0,4); yearSpin.value = parseInt(yy || "2000")
                const mm = (b.length===8) ? parseInt(b.slice(4,6)) : 1
                const dd = (b.length===8) ? parseInt(b.slice(6,8)) : 1

                monthCombo.currentIndex = Math.max(0, Math.min(11, mm - 1))
                const m = monthCombo.currentValue || (monthCombo.currentIndex + 1)

                const dim = daysInMonth(yearSpin.value, m)
                daySpin.to = dim
                daySpin.value = Math.min(dd, dim)
                syncBirthFromControls()
            }




            Component.onCompleted: {
                if (existingMode && existingIndex >= 0 && appLogic) {
                    // 1) пробуем через stub
                    const stubInfo = appLogic.findPatientStubByIndex(existingIndex)
                    if (stubInfo && stubInfo.ok) {
                        pPatientFolder = stubInfo.patientFolder
                        const demo = appLogic.readDemographicsFromFile(stubInfo.stubPath)
                        if (demo && demo.ok) {
                            pName      = demo.patientName  || "--"
                            const b    = demo.patientBirth || ""   // "YYYY" или "YYYYMMDD"
                            pBirthYear = b.length >= 4 ? b.slice(0,4) : ""
                            pBirthDA   = (b.length === 8) ? b : (pBirthYear ? (pBirthYear + "0101") : "")
                            pSex       = demo.patientSex   || "O"
                            pPatientID = demo.patientID    || "--"
                        }
                    } else {
                        // 2) фолбэк напрямую из модели
                        const g = appLogic.getPatientDemographics(existingIndex)
                        if (g && g.ok) {
                            pName          = g.fullName       || pName
                            pBirthYear     = g.birthYear      || pBirthYear  // модель даёт только год
                            pBirthDA       = pBirthYear ? (pBirthYear + "0101") : pBirthDA
                            pSex           = g.sex            || pSex
                            pPatientID     = g.patientID      || pPatientID
                            pPatientFolder = g.patientFolder  || pPatientFolder
                        }
                    }
                    // настроим контролы даты
                    setControlsFromString(pBirthDA || pBirthYear)
                } else {
                    // новый пациент: дефолтная дата
                    setControlsFromString("20000101")
                }
                if (appLogic && appLogic.setSelectedBirthDA && pBirthDA && pBirthDA.length === 8) {
                    appLogic.setSelectedBirthDA(pBirthDA)    // передаём YYYYMMDD в C++
                }
            }

            ColumnLayout {
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
                            maximumLength: 50
                        }

                        // ----- Режим ввода даты рождения -----
                        Label { text: "Дата рождения:" }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            // День
                            SpinBox {
                                id: daySpin
                                from: 1
                                to: 31
                                value: 1
                                Layout.preferredWidth: 72
                                enabled: !pageNew.existingMode
                                editable: !pageNew.existingMode
                                onValueChanged: syncBirthFromControls()
                            }

                            // Месяц
                            ComboBox {
                                id: monthCombo
                                Layout.preferredWidth: 160
                                textRole: "text"
                                valueRole: "value"
                                enabled: !pageNew.existingMode
                                model: [
                                    { text: "Январь",   value: 1 },
                                    { text: "Февраль",  value: 2 },
                                    { text: "Март",     value: 3 },
                                    { text: "Апрель",   value: 4 },
                                    { text: "Май",      value: 5 },
                                    { text: "Июнь",     value: 6 },
                                    { text: "Июль",     value: 7 },
                                    { text: "Август",   value: 8 },
                                    { text: "Сентябрь", value: 9 },
                                    { text: "Октябрь",  value: 10 },
                                    { text: "Ноябрь",   value: 11 },
                                    { text: "Декабрь",  value: 12 }
                                ]

                                onCurrentIndexChanged: {
                                    const m = monthCombo.currentValue || (currentIndex + 1)
                                    daySpin.to = daysInMonth(yearSpin.value, m)
                                    if (daySpin.value > daySpin.to) daySpin.value = daySpin.to
                                    syncBirthFromControls()
                                }

                                Component.onCompleted: {
                                    if (currentIndex < 0) currentIndex = 0 // по умолчанию Январь
                                }
                            }

                            // Год
                            SpinBox {
                                id: yearSpin
                                from: 1900
                                to: (new Date()).getFullYear()
                                value: 2000
                                editable: !pageNew.existingMode
                                enabled: !pageNew.existingMode
                                Layout.preferredWidth: 110
                                onValueChanged: {
                                    const m = monthCombo.currentValue || (monthCombo.currentIndex + 1)
                                    daySpin.to = daysInMonth(yearSpin.value, m)
                                    if (daySpin.value > daySpin.to) daySpin.value = daySpin.to
                                    syncBirthFromControls()
                                }
                            }
                        }

                        Label { text: "Пол:" }
                        ComboBox {
                            id: sexCombo
                            Layout.fillWidth: true
                            textRole: "text"
                            model: [
                                { text: "Мужской", value: "M" },
                                { text: "Женский", value: "F" },
                                { text: "Другое/не указано", value: "O" }
                            ]
                            function currentVal() {
                                return (currentIndex >= 0 && model && model.length > currentIndex && model[currentIndex].value)
                                       ? model[currentIndex].value : "O"
                            }
                            onCurrentIndexChanged: {
                                if (!pageNew.existingMode)
                                    pageNew.pSex = currentVal()
                            }
                            Component.onCompleted: {
                                if (pageNew.existingMode) {
                                    const v = pageNew.pSex || "O"
                                    const idx = model.findIndex(m => m.value === v)
                                    if (idx >= 0) currentIndex = idx
                                } else {
                                    pageNew.pSex = currentVal()
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
                                let clean = text.replace(/[^a-zA-Zа-яА-Я0-9 ]/g, "")
                                if (clean !== text) text = clean
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
                        var url = null
                        if (fileDialog.selectedFile)                 url = fileDialog.selectedFile
                        else if (fileDialog.fileUrl)                 url = fileDialog.fileUrl
                        else if (fileDialog.currentFile)             url = fileDialog.currentFile
                        else if (fileDialog.selectedFiles && fileDialog.selectedFiles.length > 0)
                            url = fileDialog.selectedFiles[0]
                        else if (fileDialog.fileUrls && fileDialog.fileUrls.length > 0)
                            url = fileDialog.fileUrls[0]

                        var localPath = ""
                        if (url && url.toLocalFile)       localPath = url.toLocalFile()
                        else if (url && url.toString) {
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
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    Layout.topMargin: 8

                    Button {
                        text: "Готово"
                        onClicked: {
                            // 1) Синхронизируем дату из контролов в pBirthYear/pBirthDA
                            syncBirthFromControls()

                            // 2) Передаём полный YYYYMMDD в C++ (если доступно)
                            if (appLogic && appLogic.setSelectedBirthDA && pBirthDA && pBirthDA.length === 8)
                                appLogic.setSelectedBirthDA(pBirthDA)

                            // 3) Метка исследования для C++
                            if (appLogic && appLogic.studyLabel !== undefined)
                                appLogic.studyLabel = pageNew.pStudyLabel || "Study"

                            if (pageNew.existingMode) {
                                // === Существующий пациент ===
                                if (appLogic && appLogic.selectExistingPatient && pageNew.existingIndex >= 0)
                                    appLogic.selectExistingPatient(pageNew.existingIndex) // зафиксировать выбор в C++

                                // Подтянуть недостающую демографию (если нужно)
                                if ((!pageNew.pPatientFolder || !pageNew.pPatientID) &&
                                    appLogic && appLogic.getPatientDemographics && pageNew.existingIndex >= 0) {
                                    const g = appLogic.getPatientDemographics(pageNew.existingIndex)
                                    if (g && g.ok) {
                                        pageNew.pPatientFolder = pageNew.pPatientFolder || g.patientFolder
                                        pageNew.pPatientID     = pageNew.pPatientID     || g.patientID
                                        pageNew.pName          = pageNew.pName          || g.fullName
                                        pageNew.pBirthYear     = pageNew.pBirthYear     || g.birthYear
                                        pageNew.pBirthDA       = pageNew.pBirthYear ? (pageNew.pBirthYear + "0101") : pageNew.pBirthDA
                                        pageNew.pSex           = pageNew.pSex           || g.sex
                                        setControlsFromString(pageNew.pBirthDA || pageNew.pBirthYear)
                                        if (appLogic && appLogic.setSelectedBirthDA && pageNew.pBirthDA && pageNew.pBirthDA.length === 8)
                                            appLogic.setSelectedBirthDA(pageNew.pBirthDA)
                                    }
                                }

                                // Фолбэк через stub
                                if ((!pageNew.pPatientFolder || !pageNew.pPatientID) &&
                                    appLogic && appLogic.findPatientStubByIndex && pageNew.existingIndex >= 0) {
                                    const stubInfo = appLogic.findPatientStubByIndex(pageNew.existingIndex)
                                    if (stubInfo && stubInfo.ok) {
                                        pageNew.pPatientFolder = pageNew.pPatientFolder || stubInfo.patientFolder
                                        if (!pageNew.pPatientID && appLogic.readDemographicsFromFile && stubInfo.stubPath) {
                                            const demo = appLogic.readDemographicsFromFile(stubInfo.stubPath)
                                            if (demo && demo.ok) {
                                                pageNew.pPatientID = demo.patientID || pageNew.pPatientID
                                                if (!pageNew.pBirthDA && demo.patientBirth) {
                                                    pageNew.pBirthYear = demo.patientBirth.slice(0,4)
                                                    pageNew.pBirthDA   = (demo.patientBirth.length === 8)
                                                                         ? demo.patientBirth
                                                                         : (pageNew.pBirthYear ? (pageNew.pBirthYear + "0101") : "")
                                                    setControlsFromString(pageNew.pBirthDA || pageNew.pBirthYear)
                                                }
                                                pageNew.pName = pageNew.pName || demo.patientName
                                                pageNew.pSex  = pageNew.pSex  || (demo.patientSex || "O")
                                            }
                                        }
                                    }
                                }

                                if (!pageNew.pPatientFolder || !pageNew.pPatientID) {
                                    console.warn("[QML] patientFolder or patientID is empty (idx:", pageNew.existingIndex, ")")
                                    return
                                }

                                // 1) Создание исследования в папке пациента
                                const study = appLogic.createStudyInPatientFolder(pageNew.pPatientFolder, pageNew.pPatientID)
                                if (!study || !study.ok) {
                                    console.warn("[QML] Failed to create study:", study ? study.error : "undefined")
                                    return
                                }

                                // 2) Конвертация выбранного файла (если выбран)
                                if (pageNew.pFile && pageNew.pFile.length > 0 && appLogic && appLogic.convertAndSaveImageAsDicom) {
                                    appLogic.convertAndSaveImageAsDicom(pageNew.pFile)
                                } else {
                                    console.log("[QML] No file selected; study folder created:", study.studyFolder)
                                }

                            } else {
                                // === Новый пациент ===
                                if (!appLogic || !appLogic.makePatientFromStrings) {
                                    console.warn("appLogic.makePatientFromStrings не найден")
                                    return
                                }

                                // В КАРТУ — полный DA (для C++ он тоже сохранится в birthYear/birthDA)
                                const patient = appLogic.makePatientFromStrings(
                                    pageNew.pName, pageNew.pBirthDA, pageNew.pSex, pageNew.pPatientID
                                )
                                if (appLogic && appLogic.selectNewPatient)
                                    appLogic.selectNewPatient(patient)

                                // 1) Создание исследования для нового пациента
                                let study = null
                                if (appLogic && appLogic.createStudyForNewPatient) {
                                    study = appLogic.createStudyForNewPatient()
                                    if (!study || !study.studyFolder) {
                                        console.warn("[QML] Failed to create study folder")
                                        return
                                    }

                                    // 1a) Stub DICOM (демография)
                                    if (appLogic && appLogic.createPatientStubDicom && study.patientFolder) {
                                        const stub = appLogic.createPatientStubDicom(study.patientFolder)
                                        if (!stub.ok) console.warn("[QML] Stub DICOM failed:", stub.error)
                                        else          console.log("[QML] Stub DICOM created:", stub.path)
                                    } else {
                                        console.warn("[QML] createPatientStubDicom не найден или нет patientFolder")
                                    }
                                } else {
                                    return
                                }

                                // 2) Конвертация выбранного файла (если выбран)
                                if (pageNew.pFile && pageNew.pFile.length > 0 && appLogic && appLogic.convertAndSaveImageAsDicom) {
                                    appLogic.convertAndSaveImageAsDicom(pageNew.pFile)
                                } else {
                                    console.log("[QML] No file selected; study folder created:", study.studyFolder)
                                }
                            }

                            // Очистка
                            pageNew.pName = ""
                            pageNew.pBirthYear = ""
                            pageNew.pBirthDA   = ""
                            pageNew.pSex = sexCombo.currentVal()
                            pageNew.pFile = ""
                            pageNew.pPatientID = ""

                            appLogic.scanPatients()
                            stack.pop()
                        }
                    }
                }
            }
        }
    }
}
