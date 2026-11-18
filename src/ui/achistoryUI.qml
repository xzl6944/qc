import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Rectangle {
    color: "#0F172A"

    property int currentPage: 1
    property int pageSize: 20
    property int totalPages: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // 顶部控制栏
        RowLayout {
            Layout.fillWidth: true
            Text { text: "全局总能耗历史记录"; color: "#38BDF8"; font.pixelSize: 22; font.bold: true; font.family: "Microsoft YaHei" }
            Item { Layout.fillWidth: true }

            Text { text: "日期:"; color: "#94A3B8"; font.pixelSize: 16 }
            ComboBox {
                id: dateCombo
                width: 130
                model: mainWindow ? mainWindow.getRecentDates() : []
                background: Rectangle { color: "#1E293B"; radius: 5; border.color: "#334155" }
                contentItem: Text { text: parent.displayText; color: "#e2e8f0"; font.pixelSize: 14; verticalAlignment: Text.AlignVCenter; leftPadding: 10 }
            }

            Button {
                text: "查询数据"
                contentItem: Text { text: parent.text; color: "#0F172A"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: "#00ffcc"; radius: 6; implicitWidth: 100; implicitHeight: 35 }
                onClicked: {
                    currentPage = 1;
                    loadPage();
                }
            }

            // 【新增】导出数据按钮
            Button {
                text: "导出CSV"
                contentItem: Text { text: parent.text; color: "#0F172A"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: "#38BDF8"; radius: 6; implicitWidth: 100; implicitHeight: 35 }
                onClicked: {
                    // 调用 C++ 的导出函数
                    var success = mainWindow.exportHistoryToCSV(dateCombo.currentText);
                    if (success) {
                        successPopup.open();
                    }
                }
            }
        }

        // 表格区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1E293B"; radius: 12; border.color: "#334155"

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 15; spacing: 0

                // 表头
                Rectangle {
                    Layout.fillWidth: true; height: 40; color: "#0F172A"; radius: 8
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 20; anchors.rightMargin: 20
                        Text { text: "时间"; color: "#94A3B8"; font.bold: true; font.pixelSize: 15; Layout.preferredWidth: 220 }
                        Text { text: "累计耗电量"; color: "#38BDF8"; font.bold: true; font.pixelSize: 15; Layout.fillWidth: true }
                        Text { text: "今日节电量"; color: "#44cc44"; font.bold: true; font.pixelSize: 15; Layout.fillWidth: true }
                        Text { text: "CO₂ 减排量"; color: "#ffcc00"; font.bold: true; font.pixelSize: 15; Layout.fillWidth: true }
                        Text { text: "实时功率"; color: "#e2e8f0"; font.bold: true; font.pixelSize: 15; Layout.fillWidth: true }
                    }
                }

                // 数据列表
                ListView {
                    id: listView
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true
                    model: ListModel { id: historyModel }

                    delegate: Rectangle {
                        width: listView.width; height: 40
                        color: index % 2 === 0 ? "transparent" : "#252526"

                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 20; anchors.rightMargin: 20
                            Text { text: model.time; color: "#e2e8f0"; font.pixelSize: 14; Layout.preferredWidth: 220 }
                            Text { text: model.total_kwh; color: "#38BDF8"; font.pixelSize: 14; font.bold: true; Layout.fillWidth: true }
                            Text { text: model.saved_kwh; color: "#44cc44"; font.pixelSize: 14; font.bold: true; Layout.fillWidth: true }
                            Text { text: model.co2; color: "#ffcc00"; font.pixelSize: 14; font.bold: true; Layout.fillWidth: true }
                            Text { text: model.power; color: "#e2e8f0"; font.pixelSize: 14; font.bold: true; Layout.fillWidth: true }
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: historyModel.count === 0 ? "当前日期暂无数据，请点击上方查询" : ""
                        color: "#64748B"; font.pixelSize: 16
                    }
                }
            }
        }

        // ================= 底部：分页控制 =================
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            Button {
                text: "上一页"
                enabled: currentPage > 1
                onClicked: { currentPage--; loadPage(); }
                background: Rectangle { color: enabled ? "#334155" : "#1E293B"; radius: 4; implicitWidth: 80; implicitHeight: 30 }
                contentItem: Text { text: parent.text; color: enabled ? "#e2e8f0" : "#64748B"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Text {
                text: "第 " + currentPage + " / " + totalPages + " 页"
                color: "#94A3B8"; font.pixelSize: 16; font.bold: true
            }

            Button {
                text: "下一页"
                enabled: currentPage < totalPages
                onClicked: { currentPage++; loadPage(); }
                background: Rectangle { color: enabled ? "#334155" : "#1E293B"; radius: 4; implicitWidth: 80; implicitHeight: 30 }
                contentItem: Text { text: parent.text; color: enabled ? "#e2e8f0" : "#64748B"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }

    // 【新增】导出成功的提示弹窗
    Popup {
        id: successPopup
        anchors.centerIn: parent
        width: 220; height: 90
        modal: true
        background: Rectangle { color: "#1E293B"; radius: 10; border.color: "#22c55e"; border.width: 2 }
        Text {
            anchors.centerIn: parent
            text: "✔ 报表导出成功！\n文件已保存至桌面"
            color: "#22c55e"
            font.pixelSize: 14
            font.family: "Microsoft YaHei"
            horizontalAlignment: Text.AlignHCenter
        }
        Timer {
            interval: 2000
            running: successPopup.visible
            onTriggered: successPopup.close()
        }
    }

    function loadPage() {
        if (mainWindow) {
            var res = mainWindow.queryHistory(dateCombo.currentText, currentPage, pageSize);
            historyModel.clear();
            var list = res.dataList;
            for (var i = 0; i < list.length; i++) {
                historyModel.append(list[i]);
            }
            totalPages = res.totalPages;
        }
    }
}