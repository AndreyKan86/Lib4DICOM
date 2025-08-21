import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

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
                                        onClicked: {
                                            selectedIsNew = true
                                            selectedIndex = -1
                                            patientsList.currentIndex = -1
                                        }
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
                                        onClicked: {
                                            selectedIsNew = false
                                            patientsList.currentIndex = index
                                            selectedIndex = index
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
                                    win.close()
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
            header: ToolBar {
                RowLayout {
                    anchors.fill: parent
                    ToolButton { text: "←"; onClicked: stack.pop() }
                    Label { text: "Следующее окно"; Layout.alignment: Qt.AlignVCenter }
                }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                Label { text: "Здесь будет интерфейс создания нового пациента."; Layout.fillWidth: true }
            }
        }
    }
}
