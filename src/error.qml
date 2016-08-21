import QtQuick 2.4
import QtQuick.Window 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0

Window {
    color: "lightgrey";
    visible: true;
    width: 320;
    height: 240;

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: spacing

        TextArea {
            text: error
            color: "orangered"
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        }
        Button {
            text: qsTr("Close")
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            onClicked: Qt.quit()
        }
    }
}
