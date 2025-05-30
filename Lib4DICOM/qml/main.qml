import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    visible: true
    minimumWidth: 900
    minimumHeight: 600
    maximumWidth: 900
    maximumHeight: 600
    title: qsTr("Название программы")

    //Переменные
    property string patientID: ""
    property string studyID: ""
    property string seriesID: ""
    property string patientWeightKG: ""
    property string patientWeightG: ""
    property string patientSex: ""
    property string patinentAgeYear: ""
    property string patientAgeMonth: ""
    property string patinentAgeDay: ""
    property string patientName: ""
    property string patinentFamily: ""
    property string patinentFatherName: ""
    property string filePath: ""
    property string patientBirthday: ""

    //Верхняя плашка
    component TopLable: Rectangle {
           height: parent.height /10 * 2
           width: parent.width
           gradient: Gradient {
                GradientStop { position: 0.0; color: "#3a6186" }
                GradientStop { position: 1.0; color: "#49253e" }
           }

           radius: 2 
           border.width: 0
           Text {
                 text: "Информация о пациенте"
                 anchors.left: parent.left
                 anchors.verticalCenter: parent.verticalCenter
                 anchors.leftMargin: 10 
                 color: "lightgrey"
                 font.family: "Times New Roman"
                 font.pixelSize: 18
           }
    }

    //Нижняя  плашка
    component BottomLable: Rectangle {
            height: parent.height /10
            width: parent.width
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#3a6186" }
                GradientStop { position: 1.0; color: "#49253e" }
            }
            radius: 2 
            border.width: 0
            Text {
                 text: "Дополнительная информация"
                 anchors.left: parent.left
                 anchors.verticalCenter: parent.verticalCenter
                 anchors.leftMargin: 10 
                 color: "lightgrey"
                 font.family: "Times New Roman"
                 font.pixelSize: 18
            }
    }

    //Файл, день рождения ID  кнопка пуска
    component FileIdDate: Column {
       width: parent.width
       height: parent.height
       //ID
       Rectangle {
            width: parent.width
            height: parent.height / 3
            color: "transparent" 

            Rectangle {
              width: 1              
              height: parent.height * 3      
              color: "black"    
            }

            Row {
                spacing: 10
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter  
                
                //ID patient
                Text {
                    text: "ID пациента:"
                    color: "lightgrey"
                    font.family: "Times New Roman"
                    font.pixelSize: 12
                    height: 30
                    width: 60
                    verticalAlignment: Text.AlignVCenter
                }

                TextField {
                    id: patientID
                    width: 65
                    height: 30
                    placeholderText: ""
                    verticalAlignment: Text.AlignVCenter

                    onTextChanged: {
                        let filtered = text.replace(/[^0-9]/g, "");
                        if (filtered !== text) {
                        text = filtered;
                        }
                        root.patientID = text
                    }
                }
                
                //ID study
                Text {
                    text: "ID исследования:"
                    color: "lightgrey"
                    font.family: "Times New Roman"
                    font.pixelSize: 12
                    height: 30
                    width: 85
                    verticalAlignment: Text.AlignVCenter
                }

                TextField {
                    id: studyID
                    width: 65
                    height: 30
                    placeholderText: ""
                    verticalAlignment: Text.AlignVCenter

                    onTextChanged: {
                        let filtered = text.replace(/[^0-9]/g, "");
                        if (filtered !== text) {
                        text = filtered;
                        }
                        root.studyID = text
                    }
                }

                //ID series
                Text {
                    text: "ID серии:"
                    color: "lightgrey"
                    font.family: "Times New Roman"
                    font.pixelSize: 12
                    height: 30
                    width: 40
                    verticalAlignment: Text.AlignVCenter
                }

                TextField {
                    id: seriesID
                    width: 65
                    height: 30
                    placeholderText: ""
                    verticalAlignment: Text.AlignVCenter

                    onTextChanged: {
                        let filtered = text.replace(/[^0-9]/g, "");
                        if (filtered !== text) {
                        text = filtered;
                        }
                        root.studyUID = text
                    }
                }
            }
       }   
        //Дата рождения
       Rectangle {
          width: parent.width
          height: parent.height / 3
          color: "transparent" 
     
                RowLayout  {
                    spacing: 10
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: "Дата рождения:"
                        color: "lightgrey"
                        font.family: "Times New Roman"
                        font.pixelSize: 14
                        width: 100
                        height: 30
                        verticalAlignment: Text.AlignVCenter
                    }
                }
       }
       //Выбор файла и кнопка старт
       Rectangle {
             width: parent.width
             height: parent.height / 3
             color: "transparent" 
             Row {
                 spacing: 15
                 anchors.left: parent.left
                 anchors.leftMargin: 8
                 anchors.verticalCenter: parent.verticalCenter   

                 Text {
                      text: "Файл:"
                      color: "lightgrey"
                      font.family: "Times New Roman"
                      font.pixelSize: 14
                      height: 30
                      width: 60
                      verticalAlignment: Text.AlignVCenter
                 }

                TextField {
                      id: filePathField
                      width: 150
                      height: 30
                      readOnly: true
                      text: fileName.selectedFile
                      
                }

                Button {
                        text: "Обзор"
                        width: 90
                        height: 30
                        onClicked: fileName.open()
                }


                Button {
                        text: "Приступить"
                        width: 80
                        height: 30
                        onClicked: {  
                            appLogic.dataTransfer(root.patientID, root.studyID, root.seriesID, root.filePath, root.patientName, root.patientFamily, root.patinetFatherName,
                                                   root.patientSex, root.patientWeightKG,  root.patientWeightG,  
                                                   root.patinentAgeYear, root.patientAgeMonth, root.patinentAgeDay, root.patientBirthday )
                        }
                }

                FileDialog {
                     id: fileName
                     title: "Выберите файл"
                     nameFilters: ["Все файлы (*)", "Текстовые файлы (*.txt)", "Изображения (*.png *.jpg)"]
                     visible: false
                     onAccepted: {
                        root.filePath = fileName.selectedFile
                     }
                }
             }
       }
    }

    //Пол возраст вес
    component AgeWeightSex: Column {
       width: parent.width
       height: parent.height
        //Age
       Rectangle {
            width: parent.width
            height: parent.height / 3
            color: "transparent" 
            //Horizontal divider
            Rectangle {
              width: 1              
              height: parent.height * 3      
              color: "black"    
            }
            //Main component row
            Row {
                spacing: 10
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter  

                //Age
                Text {
                    text: "Возраст:"
                    color: "lightgrey"
                    font.family: "Times New Roman"
                    font.pixelSize: 16
                    height: 30
                    width: 60
                    verticalAlignment: Text.AlignVCenter
                }
                    //Year
                    TextField {
                        id: year
                        width: 40
                        height: 30
                        placeholderText: "лет"
                        verticalAlignment: Text.AlignVCenter

                        onTextChanged: {
                            let filtered = text.replace(/[^0-9]/g, "");
                            if (filtered !== text) {
                            text = filtered;
                            }
                            root.patinentAgeYear  = text
                        }
                    }
                    //Month
                    TextField {
                        id: month
                        width: 40
                        height: 30
                        placeholderText: "мес."
                        verticalAlignment: Text.AlignVCenter

                        onTextChanged: {
                            let filtered = text.replace(/[^0-9]/g, "");
                            if (filtered !== text) {
                            text = filtered;
                            }
                            root.patientAgeMonth = text
                        }
                    }
                    //Days
                    TextField {
                        id: day
                        width: 40
                        height: 30
                        placeholderText: "дней"
                        verticalAlignment: Text.AlignVCenter

                        onTextChanged: {
                            let filtered = text.replace(/[^0-9]/g, "");
                            if (filtered !== text) {
                            text = filtered;
                            }
                            root.patientAgeDay = text
                        }
                    }
            }
       }   
       //Weight
       Rectangle {
           width: parent.width
           height: parent.height / 3
           color: "transparent" 
                Row {
                     spacing: 10  
                     anchors.left: parent.left
                     anchors.leftMargin: 8
                     anchors.verticalCenter: parent.verticalCenter   
                    //Weight 
                    Text {
                         text: "Вес:"
                         color: "lightgrey"
                         font.family: "Times New Roman"
                         font.pixelSize: 16
                         width: 60
                         height: 30
                         verticalAlignment: Text.AlignVCenter
                    }
                    //Kg
                    TextField {
                        id: kg
                        width: 40
                        height: 30
                        placeholderText: "кг"
                        verticalAlignment: Text.AlignVCenter
                            onTextChanged: {
                                let filtered = text.replace(/[^0-9]/g, "");
                                if (filtered !== text) {
                                text = filtered;
                                }
                            root.patientWeightKG = text
                            }
                    }
                    //Grann
                    TextField {
                        id: gramm
                        width: 40
                        height: 30
                        placeholderText: "грамм"
                        verticalAlignment: Text.AlignVCenter
                            onTextChanged: {
                                let filtered = text.replace(/[^0-9]/g, "");
                                if (filtered !== text) {
                                text = filtered;
                                }
                            root.patinetnWeightGramm = text
                            }
                    }
                } 
        }
        //Sex
        Rectangle {
            width: parent.width
            height: parent.height / 3
            color: "transparent" 
              Row {
                 spacing: 10
                 anchors.left: parent.left
                 anchors.leftMargin: 8
                 anchors.verticalCenter: parent.verticalCenter   

                 Text {
                     text: "Пол:"
                     color: "lightgrey"
                     font.family: "Times New Roman"
                     font.pixelSize: 16
                     height: 30
                     width: 60
                     verticalAlignment: Text.AlignVCenter
                 }
         
                 ComboBox {
                      id: sex
                      width: 100
                      height: 30
                      model: ListModel {
                          ListElement { label: "Мужской"; value: "M" }
                          ListElement { label: "Женский"; value: "F" }
                      }
                      textRole: "label"


                      background: Rectangle {
                          color: "#323232"
                          radius: 4
                          border.color: "#323232"
                      }  

                      onCurrentIndexChanged: {
                          root.patientSex = model.get(currentIndex).value
                      }
                 }
              }
        }
    }

    //Фамилия имя отчество
    component FIO: Column {
       width: parent.width
       height: parent.height
       //Family
       Rectangle {
            width: parent.width
            height: parent.height / 3
            color: "transparent" 
            Row {
                spacing: 10
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter   
                //Text
                Text {
                    text: "Фамилия:"
                    color: "lightgrey"
                    font.family: "Times New Roman"
                    font.pixelSize: 16
                    height: 30
                    width: 60
                    verticalAlignment: Text.AlignVCenter
                }
                //Text field
                TextField {
                    id: family
                    width: 130
                    height: 30
                    placeholderText: "Введите фамилию"
                    verticalAlignment: Text.AlignVCenter
                    onTextChanged: {
                        let filtered = text.replace(/[^а-яА-Яa-zA-Z\s\-]/g, "");
                        if (filtered !== text) {
                            text = filtered;
                        }
                        root.patientFamily = text
                    }
                }
            }
       }   
       //Name
       Rectangle {
           width: parent.width
           height: parent.height / 3
           color: "transparent" 
                
               Row {
                         spacing: 10  
                         anchors.left: parent.left
                         anchors.leftMargin: 10
                         anchors.verticalCenter: parent.verticalCenter   
                         //Text
                         Text {
                             text: "Имя:"
                             color: "lightgrey"
                             font.family: "Times New Roman"
                             font.pixelSize: 16
                             width: 60
                             height: 30
                             verticalAlignment: Text.AlignVCenter
                         }
                         //Text field
                         TextField {
                                 id: name
                                 width: 130
                                 height: 30
                                 placeholderText: "Введите имя"
                                 verticalAlignment: Text.AlignVCenter
                                 onTextChanged: {
                                        let filtered = text.replace(/[^а-яА-Яa-zA-Z\s\-]/g, "");
                                        if (filtered !== text) {
                                        text = filtered;
                                        }
                                        root.patientName = text
                                 }
                         }
               }
       }
       //Father Name
       Rectangle {
        width: parent.width
        height: parent.height / 3
        color: "transparent" 
          Row {
             spacing: 10
             anchors.left: parent.left
             anchors.leftMargin: 10
             anchors.verticalCenter: parent.verticalCenter   // ⬅️ добавлено

             Text {
                 text: "Отчество:"
                 color: "lightgrey"
                 font.family: "Times New Roman"
                 font.pixelSize: 16
                 height: 30
                 width: 60
                 verticalAlignment: Text.AlignVCenter
             }

             TextField {
                id: fatherName
                width: 130
                height: 30
                placeholderText: "Введите отчество"
                verticalAlignment: Text.AlignVCenter
                    onTextChanged: {
                        let filtered = text.replace(/[^а-яА-Яa-zA-Z\s\-]/g, "");
                        if (filtered !== text) {
                        text = filtered;
                        }
                        root.patinetFatherName = text
                    }
             }
          }
       }
    }

    //Первый верхний контейнер
    component TopRow: Item {
            width: parent.width 
            height: parent.height * 0.8
            anchors.topMargin: parent.height * 0.2
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter

            Row {
                anchors.fill: parent
                Rectangle {
                    width: parent.width * 0.24
                    height: parent.height
                    color: "transparent"
                    FIO{}
                }

                Rectangle {
                    width: parent.width * 0.25
                    height: parent.height
                    color: "transparent"
                    AgeWeightSex{}
                }

                Rectangle {
                    width: parent.width * 0.52
                    height: parent.height
                    color: "transparent"

                    FileIdDate{}
                }
            }
    }

    //Верхний контейнер
    component TopPart: Rectangle {
            height: parent.height / 3
            width: parent.width
            color: "transparent"
            border.width: 0

            TopLable{}  //Верхняя надпись

            TopRow{}    //Нижняя надпись

            //Разделительная линия 
            Rectangle {
                height: 1
                width: parent.width
                color: "black"
                anchors.bottom: parent.bottom
                anchors.left: parent.left
            }
    }

    // Нижний контейнер
    component BottomPart: Rectangle {
            height: parent.height * 2 / 3
            width: parent.width
            color: "transparent"
            border.width: 0

            BottomLable{}

            Text {
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: 20
                text: "Место для вкладок"
            }
    }

    // Основной интерфейс
    Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#3a6186" }
                GradientStop { position: 1.0; color: "#49253e" }
            }

            Column {
                anchors.fill: parent

                // Используем TopPart
                TopPart {}

                // Используем BottomPart
                BottomPart {}
            }
    }
}