"""
ctype.pyw - inline calculator for Windows 11

Type a formula anywhere, press "=", and the result is typed right after it.

    12*(3+4)=        ->  12*(3+4)=84
    10/3=            ->  10/3=3.33
    10/3=3.33=       ->  10/3=3.33=3.3333     (second "=" gives 4 decimals)
    2^10=  2**10=    ->  1024
    17%5=  17//5=    ->  2  /  3

Dates (ISO, optional @ marker), Excel style:
    @2026-04-01 - @2026-03-01=   ->  31            (days between)
    2026-04-01 + 55 =            ->  @2026-05-26
    @2026-05-26 - 7=             ->  @2026-05-19

Results chain: after "3+4=7" keep typing "*2=" and you get 14.
The typed result is also copied to the clipboard.

Hotkeys
    Ctrl+Alt+Shift+C   pause / resume   (high beep = on, low beep = off)
    Ctrl+Alt+Shift+Q   quit

Setup
    pip install pynput pyperclip
    pyw ctype.pyw          (pyw = runs without a console window)
"""

import ast
import ctypes
import math
import operator
import queue
import random
import re
import sys
import threading
import time
from datetime import date, timedelta

import pyperclip
import winsound
from pynput import keyboard, mouse

# ------------------------------- settings -----------------------------------
TYPE_DELAY = (0.111, 0.155)   # seconds between emulated keystrokes
TYPED_DECIMALS = 2            # max decimals on the first "="
PRECISE_DECIMALS = 4          # max decimals when "=" is pressed a second time
MAX_BUFFER = 200              # characters of typing history kept in memory
MAX_RESULT_LEN = 60           # never type absurdly long results
TOGGLE_HOTKEY = "<ctrl>+<alt>+<shift>+c"
QUIT_HOTKEY = "<ctrl>+<alt>+<shift>+q"
# -----------------------------------------------------------------------------

Key = keyboard.Key
ALLOWED_CHARS = set("0123456789.+-*/%^()@ ")

CTRL_KEYS = {Key.ctrl, Key.ctrl_l, Key.ctrl_r}
ALT_KEYS = {Key.alt, Key.alt_l, Key.alt_r, Key.alt_gr}
WIN_KEYS = {Key.cmd, Key.cmd_l, Key.cmd_r}
SHIFT_KEYS = {Key.shift, Key.shift_l, Key.shift_r}
MODIFIER_KEYS = CTRL_KEYS | ALT_KEYS | WIN_KEYS | SHIFT_KEYS

# Keys that move the caret or submit text: whatever we remembered is now stale.
RESET_KEYS = {
    Key.enter, Key.tab, Key.esc, Key.up, Key.down, Key.left, Key.right,
    Key.home, Key.end, Key.page_up, Key.page_down, Key.delete, Key.insert,
}

# Numpad keys sometimes arrive without a char, only a virtual-key code.
NUMPAD_VK = {96 + i: str(i) for i in range(10)}
NUMPAD_VK.update({106: "*", 107: "+", 109: "-", 110: ".", 111: "/"})

# On layouts where ^ is a dead key it can be reported as a combining circumflex.
DEAD_KEY_CHARS = {"\u0302": "^"}

# ISO date, optionally marked with @:  @2026-04-01  or  2026-04-01
DATE_RE = re.compile(r"(?<!\d)@?(\d{4})-(\d{2})-(\d{2})(?!\d)")


# ----------------------------- safe evaluator --------------------------------
class CalcError(Exception):
    pass


def _day_offset(n):
    if isinstance(n, date):
        raise CalcError("date + date is meaningless")
    return math.floor(n)  # Excel style: fractional days are dropped (floored)


def _add(a, b):
    if isinstance(a, date):
        return a + timedelta(days=_day_offset(b))
    if isinstance(b, date):
        return b + timedelta(days=_day_offset(a))
    return a + b


def _sub(a, b):
    if isinstance(a, date) and isinstance(b, date):
        return (a - b).days
    if isinstance(a, date):
        return a + timedelta(days=_day_offset(-b))
    if isinstance(b, date):
        raise CalcError("number - date is meaningless")
    return a - b


