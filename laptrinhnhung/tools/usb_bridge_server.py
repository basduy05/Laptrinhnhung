import json
import os
import threading
import time
import datetime
import csv
import io
import unicodedata
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs
from urllib.request import Request, urlopen

try:
    import serial  # pip install pyserial
    from serial.tools import list_ports
except Exception as e:  # pragma: no cover
    serial = None
    list_ports = None


HOST = "127.0.0.1"
PORT = 8081

# Change these for your PC
# - Set to "AUTO" to auto-pick a likely ESP32 port.
# - Or set explicitly: "COM3", "COM5", ...
# You can also override via environment variables:
# - USB_SERIAL_PORT=COM5
# - USB_SERIAL_BAUD=115200
SERIAL_PORT = (os.getenv("USB_SERIAL_PORT") or os.getenv("SERIAL_PORT") or "AUTO").strip() or "AUTO"
BAUD = int((os.getenv("USB_SERIAL_BAUD") or os.getenv("BAUD") or "115200").strip() or "115200")


# Optional: enrich RFID history timestamps from Google Sheet.
# Provide a public CSV export URL (or a JSON endpoint that returns rows).
# Example CSV export URL format:
# https://docs.google.com/spreadsheets/d/<SHEET_ID>/gviz/tq?tqx=out:csv&sheet=<SHEET_NAME>
RFID_SHEET_CSV_URL = os.getenv("RFID_SHEET_CSV_URL", "").strip()
RFID_SHEET_CACHE_TTL_SEC = int(os.getenv("RFID_SHEET_CACHE_TTL_SEC", "15") or "15")

_sheet_cache_lock = threading.Lock()
_sheet_cache = {
    "fetched_at": 0.0,
    # uid -> {"sheet_at": str|None, "epoch_ms": int|0, "name": str|None}
    # (name/time columns are best-effort and may be absent depending on the Sheet)
    "uid_to_info": {},
}


_last_state_lock = threading.Lock()
_last_state = {
    "temperature_c": None,
    "humidity_percent": None,
    "gas_value": None,
    "fire_status": "Unknown",
    "updated_at": None,
    # Keep a stable shape for the dashboard even before the first ESP32 state frame arrives.
    # The ESP32 will overwrite this with real values.
    "devices": {
        "light1": False,
        "light2": False,
        "light3": False,
        "fan1": False,
        "fan2": False,
        "curtain_open": False,
        "curtain_state": "STOP",
        "door_open": False,
        "door_last_method": "",
        "door_emergency_pin_set": False,
        "light1_color": "YELLOW",
        "light2_color": "YELLOW",
        "light3_color": "YELLOW",
        "lights_all_color": "YELLOW",
    },
}

_status_lock = threading.Lock()
_status = {
    "serial_connected": False,
    "serial_port": None,
    "serial_error": None,
    "last_state_at": None,
}


_rfid_lock = threading.Lock()
_rfid = {
    "last": {
        "uid": None,
        "authorized": None,
        "method": None,
        "at": None,
    },
    "history": [],
    "cards": [],
    "card_meta": {},  # uid -> {"name": str, "updated_at": float}
    "cards_updated_at": None,
}


def _norm_key(s: object) -> str:
    txt = str(s or "").strip().lower()
    if not txt:
        return ""
    txt = unicodedata.normalize("NFKD", txt)
    txt = "".join(ch for ch in txt if not unicodedata.combining(ch))
    out = []
    for ch in txt:
        if ch.isalnum():
            out.append(ch)
        else:
            out.append(" ")
    return " ".join("".join(out).split())


def _epoch_ms_from_iso(x: object) -> int:
    try:
        s = str(x or "").strip()
        if not s:
            return 0
        # Handle trailing Z (UTC)
        if s.endswith("Z"):
            s = s[:-1] + "+00:00"
        dt = datetime.datetime.fromisoformat(s)
        if dt.tzinfo is None:
            dt = dt.replace(tzinfo=datetime.timezone.utc)
        return int(dt.timestamp() * 1000)
    except Exception:
        return 0


