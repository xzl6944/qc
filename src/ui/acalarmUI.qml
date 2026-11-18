import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    color: "#0F172A"

    // 外层大卡片
    Rectangle {
        anchors.fill: parent
        anchors.margins: 20
        color: "#1E293B"
        radius: 15
        border.color: "#334155"
        border.width: 2

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15

            // 标题
            Text {
                text: "系统报警记录 (实时)"
                color: "#00ffcc"
                font.pixelSize: 20
                font.bold: true
                font.family: "Microsoft YaHei"
            }

            // 分割线
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#334155"
            }

            // 报警列表容器
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: alarmListView
                    anchors.fill: parent
                    model: alarmModel // 绑定 C++ 的报警模型
                    clip: true
                    spacing: 10

                    // 滚动条
                    ScrollBar.vertical: ScrollBar {
                        active: true
                        policy: ScrollBar.AsNeeded
                    }

                    // 每一条报警的样式
                    delegate: Rectangle {
                        width: alarmListView.width - 10 // 稍微留点空隙给滚动条
                        height: 50
                        radius: 8
                        border.color: "#334155"

                        // 根据报警等级动态改变背景色
                        color: {
                            if (level === "严重") return "#7f1d1d"; // 深红色
                            if (level === "中等") return "#78350f"; // 深橙色
                            return "#1e293b"; // 轻微或者普通用原底色
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 15
                            anchors.rightMargin: 15
                            spacing: 20

                            // 报警时间
                            Text {
                                text: time
                                color: "#94A3B8"
                                font.pixelSize: 15
                                font.family: "Consolas"
                            }

                            // 报警等级
                            Rectangle {
                                width: 50; height: 22
                                radius: 4
                                color: level === "严重" ? "#ef4444" : (level === "中等" ? "#f59e0b" : "#38BDF8")
                                Text {
                                    anchors.centerIn: parent
                                    text: level
                                    color: level === "中等" ? "#000000" : "#ffffff"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }

                            // 报警内容
                            Text {
                                Layout.fillWidth: true
                                text: message
                                color: level === "严重" ? "#fca5a5" : "#e2e8f0"
                                font.pixelSize: 16
                                font.family: "Microsoft YaHei"
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // 当列表为空时显示的提示文字
                    Text {
                        anchors.centerIn: parent
                        text: "暂无报警信息，系统运行正常"
                        color: "#64748B"
                        font.pixelSize: 18
                        font.family: "Microsoft YaHei"
                        visible: alarmModel.count === 0
                    }
                }
            }
        }
    }
}