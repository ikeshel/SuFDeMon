"""Exercise all configured instances concurrently using ROOT protocol probes."""
import pathlib
import socket
import subprocess
import sys
import tempfile

build, source, probe = map(pathlib.Path, sys.argv[1:])
processes = []
reservations = []
try:
    # Reserve distinct local ports, releasing each just before its server starts.
    configs = sorted((source / 'config/servers').glob('*.conf'))
    assert len(configs) == 19
    for _ in configs:
        reservation = socket.socket()
        reservation.bind(('127.0.0.1', 0))
        reservations.append(reservation)
    with tempfile.TemporaryDirectory() as temporary:
        jobs = []
        for config, reservation in zip(configs, reservations):
            values = dict(line.split('=', 1) for line in config.read_text().splitlines()
                          if line and not line.startswith('#'))
            kind, instance = values['type'], values['instance']
            port = reservation.getsockname()[1]
            reservation.close()
            executable = build / ('SuFDeMon' + kind + 'Server')
            # Put overrides before --config to verify order-independent precedence.
            process = subprocess.Popen([str(executable), '--port', str(port),
                                        '--hostname', 'localhost', '--config', str(config)],
                                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
            processes.append(process)
            jobs.append((process, port, kind, instance))
        for process, port, kind, instance in jobs:
            result = subprocess.run([str(probe), str(port), kind, instance],
                                    capture_output=True, text=True, timeout=15)
            assert result.returncode == 0, (instance, result.stdout, result.stderr)
            assert process.wait(timeout=5) == 0, (instance, process.stderr.read())
        generic = build / 'SuFDeMonServer'
        for args in [['--port', '0'], ['--port', '65536'], ['10001junk'],
                     ['--type', 'OTHER'], ['--port'], ['--unknown'],
                     ['--instance', 'bad name'], ['--config', '/nonexistent/config']]:
            assert subprocess.run([str(generic), *args], capture_output=True, timeout=5).returncode != 0, args
        wrong_type = subprocess.run([str(build / 'SuFDeMonMUSICServer'), '--config',
                                     str(source / 'config/servers/SCIFI1.conf')],
                                    capture_output=True, timeout=5)
        assert wrong_type.returncode != 0
        for content in ['type=MUSIC\n', 'type=OTHER\ninstance=X\nhostname=localhost\nport=1\n',
                        'type=MUSIC\ninstance=X\nhostname=localhost\nport=1\nport=2\n',
                        'type=MUSIC\ninstance=X\nhostname=localhost\nport=1\nunknown=x\n']:
            config = pathlib.Path(temporary) / 'invalid.conf'
            config.write_text(content)
            assert subprocess.run([str(generic), '--config', str(config)],
                                  capture_output=True, timeout=5).returncode != 0
    print('PASS: 19 instances, ROOT protocol, histogram transfer, shutdown and invalid configs')
finally:
    for reservation in reservations:
        reservation.close()
    for process in processes:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
