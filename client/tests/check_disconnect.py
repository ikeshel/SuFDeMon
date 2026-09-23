# Author: Irakli Keshelashvili, 2026
# SuFDeMon - Super-FRS Detector Monitoring Software. GPL-3.0; see LICENSE.
"""Check idle server exit and transport closure while waiting for ROOT replies."""
import socket
import subprocess
import sys
import threading

server_binary, probe = sys.argv[1:]
with socket.socket() as reservation:
    reservation.bind(('127.0.0.1', 0))
    port = reservation.getsockname()[1]
server = subprocess.Popen([server_binary, '--port', str(port)], stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True)
client = None
try:
    # The server reports readiness only after its listening socket is valid.
    while True:
        line = server.stdout.readline()
        if not line:
            raise RuntimeError('Server exited before listening')
        if 'listening on' in line:
            break
    client = subprocess.Popen([probe, str(port), 'idle'], stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    assert client.stdout.readline().strip() == 'READY'
    server.terminate()
    server.wait(timeout=5)
    out, err = client.communicate('\n', timeout=5)
    assert client.returncode == 0 and 'DISCONNECTED' in out, (out, err)
finally:
    for process in (client, server):
        if process is not None and process.poll() is None:
            process.kill()
            process.wait()

# A peer accepts a command and closes before replying: exercise receive failures.
for mode in ('ping', 'list', 'get', 'clear'):
    with socket.socket() as listener:
        listener.bind(('127.0.0.1', 0))
        listener.listen()
        listener.settimeout(5)
        port = listener.getsockname()[1]
        errors = []
        def close_after_request():
            try:
                conn, _ = listener.accept()
                with conn:
                    conn.settimeout(5)
                    assert conn.recv(4096), 'No ROOT request received'
            except Exception as error:
                errors.append(error)
        peer = threading.Thread(target=close_after_request)
        peer.start()
        try:
            result = subprocess.run([probe, str(port), mode], capture_output=True,
                                    text=True, timeout=8)
            assert result.returncode == 0, (mode, result.stdout, result.stderr)
        finally:
            peer.join(timeout=6)
        assert not peer.is_alive() and not errors, errors
print('PASS: idle server exit and dropped PING/LIST/GET/CLEAR replies clear client state')
