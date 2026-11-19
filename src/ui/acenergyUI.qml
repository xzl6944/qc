import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 // 需引入 Controls 用于列表和按钮

Rectangle {
    color: "#0F172A"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // ================= 顶部：数据卡片 =================
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            Layout.maximumHeight: 100
            spacing: 20

            // 累计耗电量
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: "#1E293B"; radius: 15; border.color: "#334155"
                RowLayout {
                    anchors.centerIn: parent; spacing: 15
                    Text { text: "⚡"; font.pixelSize: 35 }
                    Column { spacing: 5
                        Text { text: "累计耗电量"; color: "#94A3B8"; font.pixelSize: 16 }
                        Text { text: energyCalc.totalKwhStr; color: "#38BDF8"; font.pixelSize: 24; font.bold: true }
                    }
                }
            }

            // 累计节电量
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: "#1E293B"; radius: 15; border.color: "#334155"
                RowLayout {
                    anchors.centerIn: parent; spacing: 15
                    Text { text: "🍃"; font.pixelSize: 35 }
                    Column { spacing: 5
                        Text { text: "累计节电量"; color: "#94A3B8"; font.pixelSize: 16 }
                        Text { text: energyCalc.savedKwhStr; color: "#44cc44"; font.pixelSize: 24; font.bold: true }
                    }
                }
            }

            // CO₂ 减排
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: "#1E293B"; radius: 15; border.color: "#334155"
                RowLayout {
                    anchors.centerIn: parent; spacing: 15
                    Text { text: "🌍"; font.pixelSize: 35 }
                    Column { spacing: 5
                        Text { text: "CO₂ 减排"; color: "#94A3B8"; font.pixelSize: 16 }
                        Text { text: energyCalc.co2ReductionStr; color: "#ffcc00"; font.pixelSize: 24; font.bold: true }
                    }
                }
            }
        }

        // ================= 中间：图表 =================
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 300
            color: "#1E293B"; radius: 15; border.color: "#334155"

            Text {
                anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 15
                text: "实时功率与负荷趋势 (带平均基准线)"
                color: "#94A3B8"; font.pixelSize: 16
            }

            Text {
                anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 15
                text: "当前功率: " + energyCalc.currentPowerStr
                color: "#38BDF8"; font.pixelSize: 18; font.bold: true
            }

            Canvas {
                id: energyCanvas
                // 把外边距调小一点，因为我们会在内部代码里画左边距
                anchors.fill: parent; anchors.topMargin: 60; anchors.leftMargin: 10; anchors.bottomMargin: 50; anchors.rightMargin: 20

                Connections { target: chartModel; function onDataChanged() { energyCanvas.requestPaint(); } }

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);

                    var data = chartModel.tempHistory;
                    if (!data || data.length < 1) { return; }

                    var maxPower = 3.0;
                    var yScale = height / maxPower;
                    var totalPoints = 60;

                    // 【核心修改】手动定义绘图区，左边留出 60px 给刻度和文字
                    var startX = 60;
                    var chartWidth = width - startX;
                    var barWidth = (chartWidth / totalPoints) * 0.6;
                    var gap = (chartWidth / totalPoints) * 0.4;
                    var sumPower = 0;

                    // ==========================================
                    // 1. 画柱状图 (从 startX 开始画，避开左侧刻度)
                    // ==========================================
                    for (var j = 0; j < data.length; j++) {
                        var power = data[j].power;
                        sumPower += power;
                        if (power > maxPower) power = maxPower;
                        if (power < 0) power = 0;

                        var barHeight = (power / maxPower) * height;
                        var x = startX + j * (barWidth + gap); // <--- 这里加上 startX
                        var y = height - barHeight;

                        var gradient = ctx.createLinearGradient(0, y, 0, y + barHeight);
                        gradient.addColorStop(0, "#38BDF8");
                        gradient.addColorStop(1, "#1E3A8A");
                        ctx.fillStyle = gradient;
                        ctx.fillRect(x, y, barWidth, barHeight);
                    }

                    // ==========================================
                    // 2. 画 Y 轴网格线和刻度
                    // ==========================================
                    ctx.font = "12px 'Microsoft YaHei'";
                    ctx.fillStyle = "#94A3B8";
                    ctx.textAlign = "right";
                    ctx.textBaseline = "middle";

                    for(var i=0; i<=4; i++){
                        var v = maxPower * (i / 4);
                        var y = height - i * (height / 4);

                        // 横向网格线（从 startX 开始画）
                        ctx.strokeStyle = "#334155";
                        ctx.beginPath();
                        ctx.moveTo(startX, y);
                        ctx.lineTo(width, y);
                        ctx.stroke();

                        // 左侧刻度短线
                        ctx.beginPath();
                        ctx.moveTo(startX, y);
                        ctx.lineTo(startX - 6, y);
                        ctx.stroke();

                        // 【修复上下被切】文字绘制在 startX 左侧。对 0 和 3.00 进行微调。
                        var textY = y;
                        if (i === 4) textY += 6;  // 顶部文字往下压一点
                        if (i === 0) textY -= 6;  // 底部文字往上抬一点

                        ctx.fillText(v.toFixed(2) + " kW", startX - 15, textY);
                    }

                    // 底部 X 轴时间刻度
                    ctx.textAlign = "center";
                    ctx.textBaseline = "top";
                    for(var i=0; i<=3; i++){
                        var x = startX + i * (chartWidth / 3); // <--- 这里也要加上 startX
                        var txt = (i === 3) ? "现在" : "-" + (60 - i*20) + "s";
                        ctx.fillText(txt, x, height + 10);
                    }

                    // ==========================================
                    // 3. 画平均负荷线及说明文字
                    // ==========================================
                    if (data.length > 1) {
                        var avgPower = sumPower / data.length;
                        var avgY = height - (avgPower / maxPower) * height;

                        // 虚线（从 startX 开始画）
                        ctx.beginPath();
                        ctx.strokeStyle = "#ffcc00";
                        ctx.lineWidth = 2;
                        ctx.setLineDash([5, 5]);
                        ctx.moveTo(startX, avgY);
                        ctx.lineTo(width, avgY);
                        ctx.stroke();
                        ctx.setLineDash([]);

                        // 文字背景框及文字
                        var textStr = "平均负荷: " + avgPower.toFixed(2) + " kW";
                        var textWidth = ctx.measureText(textStr).width;
                        ctx.fillStyle = "rgba(30, 41, 59, 0.9)";
                        // 背景框的位置也向右移动避开柱子
                        ctx.fillRect(startX + 5, avgY - 22, textWidth + 10, 18);

                        ctx.fillStyle = "#ffcc00";
                        ctx.textAlign = "left";
                        ctx.textBaseline = "middle";
                        ctx.fillText(textStr, startX + 10, avgY - 13);
                    }
                }
            }
        }
    }
}