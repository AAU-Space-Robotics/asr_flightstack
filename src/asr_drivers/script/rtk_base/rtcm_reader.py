#!/usr/bin/env python3
"""
RTCM Reader node — runs on the GCS machine.

Configures a u-blox receiver as an RTK base station (survey-in), then
streams raw RTCM3 correction messages to /rtcm for comms_gcs to forward.

Parameters (set via asr_gcs_params.yaml):
  port                 Serial device, or "auto" to find the u-blox by its
                       /dev/serial/by-id name (default: auto)
  baudrate             Baud rate      (default: 115200)
  configure_receiver   Run u-blox config + survey-in on startup (default: true)
  survey_min_duration  Survey-in minimum duration in seconds    (default: 120)
  survey_accuracy_mm   Survey-in accuracy limit in 0.1 mm units (default: 500000)
  measurement_rate_hz  RTCM observation/solution rate in Hz     (default: 1.0)
"""

import glob
import os
import threading
import time

import rclpy
from rclpy.node import Node
from std_msgs.msg import UInt8MultiArray

from serial import Serial
from pyrtcm import RTCMReader
from pyubx2 import UBXMessage, UBXReader, SET
from pyubx2.ubxtypes_configdb import UBX_CONFIG_DATABASE as _UBX_CFG_DB


# pyubx2 1.0.0 ships these two TMODE survey-in keys with a 1-byte (0x2…) storage
# prefix even though they are U4, so the receiver NAKs every set/get of them and
# survey-in never starts. Force the correct 4-byte (0x4…) key IDs. Guarded so a
# fixed future pyubx2 release is left untouched.
for _name, _correct_id in (
    ('CFG_TMODE_SVIN_MIN_DUR', 0x40030010),
    ('CFG_TMODE_SVIN_ACC_LIMIT', 0x40030011),
):
    _entry = _UBX_CFG_DB.get(_name)
    if _entry and _entry[0] != _correct_id:
        _UBX_CFG_DB[_name] = (_correct_id, _entry[1])


def resolve_port(port):
    """Resolve port='auto' to the u-blox GNSS receiver's stable by-id path.

    Matching on device identity (not enumeration order) keeps this from ever
    colliding with the telemetry radio that comms_gcs opens.
    """
    if port != 'auto':
        return port
    for path in sorted(glob.glob('/dev/serial/by-id/*')):
        name = os.path.basename(path).lower()
        if 'u-blox' in name or 'gnss' in name:
            return path
    raise RuntimeError(
        'No u-blox GNSS receiver found under /dev/serial/by-id/ — is it plugged in?'
    )


NMEA_IDS = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x08]

# (msgClass, msgID, RTCM type, rate divisor in measurement epochs).
# Observations go out every epoch; the static base position (1005) and GLONASS
# code-phase biases (1230) only change slowly, so a larger divisor sends them
# every few seconds and keeps the telemetry link light.
RTCM_MSGS = [
    (0xF5, 0x05, 1005, 5),  # base station position      — static, every 5th epoch
    (0xF5, 0x4A, 1074, 1),  # GPS MSM4                    — every epoch
    (0xF5, 0x54, 1084, 1),  # GLONASS MSM4                — every epoch
    (0xF5, 0x5E, 1094, 1),  # Galileo MSM4                — every epoch
    (0xF5, 0x7B, 1230, 5),  # GLONASS code-phase biases   — static, every 5th epoch
]


