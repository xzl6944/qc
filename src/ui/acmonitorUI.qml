import QtQuick 2.15
import QtQuick.Layouts 1.15

Rectangle {
    color: "#0F172A"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // 左侧核心监控区
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 2
            rowSpacing: 10
            columnSpacing: 10

            Repeater {
                model: zoneModel
                delegate: ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 5

                    // --- 区域卡片 ---
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 155
                        color: isPowerOn ? "#1E293B" : "#121B2B" // 关机时背景变暗
                        radius: 10; border.color: "#334155"

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 12; spacing: 5

                            // 标题行 + 开关
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: zoneName; color: isPowerOn ? "#38BDF8" : "#64748B"; font.pixelSize: 14; font.bold: true }
                                Item { Layout.fillWidth: true }

                                // 独立开关 (绿色滑块)
                                Rectangle {
                                    width: 36; height: 20; radius: 10
                                    color: isPowerOn ? "#22c55e" : "#334155" // 绿色或灰色
                                    Behavior on color { ColorAnimation { duration: 150 } }

                                    Rectangle {
                                        width: 16; height: 16; radius: 8; color: "#ffffff"
                                        anchors.verticalCenter: parent.verticalCenter
                                        x: isPowerOn ? parent.width - width - 2 : 2
                                        Behavior on x { NumberAnimation { duration: 150 } }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: zoneModel.setZonePower(zoneId, !isPowerOn)
                                    }
                                }

                                // 模式与人员
                                Rectangle {
                                    width: 35; height: 18; radius: 3; color: isPowerOn ? (modeStr === "制冷" ? "#0ea5e9" : "#f97316") : "#334155"
                                    Text { anchors.centerIn: parent; text: modeStr; color: "#fff"; font.pixelSize: 10 }
                                }
                                Rectangle {
                                    width: 40; height: 18; radius: 3; color: "#0F172A"
                                    Text {
                                        anchors.centerIn: parent; text: (zoneId === 1 || zoneId === 3) ? "有人" : "无人"
                                        color: (zoneId === 1 || zoneId === 3) ? "#22c55e" : "#f59e0b"; font.pixelSize: 10
                                    }
                                }
                            }

                            // 温度大字
                            RowLayout {
                                Layout.fillWidth: true; spacing: 5
                                Text {
                                    text: isPowerOn ? currentTemp : "--";
                                    color: !isPowerOn ? "#64748B" : ((parseFloat(currentTemp) <= 26.0) ? "#00ffcc" : "#f59e0b");
                                    font.pixelSize: 32; font.bold: true
                                }
                                Column {
                                    anchors.bottom: parent.bottom; anchors.bottomMargin: 4
                                    Text { text: "℃"; color: "#64748B"; font.pixelSize: 12 }
                                    Text { text: isPowerOn ? comfortStatus : "已关闭"; color: !isPowerOn ? "#64748B" : (comfortStatus === "舒适" ? "#22c55e" : "#f59e0b"); font.pixelSize: 10 }
                                }
                            }

                            // 设定与策略
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "设定: " + userSetTemp + "℃"; color: "#94A3B8"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: (userSetTemp !== targetTemp) ? "策略: " + targetTemp + "℃" : "目标: " + targetTemp + "℃"
                                    color: (userSetTemp !== targetTemp) ? "#f59e0b" : "#e2e8f0"; font.pixelSize: 11; font.bold: (userSetTemp !== targetTemp)
                                }
                            }

                            // 进度条
                            Rectangle {
                                Layout.fillWidth: true; height: 4; color: "#0F172A"; radius: 2
                                Rectangle {
                                    width: parent.width * (parseFloat(valveOpening) / 100.0)
                                    height: parent.height; color: isPowerOn ? "#38BDF8" : "#334155"; radius: 2
                                }
                            }

                            // 阀门、新风、CO2 数据行
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "水阀: " + valveOpening + "%"; color: "#64748B"; font.pixelSize: 10 }
                                Item { Layout.fillWidth: true }

                                // 动态高亮：新风>10%时变为亮蓝色，否则灰色
                                Text {
                                    text: "新风: " + freshAir + "%";
                                    color: parseFloat(freshAir) > 10 ? "#00ffcc" : "#38BDF8"; // 超过10%变亮青色
                                    font.pixelSize: 10; font.bold: parseFloat(freshAir) > 10
                                }
                                Item { Layout.fillWidth: true }

                                // 动态高亮：CO2>800时变为黄色，>1000变为红色
                                Text {
                                    text: "CO₂: " + co2Level + " ppm";
                                    color: co2Level > 1000 ? "#ef4444" : (co2Level > 800 ? "#f59e0b" : "#44cc44");
                                    font.pixelSize: 10; font.bold: co2Level > 800
                                }
                            }
                        }
                    }

                    // --- 曲线图 ---
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#1E293B"; radius: 10; border.color: "#334155"

                        // 【新增】悬浮图例标识
                        RowLayout {
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.margins: 10
                            spacing: 15

                            // 实线图例（当前温度）
                            RowLayout {
                                spacing: 5
                                Rectangle {
                                    width: 12; height: 2
                                    // 颜色与底部画线逻辑保持完全一致
                                    color: (index === 0) ? "#00ffcc" : ((index === 1) ? "#ffcc00" : ((index === 2) ? "#f97316" : "#a855f7"))
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text { text: "当前温度"; color: "#94A3B8"; font.pixelSize: 10; font.family: "Microsoft YaHei" }
                            }

                            // 虚线图例（目标温度）
                            RowLayout {
                                spacing: 5
                                Canvas {
                                    width: 12; height: 2
                                    onPaint: {
                                        var ctx = getContext("2d");
                                        ctx.strokeStyle = "#64748B";
                                        ctx.setLineDash([2, 2]);
                                        ctx.lineWidth = 1;
                                        ctx.beginPath();
                                        ctx.moveTo(0, 1);
                                        ctx.lineTo(width, 1);
                                        ctx.stroke();
                                    }
                                }
                                Text { text: "目标温度"; color: "#94A3B8"; font.pixelSize: 10; font.family: "Microsoft YaHei" }
                            }
                        }

                        // 你的原有 Canvas 保持不变
                        Canvas {
                            id: chartCanvas
                            anchors.fill: parent; anchors.margins: 5
                            Connections { target: chartModel; function onDataChanged() { chartCanvas.requestPaint(); } }
                            onPaint: {
                                var ctx = getContext("2d"); ctx.save(); ctx.reset(); ctx.clearRect(0, 0, width, height);

                                if (!isPowerOn) { ctx.restore(); return; }

                                var data = chartModel.tempHistory;
                                if (data.length < 2) { ctx.restore(); return; }

                                var tempKey = "t" + (index + 1); var tarKey = "tar" + (index + 1);
                                var lineColor = !isPowerOn ? "#475569" : ((index === 0) ? "#00ffcc" : ((index === 1) ? "#ffcc00" : ((index === 2) ? "#f97316" : "#a855f7")));

                                var mLeft = 35, mRight = 10, mTop = 15, mBottom = 20;
                                var chartW = width - mLeft - mRight; var chartH = height - mTop - mBottom;
                                var minT = 16, maxT = 32; var yScale = chartH / (maxT - minT);

                                ctx.font = "10px 'Microsoft YaHei'"; ctx.fillStyle = "#94A3B8"; ctx.textAlign = "right"; ctx.textBaseline = "middle";
                                for(var i=0; i<=2; i++) {
                                    var v = minT + (maxT-minT)*(i/2); var y = mTop + chartH - i*(chartH/2);
                                    ctx.strokeStyle = "#334155"; ctx.beginPath(); ctx.moveTo(mLeft, y); ctx.lineTo(width - mRight, y); ctx.stroke();
                                    ctx.fillText(v.toFixed(0) + "℃", mLeft - 5, y);
                                }

                                var xStep = chartW / 59;
                                ctx.beginPath(); ctx.strokeStyle = lineColor; ctx.lineWidth = 2;
                                for(var j=0; j<data.length; j++){
                                    var x = mLeft + j * xStep; var y1 = mTop + chartH - (data[j][tempKey] - minT) * yScale;
                                    if(j === 0) ctx.moveTo(x, y1); else ctx.lineTo(x, y1);
                                }
                                ctx.stroke();

                                ctx.beginPath(); ctx.strokeStyle = "#64748B"; ctx.setLineDash([4, 4]); ctx.lineWidth = 1;
                                for(var j=0; j<data.length; j++){
                                    var x = mLeft + j * xStep; var y2 = mTop + chartH - (data[j][tarKey] - minT) * yScale;
                                    if(j === 0) ctx.moveTo(x, y2); else ctx.lineTo(x, y2);
                                }
                                ctx.stroke(); ctx.setLineDash([]); ctx.restore();
                            }
                        }
                    }
                }
            }
        }

        // 右侧能效面板
        Rectangle {
            Layout.preferredWidth: 260; Layout.fillHeight: true
            color: "#1E293B"; radius: 15; border.color: "#334155"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 15; spacing: 15
                Text { text: "系统综合能效"; color: "#94A3B8"; font.pixelSize: 18; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                // 【修改点】这里是动态仪表盘
                Item {
                    Layout.preferredWidth: 180; Layout.preferredHeight: 100; Layout.alignment: Qt.AlignHCenter
                    Canvas {
                        id: gaugeCanvas
                        anchors.fill: parent

                        // 监听 C++ 模型数据变化刷新
                        Connections {
                            target: energyCalc
                            function onDataChanged() { gaugeCanvas.requestPaint(); }
                        }

                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            var cx = width / 2;
                            var cy = height - 10;
                            var r = 65;

                            var score = energyCalc.efficiencyScore;

                            // 1. 画底部的灰色背景弧
                            ctx.beginPath();
                            ctx.arc(cx, cy, r, Math.PI, Math.PI * 2);
                            ctx.lineWidth = 12;
                            ctx.strokeStyle = "#334155";
                            ctx.stroke();

                            // 2. 画蓝色的动态进度弧
                            var endAngle = Math.PI + (score / 100.0) * Math.PI;
                            ctx.beginPath();
                            ctx.arc(cx, cy, r, Math.PI, endAngle);
                            ctx.lineWidth = 12;
                            ctx.strokeStyle = score >= 90 ? "#00ffcc" : (score >= 60 ? "#38BDF8" : "#ef4444");
                            ctx.stroke();
                        }
                    }
                    Column {
                        anchors.centerIn: parent; anchors.verticalCenterOffset: 15
                        Text { text: energyCalc.efficiencyScore.toString(); color: energyCalc.efficiencyScore >= 60 ? "#38BDF8" : "#ef4444"; font.pixelSize: 36; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                        Text { text: "分"; color: "#64748B"; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#334155" }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 10
                    RowLayout { Text { text: "实时功率:"; color: "#94A3B8"; font.pixelSize: 14 } Item { Layout.fillWidth: true } Text { text: energyCalc.currentPowerStr; color: "#38BDF8"; font.pixelSize: 15; font.bold: true } }
                    RowLayout { Text { text: "今日节电:"; color: "#94A3B8"; font.pixelSize: 14 } Item { Layout.fillWidth: true } Text { text: energyCalc.savedKwhStr; color: "#22c55e"; font.pixelSize: 15; font.bold: true } }
                    RowLayout { Text { text: "CO₂减排:"; color: "#94A3B8"; font.pixelSize: 14 } Item { Layout.fillWidth: true } Text { text: energyCalc.co2ReductionStr; color: "#22c55e"; font.pixelSize: 15; font.bold: true } }
                    RowLayout { Text { text: "累计耗电:"; color: "#94A3B8"; font.pixelSize: 14 } Item { Layout.fillWidth: true } Text { text: energyCalc.totalKwhStr; color: "#f59e0b"; font.pixelSize: 15; font.bold: true } }
                }
                Item { Layout.fillHeight: true }
            }
        }
    }
}