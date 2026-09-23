# Author: Irakli Keshelashvili, 2026
# SuFDeMon - Super-FRS Detector Monitoring Software. GPL-3.0; see LICENSE.
"""Exercise the real GUI and shell scripts with an isolated Screen substitute."""
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from PyQt6.QtCore import QProcess
from PyQt6.QtTest import QTest
from PyQt6.QtWidgets import QApplication
from server_control import ServerControlWindow

REPO = Path(__file__).resolve().parents[2]
SCREEN = '''#!/usr/bin/env python3
import json, os, sys
from pathlib import Path
p=Path(os.environ['TEST_SCREEN_STATE'])
sessions=json.loads(p.read_text())
args=sys.argv[1:]
if args == ['-ls']:
    if p.with_suffix('.error').exists():
        print('Cannot access Screen directory', file=sys.stderr)
        sys.exit(1)
    for name in sessions:
        print('  123.' + name + ' (Detached)')
    if not sessions:
        print('No Sockets found in test directory.')
        sys.exit(1)
elif args[0] == '-dmS':
    if p.with_suffix('.fail').exists():
        print('Simulated start failure', file=sys.stderr)
        sys.exit(1)
    sessions.append(args[1]); p.write_text(json.dumps(sessions))
elif args[0] == '-S':
    sessions.remove(args[1].split('.', 1)[1]); p.write_text(json.dumps(sessions))
else:
    sys.exit(2)
'''


class ControlTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="sufdemon-control-")
        self.repo = Path(self.temp.name)
        shutil.copytree(REPO / 'scripts', self.repo / 'scripts')
        shutil.copytree(REPO / 'config', self.repo / 'config')
        binaries = self.repo / 'build/server'
        binaries.mkdir(parents=True)
        for detector in ('MUSIC', 'PLSCI', 'SCIFI'):
            binary = binaries / f'SuFDeMon{detector}Server'
            binary.write_text('#!/bin/sh\nexit 0\n')
            binary.chmod(0o755)
        fakebin = self.repo / 'bin'
        fakebin.mkdir()
        screen = fakebin / 'screen'
        screen.write_text(SCREEN)
        screen.chmod(0o755)
        self.state = self.repo / 'state.json'
        self.state.write_text('[]')
        environment = dict(os.environ, PATH=str(fakebin)+os.pathsep+os.environ['PATH'],
                           TEST_SCREEN_STATE=str(self.state))
        for key in ('BUILD_DIR', 'CONFIG_DIR', 'LOG_DIR'):
            environment.pop(key, None)
        self.environment = patch.dict(os.environ, environment, clear=True)
        self.environment.start()
        self.window = ServerControlWindow(self.repo)
        self.window.show()
        self.wait(lambda: all(state is False for state in self.window.server_states.values()))

    def tearDown(self):
        self.wait(lambda: not self.window.busy)
        self.window.close()
        self.window.deleteLater()
        self.app.processEvents()
        self.environment.stop()
        self.temp.cleanup()

    def wait(self, predicate, timeout=15):
        end = time.monotonic()+timeout
        while time.monotonic() < end:
            self.app.processEvents()
            if predicate():
                return
            QTest.qWait(10)
        self.fail('Timed out: '+self.window.output.toPlainText())

    def test_individual_toggle_and_layout(self):
        w = self.window
        self.assertEqual([w.column_layouts[k].count()-1 for k in ('MUSIC','PLSCI','SCIFI')], [2,3,14])
        self.assertEqual(list(w.server_buttons)[-5:], ['SCIFI10','SCIFI11','SCIFI12','SCIFI13','SCIFI14'])
        w.server_buttons['MUSIC1'].click()
        self.assertTrue(w.busy)
        self.assertFalse(w.start_all.isEnabled())
        self.assertFalse(w.close())
        self.wait(lambda: not w.busy and w.server_states['MUSIC1'] is True)
        self.assertEqual(json.loads(self.state.read_text()), ['SuFDeMon-MUSIC1'])
        self.assertIn('Running', w.server_buttons['MUSIC1'].text())
        w.server_buttons['MUSIC1'].click()
        self.wait(lambda: not w.busy and w.server_states['MUSIC1'] is False)
        self.assertEqual(json.loads(self.state.read_text()), [])

    def test_all_and_external_changes(self):
        w = self.window
        w.start_all.click()
        self.wait(lambda: not w.busy and all(w.server_states.values()))
        self.assertEqual(len(json.loads(self.state.read_text())), 19)
        sessions=json.loads(self.state.read_text());sessions.remove('SuFDeMon-SCIFI14')
        self.state.write_text(json.dumps(sessions))
        w.poll_status()
        self.wait(lambda: w.server_states['SCIFI14'] is False)
        self.assertTrue(w.server_states['SCIFI1'])
        w.stop_all.click()
        self.wait(lambda: not w.busy and all(s is False for s in w.server_states.values()))
        self.assertEqual(json.loads(self.state.read_text()), [])

    def test_script_failure_and_unavailable_status(self):
        w = self.window
        self.state.with_suffix('.fail').touch()
        w.server_buttons['PLSCI2'].click()
        self.wait(lambda: not w.busy)
        self.assertIn('Simulated start failure', w.output.toPlainText())
        self.assertTrue(w.output_dialog.isVisible())
        self.assertFalse(w.server_states['PLSCI2'])
        self.state.with_suffix('.error').touch()
        self.wait(lambda: w.status_process.state() == QProcess.ProcessState.NotRunning)
        w.poll_status()
        self.wait(lambda: all(s is None for s in w.server_states.values()))
        self.assertFalse(w.server_buttons['PLSCI2'].isEnabled())
        self.state.with_suffix('.error').unlink()
        w.poll_status()
        self.wait(lambda: all(s is False for s in w.server_states.values()))


if __name__ == '__main__':
    unittest.main()