def _safe_pow(a, b):
    # Refuse integer powers that would take ages / eat all memory (9**9**9).
    if isinstance(a, int) and isinstance(b, int) and b > 0 and abs(a) > 1:
        if b * math.log2(abs(a)) > 4000:
            raise CalcError("result too large")
    result = a ** b
    if isinstance(result, complex):
        raise CalcError("complex result")
    return result


_BINOPS = {
    ast.Add: _add,
    ast.Sub: _sub,
    ast.Mult: operator.mul,
    ast.Div: operator.truediv,
    ast.FloorDiv: operator.floordiv,
    ast.Mod: operator.mod,
    ast.Pow: _safe_pow,
}
_UNOPS = {ast.UAdd: operator.pos, ast.USub: operator.neg}


def _eval(node, env):
    if isinstance(node, ast.Expression):
        return _eval(node.body, env)
    if isinstance(node, ast.Constant) and type(node.value) in (int, float):
        return node.value
    if isinstance(node, ast.Name) and node.id in env:
        return env[node.id]
    if isinstance(node, ast.BinOp) and type(node.op) in _BINOPS:
        left, right = _eval(node.left, env), _eval(node.right, env)
        # date * 2, date % 7, -date ... raise TypeError -> no result
        return _BINOPS[type(node.op)](left, right)
    if isinstance(node, ast.UnaryOp) and type(node.op) in _UNOPS:
        return _UNOPS[type(node.op)](_eval(node.operand, env))
    raise CalcError("unsupported expression")


def _prepare(candidate):
    """Swap dates for placeholder names, turn ^ into **, drop leading zeros.
    Raises ValueError for an impossible date such as 2026-02-30."""
    env = {}

    def to_name(m):
        name = f"_d{chr(65 + len(env))}"  # _dA, _dB, ...
        env[name] = date(int(m[1]), int(m[2]), int(m[3]))
        return name

    expr = DATE_RE.sub(to_name, candidate)
    expr = expr.replace("^", "**")
    expr = re.sub(r"(?<![\d.])0+(?=\d)", "", expr)  # "007" -> "7"
    return expr, env


def evaluate_tail(text):
    """Evaluate the longest valid formula at the end of `text`, or return None."""
    i = len(text)
    while i > 0 and text[i - 1] in ALLOWED_CHARS:
        i -= 1
    tail = text[i:]

    for start in range(len(tail)):
        candidate = tail[start:].strip()
        if not candidate:
            break
        try:
            expr, env = _prepare(candidate)
        except ValueError:
            return None  # looked like a date but isn't a real one
        try:
            tree = ast.parse(expr, mode="eval")
        except SyntaxError:
            continue
        # A lone number or date ("x = 5") is not a calculation.
        if not any(isinstance(n, ast.BinOp) for n in ast.walk(tree)):
            return None
        try:
            value = _eval(tree, env)
        except (CalcError, ArithmeticError, ValueError, TypeError):
            return None
        if isinstance(value, float) and not math.isfinite(value):
            return None
        return value
    return None


def format_value(value, decimals):
    if isinstance(value, date):
        return "@" + value.isoformat()
    if isinstance(value, int):
        return str(value)
    s = f"{value:.{decimals}f}".rstrip("0").rstrip(".")
    return "0" if s in ("", "-0") else s


# ------------------------------ the app --------------------------------------
def char_of(key):
    if isinstance(key, keyboard.KeyCode):
        ch = key.char
        if ch:
            return DEAD_KEY_CHARS.get(ch, ch)
        if key.vk in NUMPAD_VK:
            return NUMPAD_VK[key.vk]
    return None


