#!/usr/bin/env python3
# Author: Irakli Keshelashvili, 2026
#
# SuFDeMon - Super-FRS Detector Monitoring Software
# ROOT-based client-server monitoring for MUSIC, PLSCI and SCIFI detectors.
#
# Copyright (C) 2026 Irakli Keshelashvili
# This program is free software under the GNU General Public License,
# version 3. See LICENSE in the project root for the full license text.
"""Local detector server controls backed by the existing Screen scripts."""

import codecs
import os
from pathlib import Path
import re
import sys

from PyQt6.QtCore import QProcess, QProcessEnvironment, QTimer
from PyQt6.QtGui import QCloseEvent, QTextCursor
from PyQt6.QtWidgets import (
    QApplication, QDialog, QDialogButtonBox, QGroupBox, QHBoxLayout,
    QLabel, QMainWindow, QPlainTextEdit, QPushButton, QVBoxLayout, QWidget,
)


def natural_key(path: Path):
    return [int(part) if part.isdigit() else part.lower()
            for part in re.split(r"(\d+)", path.stem)]


class ServerControlWindow(QMainWindow):
    def __init__(self, repo_dir: Path | None = None):
        super().__init__()
        self.repo_dir = (repo_dir or Path(__file__).resolve().parents[1]).resolve()
        # Resolve relative environment overrides before setting the subprocess cwd.
        self.environment = QProcessEnvironment.systemEnvironment()
        for name in ("BUILD_DIR", "CONFIG_DIR", "LOG_DIR"):
            if name in os.environ:
                self.environment.insert(name, str(Path(os.environ[name]).resolve()))
        self.config_dir = Path(self.environment.value(
            "CONFIG_DIR", str(self.repo_dir / "config/servers")))
        self.busy = False
        self.action_generation = 0
        self.poll_generation = 0
        self.decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
        self.setWindowTitle("SuFDeMon — Server Control")
        self.resize(960, 800)
        self.server_buttons = {}
        self.server_states = {}
        self.last_status_error = None

        self.output_dialog = QDialog(self)
        self.output_dialog.setWindowTitle("SuFDeMon — Script Output")
        self.output_dialog.resize(780, 420)
        output_layout = QVBoxLayout(self.output_dialog)
        self.output = QPlainTextEdit()
        self.output.setReadOnly(True)
        self.output.setMaximumBlockCount(5000)
        output_layout.addWidget(self.output)
        close_output = QDialogButtonBox(QDialogButtonBox.StandardButton.Close)
        close_output.rejected.connect(self.output_dialog.hide)
        output_layout.addWidget(close_output)

        central = QWidget()
        layout = QVBoxLayout(central)
        self.setCentralWidget(central)

        # Row 1: general actions for every local server session.
        general = QGroupBox("General — all local servers")
        general_layout = QVBoxLayout(general)
        general_layout.addWidget(QLabel("Run server processes on this PC using GNU Screen."))
        buttons = QHBoxLayout()
        self.start_all = QPushButton("Start All")
        self.stop_all = QPushButton("Stop All")
        self.show_output = QPushButton("Script Output")
        self.refresh_status = QPushButton("Refresh Status")
        for button in (self.start_all, self.stop_all, self.refresh_status, self.show_output):
            buttons.addWidget(button)
        general_layout.addLayout(buttons)
        layout.addWidget(general)

        # Row 2: one vertical column for each detector type.
        columns = QHBoxLayout()
        self.column_layouts = {}
        for detector in ("MUSIC", "PLSCI", "SCIFI"):
            group = QGroupBox(detector)
            column = QVBoxLayout(group)
            column.setSpacing(5)
            self.column_layouts[detector] = column
            columns.addWidget(group, 1)
        layout.addLayout(columns, 1)
        self.load_configs()

        self.process = QProcess(self)
        self.process.setProcessChannelMode(QProcess.ProcessChannelMode.MergedChannels)
        self.process.setWorkingDirectory(str(self.repo_dir))
        self.process.setProcessEnvironment(self.environment)
        self.process.readyReadStandardOutput.connect(self.read_output)
        self.process.errorOccurred.connect(self.process_error)
        self.process.finished.connect(self.process_finished)
        self.start_all.clicked.connect(lambda: self.run_script("start"))
        self.stop_all.clicked.connect(lambda: self.run_script("stop"))
        self.show_output.clicked.connect(self.open_output)
        self.refresh_status.clicked.connect(self.poll_status)

        self.status_process = QProcess(self)
        self.status_process.setProcessChannelMode(QProcess.ProcessChannelMode.MergedChannels)
        status_environment = QProcessEnvironment(self.environment)
        status_environment.insert("LC_ALL", "C")
        self.status_process.setProcessEnvironment(status_environment)
        self.status_process.finished.connect(self.status_finished)
        self.status_process.errorOccurred.connect(self.status_error)
        self.status_timeout = QTimer(self)
        self.status_timeout.setSingleShot(True)
        self.status_timeout.setInterval(5000)
        self.status_timeout.timeout.connect(self.status_process.kill)
        self.timer = QTimer(self)
        self.timer.setInterval(2000)
        self.timer.timeout.connect(self.poll_status)
        self.timer.start()
        self.update_buttons()
        self.poll_status()

    def update_buttons(self):
        self.start_all.setEnabled(not self.busy and bool(self.server_buttons))
        self.stop_all.setEnabled(not self.busy)
        for name, button in self.server_buttons.items():
            state = self.server_states.get(name)
            button.setEnabled(not self.busy and state is not None)
            label = "Running — Stop" if state is True else "Stopped — Start" if state is False else "Checking / unavailable"
            button.setText(f"{name}\n{label}")
            color = "#176c37" if state is True else "#454e5a" if state is False else "#885800"
            button.setStyleSheet(f"QPushButton {{ color: white; background: {color}; border-radius: 4px; padding: 4px; }}"
                                "QPushButton:disabled { color: #bbbbbb; background: #525252; }"
                                "QPushButton:hover { border: 2px solid #8fbaff; }")

    def load_configs(self):
        errors = []
        try:
            configs = sorted(self.config_dir.glob("*.conf"), key=natural_key)
        except OSError as error:
            configs = []
            errors.append(str(error))
        for config in configs:
            if not re.fullmatch(r"[A-Za-z0-9_-]+", config.stem):
                errors.append(f"Invalid config filename: {config.name}")
                continue
            try:
                values = {}
                for line in config.read_text().splitlines():
                    line = line.strip()
                    if line and not line.startswith("#") and "=" in line:
                        key, value = line.split("=", 1)
                        values[key.strip()] = value.strip()
            except (OSError, UnicodeError) as error:
                errors.append(f"{config.name}: {error}")
                continue
            detector = values.get("type")
            if detector not in self.column_layouts:
                errors.append(f"Unsupported detector type in {config.name}")
                continue
            button = QPushButton()
            button.setMinimumHeight(39)
            button.setToolTip(f"{values.get('hostname', '?')}:{values.get('port', '?')}\n"
                              f"Local Screen session: SuFDeMon-{config.stem}")
            button.clicked.connect(lambda checked=False, name=config.stem: self.toggle_server(name))
            self.server_buttons[config.stem] = button
            self.server_states[config.stem] = None
            self.column_layouts[detector].addWidget(button)
        for column in self.column_layouts.values():
            column.addStretch()
        if errors:
            self.output.appendPlainText("\n".join(errors))
        self.statusBar().showMessage(f"{len(self.server_buttons)} configurations loaded — checking local sessions…")

    def toggle_server(self, name: str):
        state = self.server_states.get(name)
        if state is not None:
            self.run_script("stop" if state else "start", [name])

    def poll_status(self):
        if self.busy or self.status_process.state() != QProcess.ProcessState.NotRunning:
            return
        self.poll_generation = self.action_generation
        self.status_process.start("screen", ["-ls"])
        self.status_timeout.start()

    def set_status_unavailable(self, message: str):
        self.server_states = dict.fromkeys(self.server_buttons)
        self.update_buttons()
        if not self.busy:
            self.statusBar().showMessage("Screen status unavailable — see Script Output")
        if message != self.last_status_error:
            self.output.appendPlainText(message)
            self.last_status_error = message

    def status_error(self, error: QProcess.ProcessError):
        if error == QProcess.ProcessError.FailedToStart:
            self.status_timeout.stop()
            self.set_status_unavailable(self.status_process.errorString())

    def status_finished(self, code: int, status: QProcess.ExitStatus):
        self.status_timeout.stop()
        text = bytes(self.status_process.readAllStandardOutput()).decode("utf-8", errors="replace")
        # A poll started before an action must not overwrite its in-progress state.
        if self.busy:
            return
        if self.poll_generation != self.action_generation:
            self.poll_status()
            return
        if status != QProcess.ExitStatus.NormalExit or (code != 0 and "No Sockets found" not in text):
            self.set_status_unavailable(text.strip() or f"screen -ls failed (exit {code})")
            return
        running = set()
        stale = set()
        for line in text.splitlines():
            match = re.match(r"\s*\d+\.SuFDeMon-([A-Za-z0-9_-]+)\s+", line)
            if match:
                name = match.group(1)
                if re.search(r"\((?:Attached|Detached)\)", line):
                    running.add(name)
                else:
                    stale.add(name)
        self.server_states = {name: True if name in running else None if name in stale else False
                              for name in self.server_buttons}
        self.last_status_error = None
        self.update_buttons()
        self.statusBar().showMessage(
            f"{sum(value is True for value in self.server_states.values())}/{len(self.server_buttons)} local Screen sessions running"
            + (" — stale sessions require inspection with screen -ls" if stale else ""))

    def open_output(self):
        self.output_dialog.show()
        self.output_dialog.raise_()
        self.output_dialog.activateWindow()

    def run_script(self, action: str, names: list[str] | None = None):
        if self.busy:
            return
        if action not in ("start", "stop"):
            raise ValueError("Unknown server action")
        script = self.repo_dir / "scripts" / f"{action}_servers.sh"
        if not script.is_file():
            self.output.appendPlainText(f"Missing script: {script}")
            self.statusBar().showMessage(f"Missing script: {script.name}")
            self.open_output()
            return
        self.busy = True
        self.action_generation += 1
        self.update_buttons()
        self.decoder.reset()
        self.output.appendPlainText(f"\n> {script.name} {' '.join(names or [])}\n")
        self.statusBar().showMessage(f"Running {script.name}…")
        # Pass arguments separately; do not construct or evaluate a shell command.
        self.process.start("bash", [str(script), *(names or [])])

    def read_output(self):
        data = bytes(self.process.readAllStandardOutput())
        self.append_output(self.decoder.decode(data))

    def append_output(self, text: str):
        self.output.moveCursor(QTextCursor.MoveOperation.End)
        self.output.insertPlainText(text)
        self.output.ensureCursorVisible()

    def process_error(self, error: QProcess.ProcessError):
        self.output.appendPlainText(self.process.errorString())
        if error == QProcess.ProcessError.FailedToStart:
            self.busy = False
            self.update_buttons()
            self.statusBar().showMessage("Could not launch script — see Script Output")
        self.open_output()

    def process_finished(self, code: int, status: QProcess.ExitStatus):
        self.read_output()
        self.append_output(self.decoder.decode(b"", final=True))
        self.busy = False
        self.update_buttons()
        if status == QProcess.ExitStatus.NormalExit and code == 0:
            self.statusBar().showMessage("Script completed successfully — see Script Output for details")
        else:
            self.statusBar().showMessage(f"Script failed (exit {code}) — see Script Output")
            self.open_output()
        self.poll_status()

    def closeEvent(self, event: QCloseEvent):
        if self.busy:
            self.statusBar().showMessage("Wait for the current script to finish before closing.")
            event.ignore()
        else:
            # Screen owns the server processes; closing this window leaves them running.
            self.output_dialog.close()
            self.timer.stop()
            self.status_timeout.stop()
            if self.status_process.state() != QProcess.ProcessState.NotRunning:
                self.status_process.kill()
                self.status_process.waitForFinished(1000)
            event.accept()


def main():
    app = QApplication(sys.argv)
    window = ServerControlWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