class RtcmReaderNode(Node):
    def __init__(self):
        super().__init__('rtcm_reader')

        self.declare_parameter('port', 'auto')
        self.declare_parameter('baudrate', 115200)
        self.declare_parameter('configure_receiver', True)
        self.declare_parameter('survey_min_duration', 120)
        self.declare_parameter('survey_accuracy_mm', 500000)
        self.declare_parameter('measurement_rate_hz', 1.0)

        port = resolve_port(self.get_parameter('port').value)
        baudrate = self.get_parameter('baudrate').value
        self._configure = self.get_parameter('configure_receiver').value
        self._survey_min_dur = self.get_parameter('survey_min_duration').value
        self._survey_acc_limit = self.get_parameter('survey_accuracy_mm').value
        self._meas_rate_hz = float(self.get_parameter('measurement_rate_hz').value)
        if self._meas_rate_hz <= 0:
            self.get_logger().warn('measurement_rate_hz must be > 0 — using 1.0 Hz')
            self._meas_rate_hz = 1.0

        self._rtcm_pub = self.create_publisher(UInt8MultiArray, 'rtcm', 10)

        self.get_logger().info(f'Opening serial port {port} @ {baudrate} baud')
        self._serial = Serial(port, baudrate, timeout=1)

        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    # ------------------------------------------------------------------
    # Main thread: configure → survey-in → stream
    # ------------------------------------------------------------------

    def _run(self):
        try:
            if self._configure:
                self._configure_receiver()
                self._wait_for_survey()
            self._stream_rtcm()
        except Exception as e:
            self.get_logger().error(f'rtcm_reader thread crashed: {e}')

    # ------------------------------------------------------------------
    # Step 1 — Configure u-blox receiver
    # ------------------------------------------------------------------

    def _configure_receiver(self):
        self.get_logger().info('Configuring u-blox receiver...')

        # Disable NMEA output so only UBX/RTCM come through
        for mid in NMEA_IDS:
            msg = UBXMessage('CFG', 'CFG-MSG', SET,
                             msgClass=0xF0, msgID=mid, rateUSB=0)
            self._serial.write(msg.serialize())
            time.sleep(0.05)

        # Set the measurement/solution rate. The base only needs to emit
        # observations as fast as the rover consumes them — 1 Hz is the RTK
        # standard and keeps the shared telemetry link light. Nothing else sets
        # this, so without it the rate is whatever is persisted in flash.
        meas_ms = int(round(1000.0 / self._meas_rate_hz))
        rate_cfg = UBXMessage.config_set(layers=1, transaction=0, cfgData=[
            ('CFG_RATE_MEAS', meas_ms),
            ('CFG_RATE_NAV', 1),
        ])
        self._serial.reset_input_buffer()
        self._serial.write(rate_cfg.serialize())
        if self._await_ack() is False:
            self.get_logger().warn(
                f'Measurement-rate config ({self._meas_rate_hz:g} Hz) REJECTED '
                '(ACK-NAK) — receiver keeps its previous rate'
            )
        else:
            self.get_logger().info(f'Measurement rate set to {self._meas_rate_hz:g} Hz')

        # Reset time mode first so the new acc_limit applies to a fresh survey-in
        reset = UBXMessage.config_set(layers=1, transaction=0, cfgData=[('CFG_TMODE_MODE', 0)])
        self._serial.write(reset.serialize())
        time.sleep(0.1)

        # Start survey-in via CFG-VALSET (modern u-blox API), BEFORE enabling the
        # RTCM stream — otherwise RTCM frames drown the ACK we check for below.
        # CFG_TMODE_SVIN_ACC_LIMIT is in 0.1 mm units; parameter is in mm
        acc_limit = int(round(self._survey_acc_limit * 10))
        cfg_data = [
            ('CFG_TMODE_MODE', 1),
            ('CFG_TMODE_SVIN_ACC_LIMIT', acc_limit),
            ('CFG_TMODE_SVIN_MIN_DUR', self._survey_min_dur),
        ]
        ubx = UBXMessage.config_set(layers=1, transaction=0, cfgData=cfg_data)
        # Clear pending bytes so the next ACK we read belongs to this VALSET.
        self._serial.reset_input_buffer()
        self._serial.write(ubx.serialize())

        ack = self._await_ack()
        if ack is False:
            self.get_logger().error(
                'Survey-in config REJECTED by receiver (ACK-NAK) — base will NOT '
                'stream RTCM. Check pyubx2 key IDs / receiver model.'
            )
        elif ack is None:
            self.get_logger().warn('No ACK seen for survey-in config — proceeding anyway')

        # Enable NAV-SVIN output via CFG-MSG (works across all u-blox generations)
        msg = UBXMessage('CFG', 'CFG-MSG', SET, msgClass=0x01, msgID=0x3B, rateUSB=1)
        self._serial.write(msg.serialize())
        time.sleep(0.05)

        # Enable RTCM3 output messages (last — these flood the port). Static
        # messages use a larger divisor (see RTCM_MSGS) so they cost less link.
        for cls, mid, _num, divisor in RTCM_MSGS:
            msg = UBXMessage('CFG', 'CFG-MSG', SET,
                             msgClass=cls, msgID=mid, rateUSB=divisor)
            self._serial.write(msg.serialize())
            time.sleep(0.05)

        self.get_logger().info(
            f'Survey-in started — min {self._survey_min_dur}s, '
            f'acc limit {self._survey_acc_limit / 1000:.1f} m'
        )

    def _await_ack(self, timeout=2.0):
        """Read serial after a config write and return True on ACK-ACK, False on
        ACK-NAK, or None if neither arrives within `timeout`. Assumes the input
        buffer was cleared before the write, so the first ACK seen is ours."""
        reader = UBXReader(self._serial)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                _, msg = reader.read()
            except Exception:
                continue
            if msg is None:
                continue
            if msg.identity == 'ACK-ACK':
                return True
            if msg.identity == 'ACK-NAK':
                return False
        return None

    # ------------------------------------------------------------------
    # Step 2 — Block until survey-in reports valid
    # ------------------------------------------------------------------

    def _wait_for_survey(self):
        self.get_logger().info('Waiting for survey-in to complete...')
        reader = UBXReader(self._serial)
        consecutive_errors = 0
        last_log = (None, None, None)  # (active, acc_rounded, dur_rounded)
        last_log_time = 0.0
        HEARTBEAT_S = 15.0  # log at least this often so a stalled survey is visible

        while rclpy.ok():
            try:
                _, msg = reader.read()
                consecutive_errors = 0
            except Exception as e:
                consecutive_errors += 1
                if consecutive_errors == 1 or consecutive_errors % 50 == 0:
                    self.get_logger().warn(
                        f'UBX read error (x{consecutive_errors}): {e}'
                    )
                continue

            if msg is None:
                continue

            if msg.identity == 'NAV-SVIN':
                acc_m = msg.meanAcc / 10000.0
                acc_limit_m = self._survey_acc_limit / 1000.0

                # Log when something meaningful changes (active state, 0.1m acc
                # step, 10s dur step), and at least every HEARTBEAT_S so a stuck
                # survey (e.g. active stuck at 0) stays visible.
                now = time.monotonic()
                log_key = (msg.active, round(acc_m, 1), msg.dur // 10)
                if log_key != last_log or (now - last_log_time) >= HEARTBEAT_S:
                    self.get_logger().info(
                        f'Survey: active={msg.active}, valid={msg.valid}, '
                        f'acc={acc_m:.2f} m, dur={msg.dur} s'
                    )
                    last_log = log_key
                    last_log_time = now

                threshold_met = acc_m <= acc_limit_m and msg.dur >= self._survey_min_dur
                if msg.valid or threshold_met:
                    reason = 'receiver valid' if msg.valid else f'threshold met ({acc_m:.2f} m <= {acc_limit_m:.1f} m, {msg.dur} s)'
                    self.get_logger().info(f'Survey-in complete ({reason}) — streaming RTCM')
                    return

    # ------------------------------------------------------------------
    # Step 3 — Stream RTCM messages and publish
    # ------------------------------------------------------------------

    def _stream_rtcm(self):
        # quitonerror=0 makes RTCMReader silently skip non-RTCM frames (e.g. UBX)
        reader = RTCMReader(self._serial, quitonerror=0)
        consecutive_errors = 0

        for raw, _ in reader:
            if not rclpy.ok():
                break
            try:
                if raw:
                    out = UInt8MultiArray()
                    out.data = list(raw)
                    self._rtcm_pub.publish(out)
                    consecutive_errors = 0
            except Exception as e:
                consecutive_errors += 1
                if consecutive_errors == 1 or consecutive_errors % 50 == 0:
                    self.get_logger().error(
                        f'RTCM publish error (x{consecutive_errors}): {e} — corrections may have stopped'
                    )


def main(args=None):
    rclpy.init(args=args)
    node = RtcmReaderNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
