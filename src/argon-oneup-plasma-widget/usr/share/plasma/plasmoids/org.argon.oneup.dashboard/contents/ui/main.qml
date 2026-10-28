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
        "model": "Argon ONE UP (CM5)",
        "cpu": {
            "cores_mhz": [2800, 2800, 2800, 2800],
            "max_mhz": 2800,
            "temp_c": 36.5,
            "governor": "ondemand"
        },
        "fan": {
            "rpm": 0,
            "pwm": 0,
            "percent": 0
        },
        "battery": {
            "present": true,
            "capacity": 88,
            "status": "Discharging",
            "power_w": 8.5,
            "voltage_v": 12.5,
            "current_ma": -680,
            "cycle_count": 27,
            "health": "Good",
            "ac_online": false
        },
        "storage": {
            "nvme_temp_c": 32.5
        },
        "memory": {
            "used_mb": 650,
            "total_mb": 8060,
            "percent": 8
        },
        "profile": "balanced"
    })

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
                    // Ignore JSON parsing errors during boot/transients
                }
            }
            disconnectSource(source)
        }

        function poll() {
            connectSource("/usr/bin/argon-oneup-telemetry --json")
        }

        function setProfile(prof) {
            connectSource("/usr/bin/argon-oneup-telemetry --set-profile " + prof)
            // Immediate re-poll shortly after
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
        Layout.minimumHeight: Kirigami.Units.gridUnit * 24
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20
        Layout.preferredHeight: Kirigami.Units.gridUnit * 26

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

            // ==================== CPU & CORES ====================
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
                    // SoC Temp Badge
                    Rectangle {
                        radius: 4
                        implicitWidth: tempLabel.implicitWidth + 10
                        implicitHeight: tempLabel.implicitHeight + 4
                        color: root.telemetry.cpu.temp_c < 55 ? Qt.rgba(0.2, 0.7, 0.3, 0.25) :
                               (root.telemetry.cpu.temp_c < 70 ? Qt.rgba(0.8, 0.6, 0.1, 0.3) : Qt.rgba(0.9, 0.2, 0.2, 0.35))
                        border.color: root.telemetry.cpu.temp_c < 55 ? Kirigami.Theme.positiveTextColor :
                                      (root.telemetry.cpu.temp_c < 70 ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.negativeTextColor)
                        border.width: 1

                        QQC2.Label {
                            id: tempLabel
                            anchors.centerIn: parent
                            text: root.telemetry.cpu.temp_c + " °C"
                            font.bold: true
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                        }
                    }
                }

                // 4 CPU Cores Grid
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
                                from: 600
                                to: root.telemetry.cpu.max_mhz > 0 ? root.telemetry.cpu.max_mhz : 2800
                                value: root.telemetry.cpu.cores_mhz[index] ? root.telemetry.cpu.cores_mhz[index] : 600
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
            }

            // Divider
            Kirigami.Separator {
                Layout.fillWidth: true
            }

            // ==================== BATTERY & POWER ====================
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
                        source: root.telemetry.battery.ac_online ? "battery-charging" : "battery-080"
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

            // ==================== COOLING & STORAGE ====================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                RowLayout {
                    Layout.fillWidth: true
                    QQC2.Label {
                        text: "KÜHLUNG & SPEICHER"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.8
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    // Fan Status
                    Kirigami.Icon {
                        source: "input-fan"
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: Kirigami.Units.iconSizes.small
                    }
                    QQC2.Label {
                        text: root.telemetry.fan.percent > 0 ?
                              ("Lüfter: " + root.telemetry.fan.rpm + " RPM (" + root.telemetry.fan.percent + "%)") :
                              "Lüfter: 0 RPM (Passiv / Silent)"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        Layout.fillWidth: true
                    }

                    // NVMe Temp
                    Kirigami.Icon {
                        source: "drive-harddisk"
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: Kirigami.Units.iconSizes.small
                    }
                    QQC2.Label {
                        text: "NVMe: " + root.telemetry.storage.nvme_temp_c + " °C"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                }

                // RAM Bar
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Kirigami.Icon {
                        source: "memory"
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: Kirigami.Units.iconSizes.small
                    }

                    QQC2.ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: root.telemetry.memory.percent
                    }

                    QQC2.Label {
                        text: (root.telemetry.memory.used_mb / 1024.0).toFixed(1) + " / " + (root.telemetry.memory.total_mb / 1024.0).toFixed(1) + " GB"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        horizontalAlignment: Text.AlignRight
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
                source: "computer-laptop"
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