class CalcTyper:
    def __init__(self):
        self.buffer = ""
        self.lock = threading.Lock()
        self.busy = threading.Event()   # set while we evaluate / type
        self.enabled = True
        self.jobs = queue.Queue()
        self.held_mods = set()
        self.kb = keyboard.Controller()
        # (buffer right after a rounded result was typed, exact value).
        # A second "=" while the buffer still matches types the precise value.
        self.precise_pending = None

    # --- helpers ---
    def _append(self, s):
        self.buffer = (self.buffer + s)[-MAX_BUFFER:]

    def _is_shortcut(self):
        held = self.held_mods
        if held & WIN_KEYS:
            return True
        # Ctrl+Alt together = AltGr (normal typing); only one of them = shortcut.
        return bool(held & CTRL_KEYS) != bool(held & ALT_KEYS)

    def _wait_for_modifiers(self, timeout=1.5):
        # Don't type while the user still holds Shift etc. from pressing "=".
        deadline = time.time() + timeout
        while self.held_mods and time.time() < deadline:
            time.sleep(0.02)

    @staticmethod
    def _beep(freq):
        threading.Thread(target=winsound.Beep, args=(freq, 120), daemon=True).start()

    # --- listeners ---
    def on_press(self, key):
        if key in MODIFIER_KEYS:
            self.held_mods.add(key)
            return
        if not self.enabled or self.busy.is_set():
            return  # also skips the keystrokes we inject ourselves

        with self.lock:
            if key == Key.backspace:
                self.buffer = self.buffer[:-1]
            elif key == Key.space:
                self._append(" ")
            elif key in RESET_KEYS:
                self.buffer = ""
            else:
                ch = char_of(key)
                if ch is None:
                    return
                if self._is_shortcut() or not ch.isprintable():
                    self.buffer = ""  # Ctrl+V, Ctrl+Z, ...: text is now unknown
                elif ch == "=":
                    self.busy.set()
                    pending = self.precise_pending
                    if pending and pending[0] == self.buffer:
                        self.jobs.put(("precise", pending[1]))
                    else:
                        self.jobs.put(("calc", self.buffer))
                    self.precise_pending = None
                    self._append("=")
                else:
                    self._append(ch)

    def on_release(self, key):
        self.held_mods.discard(key)

    def on_click(self, x, y, button, pressed):
        if pressed:
            with self.lock:
                self.buffer = ""

    # --- worker ---
    def _worker(self):
        while True:
            job = self.jobs.get()
            if job is None:
                return
            try:
                kind, payload = job
                if kind == "precise":
                    self._type_result(format_value(payload, PRECISE_DECIMALS))
                else:
                    self._calculate(payload)
            except Exception:
                pass
            finally:
                time.sleep(0.05)  # let our injected events pass the hook first
                self.busy.clear()

    def _calculate(self, text):
        value = evaluate_tail(text)
        if value is None:
            return
        typed = format_value(value, TYPED_DECIMALS)
        if not self._type_result(typed):
            return
        if isinstance(value, float) and format_value(value, PRECISE_DECIMALS) != typed:
            with self.lock:
                self.precise_pending = (self.buffer, value)

    def _type_result(self, typed):
        if len(typed) > MAX_RESULT_LEN:
            return False
        try:
            pyperclip.copy(typed)
        except Exception:
            pass

        self._wait_for_modifiers()
        for ch in typed:
            time.sleep(random.uniform(*TYPE_DELAY))
            self.kb.type(ch)

        with self.lock:
            self._append(typed)  # allows chaining: "3+4=7*2=14"
        return True

    # --- hotkeys ---
    def toggle(self):
        self.enabled = not self.enabled
        with self.lock:
            self.buffer = ""
            self.precise_pending = None
        self._beep(880 if self.enabled else 440)

    def quit(self):
        self.jobs.put(None)
        self.hotkeys.stop()
        self.mouse_listener.stop()
        self.kb_listener.stop()

    def run(self):
        threading.Thread(target=self._worker, daemon=True).start()
        self.mouse_listener = mouse.Listener(on_click=self.on_click)
        self.hotkeys = keyboard.GlobalHotKeys(
            {TOGGLE_HOTKEY: self.toggle, QUIT_HOTKEY: self.quit}
        )
        self.kb_listener = keyboard.Listener(
            on_press=self.on_press, on_release=self.on_release
        )
        self.mouse_listener.start()
        self.hotkeys.start()
        self.kb_listener.start()
        self._beep(880)
        self.kb_listener.join()


def main():
    # Single instance: a second copy would type every result twice.
    ctypes.windll.kernel32.CreateMutexW(None, False, "Local\\ctype_single_instance")
    if ctypes.windll.kernel32.GetLastError() == 183:  # ERROR_ALREADY_EXISTS
        sys.exit(0)
    CalcTyper().run()


if __name__ == "__main__":
    main()