def _parse_sheet_time(value: object) -> tuple[int, str] | None:
    """Return (epoch_ms, iso_string) if parseable."""
    s = str(value or "").strip()
    if not s:
        return None
    # Try ISO first
    try:
        iso = s
        if iso.endswith("Z"):
            iso = iso[:-1] + "+00:00"
        dt = datetime.datetime.fromisoformat(iso)
        if dt.tzinfo is None:
            # Assume Vietnam time if sheet time has no tz
            dt = dt.replace(tzinfo=datetime.timezone(datetime.timedelta(hours=7)))
        return int(dt.timestamp() * 1000), dt.isoformat()
    except Exception:
        pass

    # Common VN formats
    tz_vn = datetime.timezone(datetime.timedelta(hours=7))
    fmts = [
        "%d/%m/%Y %H:%M:%S",
        "%d/%m/%Y %H:%M",
        "%Y-%m-%d %H:%M:%S",
        "%Y-%m-%d %H:%M",
        "%m/%d/%Y %H:%M:%S",
        "%m/%d/%Y %H:%M",
    ]
    for fmt in fmts:
        try:
            dt = datetime.datetime.strptime(s, fmt).replace(tzinfo=tz_vn)
            return int(dt.timestamp() * 1000), dt.isoformat()
        except Exception:
            continue
    return None


def _fetch_sheet_rows(url: str) -> list[dict]:
    if not url:
        return []
    try:
        req = Request(url, headers={"User-Agent": "usb-bridge/1.0"})
        with urlopen(req, timeout=4) as resp:
            raw = resp.read()
        text = raw.decode("utf-8-sig", errors="ignore").strip()
        if not text:
            return []

        # JSON endpoint support
        if text.startswith("[") or text.startswith("{"):
            try:
                obj = json.loads(text)
                if isinstance(obj, list):
                    return [x for x in obj if isinstance(x, dict)]
                if isinstance(obj, dict):
                    for k in ("rows", "data", "items"):
                        v = obj.get(k)
                        if isinstance(v, list):
                            return [x for x in v if isinstance(x, dict)]
            except Exception:
                return []

        # CSV
        f = io.StringIO(text)
        reader = csv.DictReader(f)
        out = []
        for row in reader:
            if isinstance(row, dict):
                out.append(row)
        return out
    except Exception:
        return []


