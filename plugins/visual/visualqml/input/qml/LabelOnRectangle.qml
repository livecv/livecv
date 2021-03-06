import QtQuick 2.3
import workspace 1.0 as Workspace
import visual.input 1.0 as Input

Rectangle{
    id: root
    width: 50
    height: 20
    color: style.background
    radius: style.radius

    property alias label: label

    property QtObject defaultStyle: Input.LabelOnRectangleStyle{}
    property QtObject style: defaultStyle

    property int margins: 0
    property string text: 'Label'

    Input.Label{
        id: label
        anchors.centerIn: parent
        text: root.text
        textStyle: root.style.textStyle
    }
}
