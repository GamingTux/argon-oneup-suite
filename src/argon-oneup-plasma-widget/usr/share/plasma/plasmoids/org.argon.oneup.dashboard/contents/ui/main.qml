import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasma5support as P5Support

PlasmoidItem {
    id: root

    preferredRepresentation: fullRepresentation
    Plasmoid.backgroundHints: PlasmaCore.Types.DefaultBackground | PlasmaCore.Types.ConfigurableBackground

    // Raw telemetry data model
    property var telemetry: ({
        "model": "Argon ONE UP Laptop (CM5)",
        "cpu": {
            "cores_mhz": [1500, 1500, 1500, 1500],
            "cores_load": [0, 0, 0, 0],
            "total_load": 0,
            "max_mhz": 2800,
            "temp_c": 36.5,
            "governor": "ondemand"
        },
        "gpu": {
            "percent": 0,
            "name": "VideoCore VII (V3D)"
        },
        "battery": {
            "present": true,
            "capacity": 85,
            "status": "Discharging",
            "power_w": 8.5,
            "voltage_v": 12.5,
            "current_ma": -680,
            "temp_c": 25.0,
            "cycle_count": 27,
            "health": "Good",
            "ac_online": false
        },
        "memory": {
            "used_mb": 1200,
            "total_mb": 8060,
            "percent": 15
        },
        "disks": [
            {
                "name": "NVMe SSD",
                "mount": "/",
                "icon": "drive-harddisk",
                "used_gb": 10.4,
                "total_gb": 238.0,
                "percent": 4
            }
        ],
        "temperatures": {
            "cpu_c": 36.5,
            "battery_c": 25.0,
            "nvme_c": 33.5
        },
        "fan": {
            "rpm": 0,
            "pwm": 0,
            "percent": 0
        },
        "profile": "balanced"
    })

    function getBatteryIcon() {
        if (!root.telemetry.battery.present) return "battery-missing"
        if (root.telemetry.battery.ac_online) return "battery-charging"
        var cap = root.telemetry.battery.capacity
        if (cap > 80) return "battery-good"
        if (cap > 30) return "battery"
        if (cap > 15) return "battery-caution"
        return "battery-low"
    }

    // Data Source using executable engine
    P5Support.DataSource {
        id: execSource
        engine: "executable"
        connectedSources: []

        onNewData: (source, data) => {
            var stdout = data["stdout"]
            if (stdout && stdout.length > 0) {
                try {
                    root.telemetry = JSON.parse(stdout)
                } catch (e) {
                    // Ignore JSON parsing transients
                }
            }
            disconnectSource(source)
        }

        function poll() {
            connectSource("/usr/bin/argon-oneup-telemetry --json")
        }

        function setProfile(prof) {
            connectSource("/usr/bin/argon-oneup-telemetry --set-profile " + prof)
            pollTimer.restart()
        }
    }

    Timer {
        id: pollTimer
        interval: 1500
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: execSource.poll()
    }

    fullRepresentation: Item {
        id: fullRep
        Layout.minimumWidth: Kirigami.Units.gridUnit * 18
        Layout.minimumHeight: Kirigami.Units.gridUnit * 26
        Layout.preferredWidth: Kirigami.Units.gridUnit * 22
        Layout.preferredHeight: Kirigami.Units.gridUnit * 30

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing * 2
            spacing: Kirigami.Units.smallSpacing * 2

            // ==================== HEADER ====================
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: "computer-laptop"
                    implicitWidth: Kirigami.Units.iconSizes.medium
                    implicitHeight: Kirigami.Units.iconSizes.medium
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Kirigami.Heading {
                        text: "Argon ONE UP"
                        level: 4
                        font.bold: true
                    }
                    QQC2.Label {
                        text: "Raspberry Pi CM5"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.7
                    }
                }

                // Profile Toggle Pill
                QQC2.Button {
                    id: profileBtn
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    flat: true
                    padding: Kirigami.Units.smallSpacing

                    contentItem: RowLayout {
                        spacing: 4
                        Kirigami.Icon {
                            source: root.telemetry.profile === "power-saver" ? "battery-profile-powersave" :
                                    (root.telemetry.profile === "performance" ? "battery-profile-performance" : "battery-profile-balanced")
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                        }
                        QQC2.Label {
                            text: root.telemetry.profile === "power-saver" ? "Eco" :
                                  (root.telemetry.profile === "performance" ? "Turbo" : "Balanced")
                            font.bold: true
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: root.telemetry.profile === "power-saver" ? Kirigami.Theme.positiveTextColor :
                                   (root.telemetry.profile === "performance" ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.highlightColor)
                        }
                    }

                    onClicked: {
                        var next = "balanced"
                        if (root.telemetry.profile === "balanced") next = "performance"
                        else if (root.telemetry.profile === "performance") next = "power-saver"
                        else next = "balanced"
                        execSource.setProfile(next)
                    }

                    QQC2.ToolTip.text: "Klicken zum Umschalten: Eco -> Balanced -> Turbo"
                    QQC2.ToolTip.visible: hovered
                }
            }

            // Divider
            Kirigami.Separator {
                Layout.fillWidth: true
            }

            // ==================== 1. PROZESSOR & GPU ====================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                RowLayout {
                    Layout.fillWidth: true
                    QQC2.Label {
                        text: "PROZESSOR (BCM2712)"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.8
                    }
                    Item { Layout.fillWidth: true }
                    QQC2.Label {
                        text: (root.telemetry.cpu.total_load !== undefined ? root.telemetry.cpu.total_load + "% · " : "") + root.telemetry.cpu.governor
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.7
                    }
                }

                // 4 CPU Cores Grid (Bar = CPU Load %, Label = Clock GHz)
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    rowSpacing: 4
                    columnSpacing: 8

                    Repeater {
                        model: root.telemetry.cpu.cores_mhz.length

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            QQC2.Label {
                                text: "C" + index + ":"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.7
                                Layout.minimumWidth: 20
                            }

                            QQC2.ProgressBar {
                                Layout.fillWidth: true
                                from: 0
                                to: 100
                                value: (root.telemetry.cpu.cores_load && root.telemetry.cpu.cores_load[index] !== undefined) ? root.telemetry.cpu.cores_load[index] : 0
                            }

                            QQC2.Label {
                                text: (root.telemetry.cpu.cores_mhz[index] / 1000.0).toFixed(2) + " GHz"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                font.family: "monospace"
                                Layout.minimumWidth: 55
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }
                }

                // GPU Load Row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Layout.topMargin: 2

                    Kirigami.Icon {
                        source: "video-display"
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: Kirigami.Units.iconSizes.small
                    }

                    QQC2.Label {
                        text: "GPU (V3D):"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.8
                        Layout.minimumWidth: 65
                    }

                    QQC2.ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: root.telemetry.gpu ? root.telemetry.gpu.percent : 0
                    }

                    QQC2.Label {
                        text: (root.telemetry.gpu ? root.telemetry.gpu.percent : 0) + " %"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        font.family: "monospace"
                        Layout.minimumWidth: 55
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            // Divider
            Kirigami.Separator {
                Layout.fillWidth: true
            }

            // ==================== 2. AKKU & ENERGIE ====================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                RowLayout {
                    Layout.fillWidth: true
                    QQC2.Label {
                        text: "AKKU & ENERGIE (CW2217)"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.8
                    }
                    Item { Layout.fillWidth: true }
                    QQC2.Label {
                        text: root.telemetry.battery.ac_online ? "🔌 Netzteil verbunden" : "🔋 Akkubetrieb"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        color: root.telemetry.battery.ac_online ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Kirigami.Icon {
                        source: root.getBatteryIcon()
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            QQC2.Label {
                                text: root.telemetry.battery.capacity + "%"
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize + 3
                            }
                            Item { Layout.fillWidth: true }
                            QQC2.Label {
                                text: root.telemetry.battery.power_w + " W"
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize + 1
                                color: root.telemetry.battery.ac_online ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.neutralTextColor
                            }
                        }

                        QQC2.ProgressBar {
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            value: root.telemetry.battery.capacity
                        }
                    }
                }

                // Battery Details Grid
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    rowSpacing: 2
                    columnSpacing: 10

                    QQC2.Label {
                        text: "Spannung: " + root.telemetry.battery.voltage_v + " V (" + root.telemetry.battery.current_ma + " mA)"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.8
                    }
                    QQC2.Label {
                        text: "Zyklen: " + root.telemetry.battery.cycle_count + " (" + root.telemetry.battery.health + ")"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.8
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            // Divider
            Kirigami.Separator {
                Layout.fillWidth: true
            }

            // ==================== 3. SPEICHER ====================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                QQC2.Label {
                    text: "SPEICHER"
                    font.bold: true
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    opacity: 0.8
                }

                // RAM Row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Kirigami.Icon {
                        source: "memory"
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: Kirigami.Units.iconSizes.small
                    }

                    QQC2.Label {
                        text: "RAM:"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        Layout.minimumWidth: 50
                    }

                    QQC2.ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: root.telemetry.memory.percent
                    }

                    QQC2.Label {
                        text: (root.telemetry.memory.used_mb / 1024.0).toFixed(1) + " / " + (root.telemetry.memory.total_mb / 1024.0).toFixed(1) + " GB (" + root.telemetry.memory.percent + "%)"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        horizontalAlignment: Text.AlignRight
                    }
                }

                // Disks & Drives (NVMe SSD, SD Card, USB Sticks)
                Repeater {
                    model: root.telemetry.disks ? root.telemetry.disks : []

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Kirigami.Icon {
                            source: modelData.icon || "drive-harddisk"
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                        }

                        QQC2.Label {
                            text: modelData.name + ":"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            Layout.minimumWidth: 50
                            elide: Text.ElideRight
                        }

                        QQC2.ProgressBar {
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            value: modelData.percent
                        }

                        QQC2.Label {
                            text: modelData.used_gb.toFixed(1) + " / " + modelData.total_gb.toFixed(1) + " GB (" + modelData.percent + "%)"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }
            }

            // Divider
            Kirigami.Separator {
                Layout.fillWidth: true
            }

            // ==================== 4. TEMPERATUREN & KÜHLUNG ====================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                QQC2.Label {
                    text: "TEMPERATUREN & KÜHLUNG"
                    font.bold: true
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    opacity: 0.8
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    rowSpacing: 6
                    columnSpacing: 10

                    // CPU Temp
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Kirigami.Icon {
                            source: "cpu"
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                        }
                        QQC2.Label {
                            text: "CPU / SoC:"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.8
                        }
                        Item { Layout.fillWidth: true }
                        QQC2.Label {
                            text: (root.telemetry.temperatures ? root.telemetry.temperatures.cpu_c : root.telemetry.cpu.temp_c) + " °C"
                            font.bold: true
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: ((root.telemetry.temperatures ? root.telemetry.temperatures.cpu_c : root.telemetry.cpu.temp_c) < 55) ?
                                   Kirigami.Theme.positiveTextColor :
                                   (((root.telemetry.temperatures ? root.telemetry.temperatures.cpu_c : root.telemetry.cpu.temp_c) < 70) ?
                                   Kirigami.Theme.neutralTextColor : Kirigami.Theme.negativeTextColor)
                        }
                    }

                    // Battery Temp
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Kirigami.Icon {
                            source: "battery"
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                        }
                        QQC2.Label {
                            text: "Akku:"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.8
                        }
                        Item { Layout.fillWidth: true }
                        QQC2.Label {
                            text: (root.telemetry.temperatures ? root.telemetry.temperatures.battery_c : root.telemetry.battery.temp_c) + " °C"
                            font.bold: true
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: ((root.telemetry.temperatures ? root.telemetry.temperatures.battery_c : root.telemetry.battery.temp_c) < 45) ?
                                   Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                        }
                    }

                    // NVMe Temp
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Kirigami.Icon {
                            source: "drive-harddisk"
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                        }
                        QQC2.Label {
                            text: "NVMe SSD:"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.8
                        }
                        Item { Layout.fillWidth: true }
                        QQC2.Label {
                            text: (root.telemetry.temperatures ? root.telemetry.temperatures.nvme_c : 0.0) + " °C"
                            font.bold: true
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: ((root.telemetry.temperatures ? root.telemetry.temperatures.nvme_c : 0.0) < 60) ?
                                   Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                        }
                    }

                    // Fan RPM
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Kirigami.Icon {
                            source: "input-fan"
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                        }
                        QQC2.Label {
                            text: "Lüfter:"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.8
                        }
                        Item { Layout.fillWidth: true }
                        QQC2.Label {
                            text: root.telemetry.fan.percent > 0 ?
                                  (root.telemetry.fan.rpm + " RPM (" + root.telemetry.fan.percent + "%)") :
                                  "0 RPM (Silent)"
                            font.bold: true
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: root.telemetry.fan.percent > 0 ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.positiveTextColor
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    // Panel Compact Representation
    compactRepresentation: MouseArea {
        implicitWidth: Kirigami.Units.iconSizes.medium + Kirigami.Units.smallSpacing * 2
        implicitHeight: Kirigami.Units.iconSizes.medium + Kirigami.Units.smallSpacing * 2

        onClicked: root.expanded = !root.expanded

        RowLayout {
            anchors.centerIn: parent
            spacing: 2

            Kirigami.Icon {
                source: root.getBatteryIcon()
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: Kirigami.Units.iconSizes.small
            }

            QQC2.Label {
                text: root.telemetry.battery.capacity + "%"
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.bold: true
            }
        }
    }
}