def _get_sheet_uid_info_map() -> dict:
    """Return mapping uid-> {sheet_at, epoch_ms, name}. Cached.

    - Requires a UID-like column.
    - Time and Name columns are optional (best-effort).
    """
    url = RFID_SHEET_CSV_URL
    if not url:
        return {}
    now = time.time()
    with _sheet_cache_lock:
        if (now - float(_sheet_cache.get("fetched_at") or 0.0)) < float(RFID_SHEET_CACHE_TTL_SEC or 15):
            cached = _sheet_cache.get("uid_to_info")
            return dict(cached) if isinstance(cached, dict) else {}

    rows = _fetch_sheet_rows(url)
    if not rows:
        with _sheet_cache_lock:
            _sheet_cache["fetched_at"] = now
            _sheet_cache["uid_to_info"] = {}
        return {}

    # Find likely columns
    keys = list(rows[0].keys()) if isinstance(rows[0], dict) else []
    norm = {_norm_key(k): k for k in keys}

    def pick_col(cands: list[str]) -> str | None:
        for c in cands:
            nk = _norm_key(c)
            if nk in norm:
                return norm[nk]
        # fallback: fuzzy contains
        for nk, orig in norm.items():
            for c in cands:
                if _norm_key(c) and _norm_key(c) in nk:
                    return orig
        return None

    uid_col = pick_col(["uid", "rfid", "card", "ma the", "mathe", "id the", "idthe"])  # best-effort
    time_col = pick_col(["time", "timestamp", "datetime", "thoi gian", "thoigian", "ngay gio", "ngaygio"])  # optional
    name_col = pick_col(["name", "ten", "ten the", "tenthe", "chu the", "chuthe", "holder", "owner"])  # optional
    if not uid_col:
        with _sheet_cache_lock:
            _sheet_cache["fetched_at"] = now
            _sheet_cache["uid_to_info"] = {}
        return {}

    uid_to_best: dict[str, dict] = {}
    for row in rows:
        if not isinstance(row, dict):
            continue
        uid = str(row.get(uid_col) or "").strip().upper().replace(" ", "")
        if not uid:
            continue
        name = str(row.get(name_col) or "").strip() if name_col else ""

        epoch_ms = 0
        iso = None
        if time_col:
            parsed = _parse_sheet_time(row.get(time_col))
            if parsed:
                epoch_ms, iso = parsed

        prev = uid_to_best.get(uid)
        if not prev:
            uid_to_best[uid] = {"sheet_at": iso, "epoch_ms": epoch_ms, "name": name}
            continue

        # Prefer the newest timestamp if available; otherwise, keep whichever has a non-empty name.
        prev_ms = int(prev.get("epoch_ms") or 0)
        prev_name = str(prev.get("name") or "").strip()
        should_replace = False
        if epoch_ms and (epoch_ms > prev_ms):
            should_replace = True
        elif (not prev_name) and name:
            should_replace = True

        if should_replace:
            uid_to_best[uid] = {"sheet_at": iso or prev.get("sheet_at"), "epoch_ms": max(prev_ms, epoch_ms), "name": name or prev_name}

    with _sheet_cache_lock:
        _sheet_cache["fetched_at"] = now
        _sheet_cache["uid_to_info"] = dict(uid_to_best)
    return dict(uid_to_best)


def _get_sheet_uid_time_map() -> dict:
    """Backward-compatible: uid -> {sheet_at, epoch_ms}."""
    info = _get_sheet_uid_info_map()
    out = {}
    for uid, v in info.items():
        if not isinstance(v, dict):
            continue
        sheet_at = v.get("sheet_at")
        epoch_ms = int(v.get("epoch_ms") or 0)
        if sheet_at or epoch_ms:
            out[uid] = {"sheet_at": sheet_at, "epoch_ms": epoch_ms}
    return out


def _get_rfid() -> dict:
    with _rfid_lock:
        return {
            "last": dict(_rfid["last"]),
            "history": list(_rfid["history"]),
            "cards": list(_rfid["cards"]),
            "card_meta": dict(_rfid.get("card_meta") or {}),
            "cards_updated_at": _rfid["cards_updated_at"],
        }


def _get_card_meta(uid: str | None) -> dict:
    if not uid:
        return {}
    u = str(uid).strip().upper().replace(" ", "")
    if not u:
        return {}
    with _rfid_lock:
        meta = (_rfid.get("card_meta") or {}).get(u)
        return dict(meta) if isinstance(meta, dict) else {}


