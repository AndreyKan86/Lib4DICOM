import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 

ApplicationWindow {
    id: win
    width: 720
    height: 480
    visible: true
    title: "DICOM — пациенты"

Connections {
    target: win
    onProceed: {
        console.log("[QML] proceed:", payload)
    }
}

    // Пользовательский сигнал: «идём дальше» с полезной нагрузкой
    signal proceed(string payload)

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
                        text: patientsList.count === 0
                              ? "Пациенты (нет пациентов)"
                              : "Пациенты (" + patientsList.count + " найдено)"
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
                                        verticalAlignment: Qt.AlignVCenter
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
                                        verticalAlignment: Qt.AlignVCenter
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

                        // --- Данные ---
                        ScrollView {
                            id: sv
                            width: parent.width
                            height: parent.height - headerRow.height
                            clip: true
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded

                            ListView {
                                id: patientsList
                                width: parent.width
                                model: appLogic ? appLogic.patientModel : null
                                boundsBehavior: Flickable.StopAtBounds
                                clip: true
                                interactive: true

                                // Хедер — строка «Новый пациент»
                                header: Rectangle {
                                    width: patientsList.width
                                    height: 36
                                    color: selectedIsNew ? "#e6f2ff" : "white"

                                    Row {
                                        anchors.fill: parent
                                        spacing: 0

                                        // ФИО (Новый пациент)
                                        Rectangle {
                                            width: tableFrame.nameW
                                            height: parent.height
                                            color: "transparent"
                                            Text {
                                                anchors.fill: parent
                                                anchors.margins: 8
                                                text: "Новый пациент"
                                                elide: Text.ElideRight
                                                verticalAlignment: Text.AlignVCenter
                                                color: "black"
                                                font.bold: true
                                            }
                                        }
                                        // вертикальный разделитель
                                        Rectangle { width: tableFrame.sepW; height: parent.height; color: tableFrame.gridColor }

                                        // Год рождения (пусто/тире)
                                        Rectangle {
                                            width: tableFrame.yearW
                                            height: parent.height
                                            color: "transparent"
                                            Text {
                                                anchors.fill: parent
                                                text: "-"
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                                color: "black"
                                            }
                                        }
                                        // вертикальный разделитель
                                        Rectangle { width: tableFrame.sepW; height: parent.height; color: tableFrame.gridColor }

                                        // Пол (пусто/тире)
                                        Rectangle {
                                            width: tableFrame.sexW
                                            height: parent.height
                                            color: "transparent"
                                            Text {
                                                anchors.fill: parent
                                                text: "-"
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
                                        acceptedButtons: Qt.LeftButton
                                        hoverEnabled: true
     
                                        onClicked: {
                                            // просто выделяем "Новый пациент"
                                            selectedIsNew = true
                                            selectedIndex = -1
                                            patientsList.currentIndex = -1
                                        }

                                        onDoubleClicked: {
                                            // мгновенно переходим к форме создания пациента
                                            selectedIsNew = true
                                            selectedIndex = -1
                                            patientsList.currentIndex = -1
                                            stack.push(nextPage)
                                        }

                                        Keys.onEnterPressed: onDoubleClicked()
                                        Keys.onReturnPressed: onDoubleClicked()
                                    }



                                }

                                // Делегат обычных пациентов
                                delegate: Rectangle {
                                    height: 36
                                    width: patientsList.width
                                    color: (selectedIsNew ? false : ListView.isCurrentItem) ? "#e6f2ff" : "white"

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
                                            // двойной клик — выбираем пациента и завершаем (как "Далее" для существующего)
                                            selectedIsNew = false
                                            patientsList.currentIndex = index
                                            selectedIndex = index

                                            // при необходимости — шлём сигнал наверх (как у тебя было)
                                            win.proceed("START")

                                            // закрываем окно
                                            win.close()
                                        }
                                    }
                                }
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
                                    stack.push(nextPage)
                                } else {
                                    // Отправляем сигнал и закрываем окно
                                    win.proceed("START")
                                    stack.push(nextPage)
                                    //win.close()
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
                    Label { text: "Следующее окно"; Layout.alignment: Qt.AlignVCenter }
                }
            }

            // локальные данные пациента (ВСЕ объявлены на Page!)
            property string pName: ""
            property string pBirth: ""
            property string pSex: ""
            property string pFile: ""
            property string pPatientID: ""

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
                            onTextChanged: pageNew.pName = text
                        }

                        Label { text: "Дата рождения:" }
                        TextField {
                            id: birthField
                            Layout.fillWidth: true
                            placeholderText: "YYYY или YYYYMMDD"
                            inputMethodHints: Qt.ImhPreferNumbers
                            text: pageNew.pBirth
                            onTextChanged: pageNew.pBirth = text
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
                            onCurrentIndexChanged: pageNew.pSex = currentValue
                            Component.onCompleted: pageNew.pSex = currentValue
                        }

                        // --- Новые поля ---
                        Label { text: "ID пациента:" }
                        TextField {
                            id: patientIdField
                            Layout.fillWidth: true
                            placeholderText: "Напр.: P12345"
                            text: pageNew.pPatientID
                            onTextChanged: pageNew.pPatientID = text
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
                        const urlStr = selectedFile.toString()
                        const localPath = decodeURIComponent(urlStr.replace(/^file:\/+/, ""))

                        pageNew.pFile = localPath
                        filePathField.text = localPath

                        console.log("[FileDialog] URL:", urlStr)
                        console.log("[FileDialog] Local path:", localPath)

                        if (appLogic && appLogic.logSelectedFileAndPatient) {
                            appLogic.logSelectedFileAndPatient(
                                localPath,
                                pageNew.pName,
                                pageNew.pBirth,
                                pageNew.pSex
                            )
                        } else {
                            console.warn("appLogic.logSelectedFileAndPatient не найден")
                        }
                    }
                    onRejected: console.log("[FileDialog] отменено пользователем")
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    Layout.topMargin: 8



                    Button {
                        text: "Далее"
                        onClicked: {
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
                            if (win.selectedIsNew && patient && appLogic && appLogic.createStudyForNewPatient) {
                                study = appLogic.createStudyForNewPatient(
                                    patient.fullName, patient.birthYear, patient.patientID
                                )
                                if (!study || !study.studyFolder) {
                                    console.warn("[QML] Failed to create study folder")
                                    return
                                }

                                // 2.1) Создаём stub DICOM в корне папки пациента (демография)
                                if (appLogic && appLogic.createPatientStubDicom && study.patientFolder) {
                                    const stub = appLogic.createPatientStubDicom(
                                        study.patientFolder,            // корневой каталог пациента
                                        patient.patientID,
                                        patient.fullName,
                                        patient.birthYear,              // "YYYY" или "YYYYMMDD"
                                        patient.sex                     // "M"/"F"/"O"
                                    )
                                    if (!stub.ok) console.warn("[QML] Stub DICOM failed:", stub.error)
                                    else          console.log("[QML] Stub DICOM created:", stub.path)
                                } else {
                                    console.warn("[QML] createPatientStubDicom не найден или нет patientFolder")
                                }
                            }

                            // 3) Файл -> QVector<QImage> -> DICOM (всё внутри C++)
                            if (pageNew.pFile && pageNew.pFile.length > 0 && study &&
                                appLogic && appLogic.convertAndSaveImageAsDicom) {

                                const res = appLogic.convertAndSaveImageAsDicom(
                                  pageNew.pFile,
                                  study.studyFolder,
                                  patient.patientID,
                                  "SER01",
                                  study.studyUID,
                                  patient.fullName,
                                  patient.birthYear, // запишется как DA "YYYY" или "YYYYMMDD"
                                  patient.sex        // "M"/"F"/"O"
                                )
                                console.log("[QML] DICOM save:", JSON.stringify(res))
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
