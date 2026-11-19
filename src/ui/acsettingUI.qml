import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#0F172A"

    // 【新增】全局悬浮提示框 (Toast)
    Rectangle {
        id: successToast
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        anchors.horizontalCenter: parent.horizontalCenter
        width: 180; height: 40
        radius: 20
        color: "#22c55e" // 绿色成功提示
        opacity: 0 // 默认隐藏
        visible: opacity > 0 // 透明时不可见且不遮挡点击
        z: 999 // 保证在最上层显示

        Text {
            anchors.centerIn: parent
            text: "✔ 指令下发成功"
            color: "#ffffff"
            font.pixelSize: 14
            font.bold: true
            font.family: "Microsoft YaHei"
        }

        // 透明度渐变动画
        Behavior on opacity { NumberAnimation { duration: 300 } }

        // 2秒后自动消失
        Timer {
            id: toastTimer
            interval: 500
            onTriggered: successToast.opacity = 0
        }
    }

    Rectangle {
        anchors.fill: parent; anchors.margins: 20
        color: "#1E293B"; radius: 15; border.color: "#334155"

        ColumnLayout {
            anchors.fill: parent; anchors.margins: 30; spacing: 20

            // 标题行与总开关
            RowLayout {
                Layout.fillWidth: true
                Text { text: "系统参数与独立控制策略"; color: "#00ffcc"; font.pixelSize: 20; font.bold: true }
                Item { Layout.fillWidth: true }

                Text { text: "全局总开关:"; color: "#e2e8f0"; font.pixelSize: 16 }
                Rectangle {
                    width: 54; height: 28; radius: 14
                    color: zoneModel.allPowerOn ? "#22c55e" : "#334155"
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Rectangle {
                        width: 24; height: 24; radius: 12; color: "#ffffff"
                        anchors.verticalCenter: parent.verticalCenter
                        x: zoneModel.allPowerOn ? parent.width - width - 2 : 2
                        Behavior on x { NumberAnimation { duration: 150 } }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: zoneModel.toggleAllZonesPower()
                    }
                }
                Text {
                    text: zoneModel.allPowerOn ? "全开" : "全关"
                    color: zoneModel.allPowerOn ? "#22c55e" : "#64748B"
                    font.pixelSize: 14
                    width: 40
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }

            // 2x2 网格设置区域
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: 20
                columnSpacing: 40

                Repeater {
                    model: zoneModel
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 150
                        color: "#0F172A"; radius: 10; border.color: "#334155"

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 15; spacing: 10

                            RowLayout {
                                Text { text: zoneName; color: "#38BDF8"; font.bold: true; font.pixelSize: 16 }
                                Item { Layout.fillWidth: true }

                                // 独立开关
                                Rectangle {
                                    width: 40; height: 22; radius: 11
                                    color: isPowerOn ? "#22c55e" : "#334155"
                                    Rectangle {
                                        width: 18; height: 18; radius: 9; color: "#fff"
                                        anchors.verticalCenter: parent.verticalCenter
                                        x: isPowerOn ? parent.width - width - 2 : 2
                                        Behavior on x { NumberAnimation { duration: 150 } }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: zoneModel.setZonePower(zoneId, !isPowerOn)
                                    }
                                }
                            }

                            RowLayout {
                                spacing: 10
                                Text { text: "目标温度:"; color: "#94A3B8"; font.pixelSize: 14 }

                                // 温度输入框
                                TextField {
                                    id: tempInput
                                    text: userSetTemp
                                    color: "#e2e8f0"; font.pixelSize: 14; implicitWidth: 70
                                    background: Rectangle { color: "#1E293B"; radius: 5; border.color: "#334155" }
                                }
                                Text { text: "℃"; color: "#94A3B8"; font.pixelSize: 14 }

                                // 模式选择
                                ComboBox {
                                    id: modeCombo
                                    model: ["制冷", "制热"]
                                    width: 80; height: 30
                                    Component.onCompleted: currentIndex = (modeStr === "制冷" ? 0 : 1)
                                    property string lastMode: modeStr
                                    onLastModeChanged: {
                                        if (modeStr === "制冷") currentIndex = 0;
                                        else if (modeStr === "制热") currentIndex = 1;
                                    }
                                    background: Rectangle { color: "#1E293B"; radius: 5; border.color: "#334155" }
                                    contentItem: Text { text: parent.displayText; color: "#e2e8f0"; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter; leftPadding: 5 }
                                }
                            }

                            Button {
                                text: "下发指令"
                                Layout.fillWidth: true
                                contentItem: Text { text: parent.text; color: "#0F172A"; font.bold: true; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                background: Rectangle { color: "#00ffcc"; radius: 6; implicitHeight: 30 }
                                onClicked: {
                                    var t = parseFloat(tempInput.text);
                                    if (!isNaN(t)) zoneModel.setTargetTemp(zoneId, t);
                                    zoneModel.setAcMode(zoneId, modeCombo.currentIndex);

                                    // 【新增】触发提示框显示
                                    successToast.opacity = 1;
                                    toastTimer.restart(); // 重新计时
                                }
                            }
                        }
                    }
                }
            }
            Item { Layout.fillHeight: true }
        }
    }
}