def _set_rfid_last(uid: str | None, authorized: bool | None, method: str | None) -> None:
    with _rfid_lock:
        _rfid["last"] = {
            "uid": uid,
            "authorized": authorized,
            "method": method,
            "at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        }


def _push_rfid_history(item: dict) -> None:
    with _rfid_lock:
        _rfid["history"].insert(0, item)
        # keep last 30
        _rfid["history"] = _rfid["history"][:30]


def _set_rfid_cards(cards: list[str]) -> None:
    with _rfid_lock:
        cleaned = []
        for c in cards:
            u = str(c).strip().upper().replace(" ", "")
            if u and u not in cleaned:
                cleaned.append(u)

        _rfid["cards"] = cleaned
        # remove metadata for cards no longer present
        meta = _rfid.get("card_meta") or {}
        if isinstance(meta, dict):
            keep = set(cleaned)
            for k in list(meta.keys()):
                if k not in keep:
                    meta.pop(k, None)
            _rfid["card_meta"] = meta
        _rfid["cards_updated_at"] = time.time()


def _set_status(**kwargs) -> None:
    with _status_lock:
        _status.update(kwargs)


def _get_status() -> dict:
    with _status_lock:
        return dict(_status)


def _list_serial_ports() -> list[dict]:
    if list_ports is None:
        return []
    out = []
    for p in list_ports.comports():
        out.append({
            "device": getattr(p, "device", None),
            "description": getattr(p, "description", None),
            "hwid": getattr(p, "hwid", None),
        })
    return out


def _auto_pick_port() -> str | None:
    ports = _list_serial_ports()
    if not ports:
        return None

    def score(p: dict) -> int:
        text = f"{p.get('device','')} {p.get('description','')} {p.get('hwid','')}".lower()
        s = 0
        # common ESP32 USB-UART chips / keywords
        for kw in ["silicon labs", "cp210", "wch", "ch340", "usb serial", "uart", "esp32", "cdc"]:
            if kw in text:
                s += 10
        if (p.get("device") or "").upper().startswith("COM"):
            s += 1
        return s

    ports_sorted = sorted(ports, key=score, reverse=True)
    return ports_sorted[0].get("device")


def _set_last_state(update: dict) -> None:
    global _last_state
    with _last_state_lock:
        if not isinstance(update, dict):
            return

        # Deep-merge nested `devices` so partial updates can't drop keys.
        if isinstance(update.get("devices"), dict):
            prev_devices = _last_state.get("devices")
            if not isinstance(prev_devices, dict):
                prev_devices = {}
            merged_devices = {**prev_devices, **update.get("devices")}
            update = dict(update)
            update["devices"] = merged_devices

        _last_state = {**_last_state, **update}


def _get_last_state() -> dict:
    with _last_state_lock:
        state = dict(_last_state)
        if not isinstance(state.get("devices"), dict):
            state["devices"] = {}
        return state


class SerialWorker(threading.Thread):
    def __init__(self, port: str, baud: int):
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self._ser = None
        self._tx_lock = threading.Lock()

    def connect(self) -> None:
        if serial is None:
            raise RuntimeError("pyserial not installed. Run: pip install pyserial")
        self._ser = serial.Serial(self.port, self.baud, timeout=1)
        _set_status(serial_connected=True, serial_port=self.port, serial_error=None)
        # Let ESP32 reset and boot
        time.sleep(2)
        # Ask ESP32 to publish current RFID cards
        self.send_line("RFID_LIST")

    def send_line(self, line: str) -> None:
        if not self._ser:
            return
        payload = (line.strip() + "\n").encode("utf-8", errors="ignore")
        with self._tx_lock:
            self._ser.write(payload)

    def run(self) -> None:
        while True:
            try:
                self.connect()
                while True:
                    raw = self._ser.readline()
                    if not raw:
                        continue
                    line = raw.decode("utf-8", errors="ignore").strip()
                    if not line:
                        continue

                    # Expect JSON lines from ESP32
                    if line.startswith("{") and line.endswith("}"):
                        try:
                            msg = json.loads(line)
                        except Exception:
                            continue

                        # Our sketch sends {"type":"state", ...}
                        if msg.get("type") == "state":
                            msg.pop("type", None)
                            _set_last_state(msg)
                            _set_status(last_state_at=time.time())

                        if msg.get("type") == "rfid_scan":
                            uid = msg.get("uid")
                            authorized = msg.get("authorized")
                            method = msg.get("method")
                            _set_rfid_last(uid, bool(authorized) if authorized is not None else None, method)
                            _push_rfid_history({
                                "uid": uid,
                                "authorized": authorized,
                                "method": method,
                                "at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                            })

                        if msg.get("type") == "rfid_cards":
                            cards = msg.get("cards")
                            if isinstance(cards, list):
                                _set_rfid_cards([str(x) for x in cards])
            except Exception as e:
                _set_status(serial_connected=False, serial_error=str(e))
                try:
                    if self._ser:
                        self._ser.close()
                except Exception:
                    pass
                self._ser = None
                time.sleep(1.0)


_serial_worker = None


class Handler(BaseHTTPRequestHandler):
    def _cors(self) -> None:
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET,POST,OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def do_OPTIONS(self):  # noqa: N802
        self.send_response(204)
        self._cors()
        self.end_headers()

    def do_GET(self):  # noqa: N802
        parsed = urlparse(self.path)
        path = parsed.path
        qs = parse_qs(parsed.query)

        if path == "/health":
            body = {
                "ok": True,
                "host": HOST,
                "port": PORT,
                "serial": _get_status(),
                "known_ports": _list_serial_ports(),
            }
            payload = json.dumps(body, ensure_ascii=False).encode("utf-8")
            self.send_response(200)
            self._cors()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(payload)
            return

        if path == "/sensors":
            state = _get_last_state()
            body = json.dumps(state, ensure_ascii=False).encode("utf-8")
            self.send_response(200)
            self._cors()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(body)
            return

        if path == "/control":
            cmd = (qs.get("cmd", [""])[0] or "").strip()
            if not cmd:
                self.send_response(400)
                self._cors()
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(b"{\"ok\":false,\"error\":\"Missing cmd\"}")
                return

            status = _get_status()
            if not _serial_worker or not bool(status.get("serial_connected")):
                self.send_response(503)
                self._cors()
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                err = {
                    "ok": False,
                    "error": "Serial not connected",
                    "serial": status,
                }
                self.wfile.write(json.dumps(err, ensure_ascii=False).encode("utf-8"))
                return

            # Protocol: send plain command line
            _serial_worker.send_line(cmd)
            # Ask for an immediate state frame (ESP32 prints every 1s anyway, this just speeds UI sync)
            _serial_worker.send_line("STATE")

            self.send_response(200)
            self._cors()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b"{\"ok\":true}")
            return

        if path == "/rfid/last":
            r = _get_rfid()
            last = dict(r["last"])
            meta = _get_card_meta(last.get("uid"))
            if meta and meta.get("name"):
                last["name"] = meta.get("name")
            else:
                sheet_map = _get_sheet_uid_info_map()
                uid_key = str(last.get("uid") or "").strip().upper().replace(" ", "")
                if uid_key and uid_key in sheet_map:
                    n = (sheet_map[uid_key] or {}).get("name")
                    if n:
                        last["name"] = n
            payload = json.dumps(last, ensure_ascii=False).encode("utf-8")
            self.send_response(200)
            self._cors()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(payload)
            return

        if path == "/rfid/history":
            r = _get_rfid()
            sheet_map = _get_sheet_uid_info_map()
            out = []
            for item in r["history"]:
                if not isinstance(item, dict):
                    continue
                row = dict(item)
                meta = _get_card_meta(row.get("uid"))
                if meta and meta.get("name"):
                    row["name"] = meta.get("name")
                uid_key = str(row.get("uid") or "").strip().upper().replace(" ", "")
                if uid_key and uid_key in sheet_map:
                    row["sheet_at"] = (sheet_map[uid_key] or {}).get("sheet_at")
                    if not row.get("name"):
                        n = (sheet_map[uid_key] or {}).get("name")
                        if n:
                            row["name"] = n
                out.append(row)

            # Newest first (prefer sheet_at if present, else at)
            out.sort(
                key=lambda x: _epoch_ms_from_iso(x.get("sheet_at")) or _epoch_ms_from_iso(x.get("at")) if isinstance(x, dict) else 0,
                reverse=True,
            )
            payload = json.dumps({"history": out}, ensure_ascii=False).encode("utf-8")
            self.send_response(200)
            self._cors()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(payload)
            return

        if path == "/rfid/cards":
            r = _get_rfid()
            cards_out = []
            sheet_map = _get_sheet_uid_info_map()
            meta = r.get("card_meta") or {}
            for uid in r["cards"]:
                m = meta.get(uid) if isinstance(meta, dict) else None
                sheet_name = (sheet_map.get(uid) or {}).get("name") if isinstance(sheet_map, dict) else ""
                if isinstance(m, dict):
                    cards_out.append({
                        "uid": uid,
                        "name": m.get("name") or (sheet_name or ""),
                    })
                else:
                    cards_out.append({"uid": uid, "name": sheet_name or ""})
            payload = json.dumps({"cards": cards_out, "updated_at": r["cards_updated_at"]}, ensure_ascii=False).encode("utf-8")
            self.send_response(200)
            self._cors()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(payload)
            return

        self.send_response(404)
        self._cors()
        self.send_header("Content-Type", "text/plain")
        self.end_headers()
        self.wfile.write(b"Not found")

    def do_POST(self):  # noqa: N802
        parsed = urlparse(self.path)
        path = parsed.path

        if path == "/rfid/cards":
            length = int(self.headers.get("Content-Length", "0") or "0")
            raw = self.rfile.read(length) if length > 0 else b"{}"
            try:
                data = json.loads(raw.decode("utf-8", errors="ignore"))
            except Exception:
                data = {}

            action = str(data.get("action") or "").lower().strip()
            uid = str(data.get("uid") or "").strip().upper().replace(" ", "")
            if action == "del":
                action = "delete"

            if not uid or action not in {"add", "delete"}:
                self.send_response(400)
                self._cors()
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(b"{\"ok\":false,\"error\":\"Missing uid or invalid action\"}")
                return

            if action == "add":
                name = str(data.get("name") or "").strip()
                if not name:
                    self.send_response(400)
                    self._cors()
                    self.send_header("Content-Type", "application/json")
                    self.end_headers()
                    self.wfile.write(b"{\"ok\":false,\"error\":\"Missing name\"}")
                    return
                with _rfid_lock:
                    meta = _rfid.get("card_meta")
                    if not isinstance(meta, dict):
                        meta = {}
                    meta[uid] = {"name": name, "updated_at": time.time()}
                    _rfid["card_meta"] = meta

            if _serial_worker:
                if action == "add":
                    _serial_worker.send_line(f"RFID_ADD:{uid}")
                else:
                    _serial_worker.send_line(f"RFID_DEL:{uid}")
                # ask for latest list
                _serial_worker.send_line("RFID_LIST")

            if action == "delete":
                with _rfid_lock:
                    meta = _rfid.get("card_meta")
                    if isinstance(meta, dict):
                        meta.pop(uid, None)
                        _rfid["card_meta"] = meta

            self.send_response(200)
            self._cors()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b"{\"ok\":true}")
            return

        self.send_response(404)
        self._cors()
        self.send_header("Content-Type", "text/plain")
        self.end_headers()
        self.wfile.write(b"Not found")

    def log_message(self, format, *args):  # silence default logging
        return


def main() -> None:
    global _serial_worker

    port = SERIAL_PORT
    if port.upper() == "AUTO":
        picked = _auto_pick_port()
        if not picked:
            print("No serial ports found. Plug in ESP32 and check Device Manager (Ports/COM).")
            print("If you know the port, set SERIAL_PORT = 'COMx' in this file.")
            picked = "COM5"  # fallback placeholder
        port = picked

    print(f"Starting USB bridge on http://{HOST}:{PORT}")
    print(f"Serial target: {port} @ {BAUD}")
    ports = _list_serial_ports()
    if ports:
        print("Detected ports:")
        for p in ports:
            print(f"- {p.get('device')} | {p.get('description')} | {p.get('hwid')}")

    _serial_worker = SerialWorker(port, BAUD)
    _serial_worker.start()

    httpd = ThreadingHTTPServer((HOST, PORT), Handler)
    httpd.serve_forever()


if __name__ == "__main__":
    main()
