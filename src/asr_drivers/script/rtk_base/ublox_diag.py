#!/usr/bin/env python3
"""u-blox RTK base diagnostic.

A standalone health check for the GCS u-blox receiver that rtcm_reader drives.
It does NOT need ROS running — talk straight to the serial port:

    ros2 run asr_drivers ublox_diag.py            # auto-detect the u-blox
    ros2 run asr_drivers ublox_diag.py /dev/ttyACM0 115200
    python3 ublox_diag.py /dev/ttyACM0            # without ROS

It reports, in order:
  1. Which receiver this is (MON-VER: model / firmware / protocol).
  2. Whether each TMODE survey-in config key is ACK'd or NAK'd (per-key bisect).
  3. Whether a full survey-in command starts (NAV-SVIN active=1, dur climbing).
  4. Whether RTCM3 frames actually come out of the port.

The key-ID patch below mirrors rtcm_reader.py — see that file (and the
pyubx2-tmode-key-id-bug note) for why it is needed.
"""
import argparse
import glob
import os
import sys
import time

from serial import Serial
from pyubx2 import UBXMessage, UBXReader, POLL, SET
from pyubx2.ubxtypes_configdb import UBX_CONFIG_DATABASE as _UBX_CFG_DB

# pyubx2 1.0.0 ships these two U4 TMODE keys with a 1-byte (0x2…) storage prefix,
# so the receiver NAKs them. Force the correct 4-byte (0x4…) IDs. Same patch as
# rtcm_reader.py, kept local so this tool stands alone.
for _name, _correct_id in (
    ('CFG_TMODE_SVIN_MIN_DUR', 0x40030010),
    ('CFG_TMODE_SVIN_ACC_LIMIT', 0x40030011),
):
    _entry = _UBX_CFG_DB.get(_name)
    if _entry and _entry[0] != _correct_id:
        _UBX_CFG_DB[_name] = (_correct_id, _entry[1])

GREEN, RED, YELLOW, RESET = '\033[32m', '\033[31m', '\033[33m', '\033[0m'


def resolve_port(port):
    if port and port != 'auto':
        return port
    for path in sorted(glob.glob('/dev/serial/by-id/*')):
        name = os.path.basename(path).lower()
        if 'u-blox' in name or 'gnss' in name:
            return path
    raise SystemExit('No u-blox GNSS receiver found under /dev/serial/by-id/ — is it plugged in?')


def ok(msg):   print(f'  {GREEN}PASS{RESET} {msg}')
def bad(msg):  print(f'  {RED}FAIL{RESET} {msg}')
def warn(msg): print(f'  {YELLOW}WARN{RESET} {msg}')


