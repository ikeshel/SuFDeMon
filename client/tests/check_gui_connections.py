"""Integration test using temporary server configs and real ROOT GUI controls.
Run with a working X display (or xvfb-run).
"""
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

if not os.environ.get("DISPLAY"):
    print("SKIP: GUI integration test requires an X display")
    sys.exit(77)

repo = Path(sys.argv[1]).resolve()
build = Path(sys.argv[2]).resolve()
processes = []
reservations = []
try:
    with tempfile.TemporaryDirectory(prefix='sufdemon-gui-') as tmp:
        configs = Path(tmp)
        sources = sorted((repo / 'config/servers').glob('*.conf'))
        ports = {}
        for config in sources:
            reserve = socket.socket()
            reserve.bind(('127.0.0.1', 0))
            reservations.append(reserve)
            ports[config.stem] = reserve.getsockname()[1]
        for config, reserve in zip(sources, reservations):
            values = dict(line.split('=', 1) for line in config.read_text().splitlines()
                          if line and not line.startswith('#'))
            values['hostname'] = 'localhost'
            values['port'] = str(ports[config.stem])
            target = configs / config.name
            target.write_text(''.join(f'{key}={value}\n' for key, value in values.items()))
            reserve.close()
            process = subprocess.Popen([str(build / 'server' / ('SuFDeMon' + values['type'] + 'Server')),
                                        '--config', str(target)], stdout=subprocess.DEVNULL,
                                       stderr=subprocess.DEVNULL)
            processes.append(process)
        time.sleep(.5)
        commands = ['#include <TH1D.h>', '#include <TGButton.h>', '#include <TGTab.h>', '#include <TSuFDeMonClient.h>']
        checks = {}
        def check(label, expression, expected):
            commands.append(f'std::cout << "CHECK_{label}=" << ({expression}) << std::endl;')
            checks[label] = str(expected)
        listings = {}
        def list_check(label, instances):
            commands.extend([f'std::cout << "LIST_BEGIN_{label}" << std::endl;',
                             'ListOfHistograms();',
                             f'std::cout << "LIST_END_{label}" << std::endl;'])
            listings[label] = [
                'h' + instance + '_' + (f'FC{fc}_' if fc else '') + quantity + ('' if instance.startswith('SCIFI') else str(channel))
                for instance in instances for quantity in (('ToT', 'TDC') if instance.startswith('SCIFI') else ('ADC', 'TDC'))
                for fc in (range(1, 4) if instance.startswith('MUSIC') else [0])
                for channel in range((8 if instance in ('PLSCI4', 'PLSCI6') else 6) if instance.startswith('PLSCI') else (1 if instance.startswith('SCIFI') else 32))]
        check('startup_all', 'gSuFDeMonGui->ConnectedServerCount()', 22)
        for button, count in [(91, 0), (91, 0), (90, 22), (90, 22)]:
            commands.append(f'gSuFDeMonGui->ProcessMessage(MK_MSG(kC_COMMAND,kCM_BUTTON),{button},0);')
            check(f'global{button}_{len(checks)}', 'gSuFDeMonGui->ConnectedServerCount()', count)
        commands += ['gSuFDeMonGui->DisconnectGroup(0);', 'gSuFDeMonGui->DisconnectGroup(1);', 'gSuFDeMonGui->DisconnectGroup(2);']
        # Exercise the actual group button message dispatch.
        for button, count in [(100, 2), (102, 8), (104, 22), (104, 22)]:
            commands.append(f'gSuFDeMonGui->ProcessMessage(MK_MSG(kC_COMMAND,kCM_BUTTON),{button},0);')
            check(f'button{button}_{len(checks)}', 'gSuFDeMonGui->ConnectedServerCount()', count)
        ordered = [f'MUSIC{i}' for i in range(1,3)] + [f'PLSCI{i}' for i in range(1,7)] + [f'SCIFI{i}' for i in range(1,15)]
        list_check("all", ordered)
        commands += ['auto* promptHistogram = SuFDeMonGet("hSCIFI14_TDC");']
        check('prompt_get', 'promptHistogram ? promptHistogram->GetName() : "null"', 'hSCIFI14_TDC')
        commands += ['#include <TColor.h>']
        for detector in ('MUSIC1_FC1', 'PLSCI4'):
            for quantity, color in (('ADC', 'kRed'), ('TDC', 'kBlue')):
                name = f'h{detector}_{quantity}0'
                commands.append(f'promptHistogram = SuFDeMonGet("{name}");')
                check(name+'_fill',
                      f'promptHistogram && promptHistogram->GetFillStyle() == 1001 && '
                      f'promptHistogram->GetFillColor() == TColor::GetColorTransparent({color}, 0.35f) && '
                      f'promptHistogram->GetLineColor() == {color}', 1)
        for index, name in enumerate(ordered):
            commands += [f'gSuFDeMonGui->SelectServer({index});', 'gSuFDeMonGui->DrawSelected();']
            check(name, 'gSuFDeMonGui->GetHistogram()->GetName()', 'h'+name+('_ToT' if name.startswith('SCIFI') else ('_FC1' if name.startswith('MUSIC') else '')+'_ADC0'))
        list_check("after_selection", ordered)
        check('after_selection', 'gSuFDeMonGui->ConnectedServerCount()', 22)
        commands += [
            '#include <TGComboBox.h>', '#include <functional>', '#include <vector>',
            'auto* tabs = static_cast<TGTab*>(static_cast<TGFrameElement*>(gSuFDeMonGui->GetList()->First())->fFrame);',
            'std::vector<TGComboBox*> combos;',
            'std::function<void(TGCompositeFrame*)> collect = [&](TGCompositeFrame* f) { TIter next(f->GetList()); while (auto* item = next()) { auto* child = static_cast<TGFrameElement*>(item)->fFrame; if (auto* c = dynamic_cast<TGComboBox*>(child)) combos.push_back(c); else if (auto* nested = dynamic_cast<TGCompositeFrame*>(child)) collect(nested); } };',
        ]
        for tab, detector, count, server, channel in [(1, 'MUSIC', 2, 1, 26), (2, 'PLSCI', 6, 3, 5), (3, 'SCIFI', 14, 10, 11)]:
            commands += [f'tabs->SetTab({tab});', 'combos.clear(); collect(tabs->GetCurrentContainer());']
            check(detector+'_server_count', 'combos[0]->GetNumberOfEntries()', count)
            check(detector+'_control_count', 'combos.size()', 5 if tab == 1 else (3 if tab == 3 else 4))
            quantity_index = 2 if tab == 1 else 1
            channel_index = 3 if tab == 1 else 2
            commands += [f'combos[0]->Select({server});', f'combos[{quantity_index}]->Select(1);']
            if tab != 3: commands += [f'combos[{channel_index}]->Select({channel});']
            if tab == 1:
                commands += ['combos[1]->Select(2);']
            commands += ['gSuFDeMonGui->DrawSelected();']
            expected = ['hMUSIC2_FC2_TDC26', 'hPLSCI2_TDC5', 'hSCIFI3_TDC'][tab-1]
            check(detector+'_tab_draw', 'gSuFDeMonGui->GetHistogram()->GetName()', expected)
        for tab, name in [(1, 'hMUSIC2_FC2_TDC26'), (2, 'hPLSCI2_TDC5'), (3, 'hSCIFI3_TDC')]:
            commands += [f'tabs->SetTab({tab});', 'gSuFDeMonGui->DrawSelected();']
            check('retained_tab'+str(tab), 'gSuFDeMonGui->GetHistogram()->GetName()', name)
            quantity_index = 2 if tab == 1 else 1
            channel_index = 3 if tab == 1 else 2
            commands += ['combos.clear(); collect(tabs->GetCurrentContainer());', f'combos[{quantity_index}]->Select(0);']
            if tab != 3: commands += [f'combos[{channel_index}]->Select(0);']
        check('tabs_keep_connections', 'gSuFDeMonGui->ConnectedServerCount()', 22)
        commands += ['tabs->SetTab(2);', 'combos.clear(); collect(tabs->GetCurrentContainer());']
        for instance, channels in [(4, 8), (5, 6), (6, 8), (1, 6), (2, 6), (3, 6)]:
            commands += [f'combos[0]->Select({instance+1});']
            check(f'PLSCI{instance}_channels', 'combos[2]->GetNumberOfEntries()', channels)
            check(f'PLSCI{instance}_valid_selection', f'combos[2]->GetSelected() < {channels}', 1)
            commands += [f'combos[2]->Select({channels-1});', 'gSuFDeMonGui->DrawSelected();']
            check(f'PLSCI{instance}_last_pmt', 'gSuFDeMonGui->GetHistogram()->GetName()', f'hPLSCI{instance}_ADC{channels-1}')
        commands += ['combos[2]->Select(0);']
        commands += ['tabs->SetTab(1);', 'gSuFDeMonGui->DrawSelectedMacro();']
        check('music_macro_canvas', 'gROOT->FindObject("cMUSIC1_ADC_ALL") != nullptr', 1)
        commands += ['#include <TCanvas.h>', '#include <TPad.h>']
        # Exercise the real SCIFI button signals, including a user-closed canvas.
        commands += [
            'tabs->SetTab(3);', 'combos.clear(); collect(tabs->GetCurrentContainer());',
            'combos[0]->Select(8);',
            'std::vector<TGTextButton*> scifiButtons;',
            'std::function<void(TGCompositeFrame*)> collectButtons = [&](TGCompositeFrame* f) { TIter next(f->GetList()); while (auto* item = next()) { auto* child = static_cast<TGFrameElement*>(item)->fFrame; if (auto* b = dynamic_cast<TGTextButton*>(child)) { if (!dynamic_cast<TGCheckButton*>(b)) scifiButtons.push_back(b); } else if (auto* nested = dynamic_cast<TGCompositeFrame*>(child)) collectButtons(nested); } };',
            'collectButtons(tabs->GetCurrentContainer());',
        ]
        for quantity_index, quantity in enumerate(('ToT', 'TDC')):
            commands += [f'combos[1]->Select({quantity_index});',
                         'static_cast<TCanvas*>(gROOT->FindObject("SuFDeMonCanvas"))->Close();',
                         'scifiButtons[0]->Clicked();']
            check('scifi_reopen_'+quantity,
                  '[]() { auto* c = static_cast<TCanvas*>(gROOT->GetListOfCanvases()->FindObject("SuFDeMonCanvas")); '
                  f'return c && c->GetCanvasImp() && c->FindObject("hSCIFI1_{quantity}"); }}()', 1)
            commands += ['gSystem->Sleep(200);', 'scifiButtons[0]->Clicked();',
                         f'double beforeClear{quantity} = gSuFDeMonGui->GetHistogram()->GetEntries();',
                         'static_cast<TCanvas*>(gROOT->FindObject("SuFDeMonCanvas"))->Close();',
                         'scifiButtons[1]->Clicked();']
            check('scifi_clear_'+quantity,
                  f'std::string(gSuFDeMonGui->GetHistogram()->GetName()) == "hSCIFI1_{quantity}" && '
                  f'gSuFDeMonGui->GetHistogram()->GetEntries() < beforeClear{quantity}', 1)
            check('scifi_clear_reopen_'+quantity,
                  'gROOT->GetListOfCanvases()->FindObject("SuFDeMonCanvas") != nullptr', 1)
        for tab, detector, instances in [(2, 'PLSCI', 6)]:
            commands += [f'tabs->SetTab({tab});', 'combos.clear(); collect(tabs->GetCurrentContainer());']
            check(detector+'_macro_count', 'combos.back()->GetNumberOfEntries()', instances * 2)
            for instance in range(1, instances + 1):
                channels = (8 if instance in (4, 6) else 6) if detector == 'PLSCI' else 32
                for quantity_index, quantity in enumerate(('ADC', 'TDC')):
                    macro_id = (instance - 1) * 2 + quantity_index
                    canvas = f'c{detector}{instance}_{quantity}_ALL'
                    commands += [f'combos.back()->Select({macro_id});', 'gSuFDeMonGui->DrawSelectedMacro();']
                    check(canvas+'_pads', f'static_cast<TCanvas*>(gROOT->FindObject("{canvas}"))->GetListOfPrimitives()->GetSize()', channels)
                    prefix = f'h{detector}{instance}_{quantity}'
                    check(canvas+'_channels',
                          f'[]() {{ auto* c = static_cast<TCanvas*>(gROOT->FindObject("{canvas}")); '
                          f'int found = 0; for (int ch = 0; ch < {channels}; ++ch) '
                          f'if (c->GetPad(ch+1)->FindObject((std::string("{prefix}") + std::to_string(ch)).c_str())) ++found; '
                          f'return found; }}()', channels)
                    commands += [f'delete gROOT->FindObject("{canvas}");']

        commands += ['tabs->SetTab(3);', 'combos.clear(); collect(tabs->GetCurrentContainer());']
        check('SCIFI_macro_count', 'combos.back()->GetNumberOfEntries()', 14)
        for instance in range(1, 15):
            canvas = f'cSCIFI{instance}_ALL'
            commands += [f'combos.back()->Select({instance-1});', 'gSuFDeMonGui->DrawSelectedMacro();']
            check(canvas+'_pads', f'static_cast<TCanvas*>(gROOT->FindObject("{canvas}"))->GetListOfPrimitives()->GetSize()', 2)
            for pad, quantity in enumerate(('ToT', 'TDC'), 1):
                name = f'hSCIFI{instance}_{quantity}'
                check(name+'_map',
                      f'[]() {{ auto* c = static_cast<TCanvas*>(gROOT->FindObject("{canvas}")); '
                      f'auto* h = dynamic_cast<TH1*>(c->GetPad({pad})->FindObject("{name}")); '
                      f'return h && h->GetDimension() == 2 && h->GetNbinsX() == 2048 && '
                      f'std::string(h->GetOption()) == "COLZ"; }}()', 1)
            commands += [f'delete gROOT->FindObject("{canvas}");']
        # Disconnect just MUSIC; verify PLSCI and SCIFI still serve histograms.
        commands.append('gSuFDeMonGui->ProcessMessage(MK_MSG(kC_COMMAND,kCM_BUTTON),101,0);')
        list_check("music_off", ordered[2:])
        check('music_off', 'gSuFDeMonGui->ConnectedServerCount()', 20)
        commands += ['gSuFDeMonGui->SelectServer(2);', 'gSuFDeMonGui->DrawSelected();']
        check('plsci_retained', 'gSuFDeMonGui->GetHistogram()->GetName()', 'hPLSCI1_ADC0')
        # Individual toggle and group reconnect remain isolated.
        commands.append('gSuFDeMonGui->ProcessMessage(MK_MSG(kC_COMMAND,kCM_BUTTON),1002,0);')
        check('individual_off', 'gSuFDeMonGui->ConnectedServerCount()', 19)
        commands.append('gSuFDeMonGui->ConnectGroup(1);')
        check('plsci_reconnect', 'gSuFDeMonGui->ConnectedServerCount()', 20)
        commands += ['gSuFDeMonGui->DisconnectGroup(1);', 'gSuFDeMonGui->DisconnectGroup(2);']
        list_check("none", [])
        check('all_off', 'gSuFDeMonGui->ConnectedServerCount()', 0)
        commands += ['gSuFDeMonGui->ConnectGroup(0);', 'gSuFDeMonGui->ConnectGroup(1);', 'gSuFDeMonGui->ConnectGroup(2);']
        check('all_reconnected', 'gSuFDeMonGui->ConnectedServerCount()', 22)
        commands += ['gSuFDeMonGui->SelectServer(21);', 'gSuFDeMonGui->GetClient()->ShutdownServer();', 'gSuFDeMonGui->CheckConnection();']
        list_check("server_closed", ordered[:-1])
        check('server_closed', 'gSuFDeMonGui->ConnectedServerCount()', 21)
        commands += ['gSuFDeMonGui->ConnectGroup(2);']
        check('unavailable_isolated', 'gSuFDeMonGui->ConnectedServerCount()', 21)
        commands += ['gSuFDeMonGui->ProcessMessage(MK_MSG(kC_COMMAND,kCM_BUTTON),91,0);']
        check('global_partial_off', 'gSuFDeMonGui->ConnectedServerCount()', 0)
        commands += ['gSuFDeMonGui->ProcessMessage(MK_MSG(kC_COMMAND,kCM_BUTTON),90,0);']
        check('global_partial_on', 'gSuFDeMonGui->ConnectedServerCount()', 21)
        commands += ['gSuFDeMonGui->CloseWindow();', 'ListOfHistograms();', '.q']
        result = subprocess.run([str(build/'client/SuFDeMonGui'), 'localhost', str(ports['MUSIC1'])],
                                input='\n'.join(commands)+'\n', text=True, capture_output=True,
                                env=dict(os.environ, CONFIG_DIR=str(configs)), cwd=repo, timeout=60)
        assert result.returncode == 0, result.stdout + result.stderr
        for label, expected in listings.items():
            section = result.stdout.split(f'LIST_BEGIN_{label}\n', 1)[1].split(f'LIST_END_{label}\n', 1)[0]
            actual = section.splitlines()
            assert actual == (expected or ['No connected servers.']), (label, actual)
        for label, expected in checks.items():
            assert f'CHECK_{label}={expected}\n' in result.stdout, (label, result.stdout, result.stderr)
        assert 'error:' not in result.stderr and 'histogram not found' not in result.stderr, result.stderr
        assert 'No connected servers.' in result.stdout
        assert all(p.poll() is None for config, p in zip(sources, processes) if config.stem != 'SCIFI14'), 'Disconnect stopped a server'
        # Startup tolerates an unavailable configured server and connects the rest.
        result = subprocess.run([str(build/'client/SuFDeMonGui'), 'localhost', str(ports['SCIFI14'])],
                                input='std::cout << "STARTUP_PARTIAL=" << gSuFDeMonGui->ConnectedServerCount() << std::endl;\ngSuFDeMonGui->CloseWindow();\n.q\n',
                                text=True, capture_output=True,
                                env=dict(os.environ, CONFIG_DIR=str(configs)), cwd=repo, timeout=30)
        assert result.returncode == 0 and 'STARTUP_PARTIAL=21' in result.stdout, result.stdout + result.stderr
        # A fresh client can use a server after the GUI releases its sockets.
        result = subprocess.run([str(build/'client/SuFDeMonClient'), 'localhost', str(ports['SCIFI13'])],
                                input='ListOfHistograms();\nstd::cout << "PING=" << SuFDeMonPing() << std::endl;\n.q\n',
                                text=True, capture_output=True, timeout=10)
        assert 'hSCIFI13_TDC' in result.stdout
        assert 'PING=1' in result.stdout, result.stdout + result.stderr
        print('PASS: 22 simultaneous GUI connections, group isolation, individual toggle, drawing, reconnect, and cleanup')
finally:
    for s in reservations:
        s.close()
    for p in processes:
        if p.poll() is None:
            p.terminate()
            try:
                p.wait(timeout=3)
            except subprocess.TimeoutExpired:
                p.kill()
                p.wait()