class Diag:
    def __init__(self, ser):
        self.ser = ser
        self.reader = UBXReader(ser)

    def _read_for(self, seconds, on_msg):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            try:
                _, msg = self.reader.read()
            except Exception:
                continue
            if msg is not None:
                on_msg(msg)

    def send_ack(self, msg, timeout=2.0):
        """Write msg, return True on ACK-ACK / False on ACK-NAK / None on timeout."""
        self.ser.reset_input_buffer()
        self.ser.write(msg.serialize())
        result = [None]

        def handle(m):
            if m.identity == 'ACK-ACK':
                result[0] = True
            elif m.identity == 'ACK-NAK':
                result[0] = False
        self._read_for(timeout, lambda m: handle(m) if result[0] is None else None)
        return result[0]

    # --- 1. identity ---------------------------------------------------
    def check_version(self):
        print('1. Receiver identity (MON-VER)')
        info = {}

        def grab(m):
            if m.identity != 'MON-VER':
                return
            for attr in dir(m):
                if attr.startswith('extension') or attr == 'swVersion' or attr == 'hwVersion':
                    val = getattr(m, attr)
                    text = val.decode(errors='ignore').strip('\x00 ') if isinstance(val, bytes) else str(val)
                    for tag in ('MOD=', 'FWVER=', 'PROTVER='):
                        if text.startswith(tag):
                            info[tag.rstrip('=')] = text[len(tag):]
                    if attr == 'swVersion':
                        info['sw'] = text
        self.ser.reset_input_buffer()
        self.ser.write(UBXMessage('MON', 'MON-VER', POLL).serialize())
        self._read_for(2.0, grab)

        if not info:
            bad('no MON-VER reply — receiver not responding to UBX')
            return None
        model = info.get('MOD', '(unknown)')
        ok(f"model={model}  fw={info.get('FWVER','?')}  protver={info.get('PROTVER','?')}")
        if model.startswith('ZED-F9') or 'HPG' in info.get('FWVER', ''):
            ok('RTK base capable (survey-in + RTCM supported)')
        else:
            warn(f'model "{model}" may not support survey-in / RTCM base mode')
        return model

    # --- 2. per-key ACK bisect ----------------------------------------
    def check_keys(self):
        print('2. TMODE config keys (ACK/NAK per key)')
        keys = [
            ('CFG_TMODE_MODE', 0),
            ('CFG_TMODE_POS_TYPE', 0),
            ('CFG_TMODE_SVIN_ACC_LIMIT', 250000),
            ('CFG_TMODE_SVIN_MIN_DUR', 60),
        ]
        all_ok = True
        for name, val in keys:
            res = self.send_ack(UBXMessage.config_set(layers=1, transaction=0, cfgData=[(name, val)]))
            if res is True:
                ok(f'{name}')
            elif res is False:
                bad(f'{name} -> ACK-NAK (key ID likely wrong)')
                all_ok = False
            else:
                warn(f'{name} -> no ACK')
                all_ok = False
        return all_ok

    # --- 3. survey-in starts ------------------------------------------
    def check_survey(self, acc_mm=25000, min_dur=60):
        print('3. Survey-in start')
        self.send_ack(UBXMessage.config_set(layers=1, transaction=0, cfgData=[('CFG_TMODE_MODE', 0)]))
        res = self.send_ack(UBXMessage.config_set(layers=1, transaction=0, cfgData=[
            ('CFG_TMODE_MODE', 1),
            ('CFG_TMODE_SVIN_ACC_LIMIT', acc_mm * 10),
            ('CFG_TMODE_SVIN_MIN_DUR', min_dur),
        ]))
        if res is False:
            bad('survey-in config NAK\'d — survey will never start')
            return False
        if res is None:
            warn('survey-in config got no ACK')

        self.ser.write(UBXMessage('CFG', 'CFG-MSG', SET, msgClass=0x01, msgID=0x3B, rateUSB=1).serialize())
        state = {'active': 0, 'dur': 0, 'acc': 0.0}

        def grab(m):
            if m.identity == 'NAV-SVIN':
                state.update(active=m.active, dur=m.dur, acc=m.meanAcc / 10000.0)
        print('   watching NAV-SVIN for 6 s ...')
        self._read_for(6.0, grab)
        if state['active'] == 1 or state['dur'] > 0:
            ok(f"survey running: active={state['active']} dur={state['dur']}s acc={state['acc']:.2f}m")
            return True
        bad(f"survey NOT running (active={state['active']} dur={state['dur']} acc={state['acc']:.0f}m frozen)")
        return False

    # --- 4. RTCM output -----------------------------------------------
    def check_rtcm(self):
        print('4. RTCM3 output')
        for cls, mid in [(0xF5, 0x05), (0xF5, 0x4A), (0xF5, 0x54), (0xF5, 0x5E), (0xF5, 0x7B)]:
            self.ser.write(UBXMessage('CFG', 'CFG-MSG', SET, msgClass=cls, msgID=mid, rateUSB=1).serialize())
            time.sleep(0.05)
        # Count RTCM3 preambles (0xD3) seen on the raw stream over 4 s.
        self.ser.reset_input_buffer()
        count = [0]
        end = time.monotonic() + 4.0
        while time.monotonic() < end:
            b = self.ser.read(256)
            count[0] += b.count(0xD3)
        if count[0] > 0:
            ok(f'~{count[0]} RTCM3 frame markers in 4 s (base station only emits once survey-in is valid)')
        else:
            warn('no RTCM3 frames yet — expected until survey-in completes')


def main():
    ap = argparse.ArgumentParser(description='u-blox RTK base diagnostic')
    ap.add_argument('port', nargs='?', default='auto', help='serial device or "auto" (default: auto)')
    ap.add_argument('baud', nargs='?', type=int, default=115200)
    args = ap.parse_args()

    port = resolve_port(args.port)
    print(f'=== u-blox diagnostic on {port} @ {args.baud} ===\n')
    with Serial(port, args.baud, timeout=1) as ser:
        diag = Diag(ser)
        diag.check_version()
        print()
        diag.check_keys()
        print()
        diag.check_survey()
        print()
        diag.check_rtcm()
    print('\nDone.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
