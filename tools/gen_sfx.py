#!/usr/bin/env python3
"""Regenerate the R4 skin's menu blips (assets/skins/r4/sfx/*.wav).

They are synthesised here: a short linear frequency sweep of a sine wave with
a quick attack and a squared decay, 22.05 kHz mono 16-bit. No recorded or
game audio is involved.
"""
import math, os, struct, wave

def tone(path, f0, f1, ms, vol=0.35):
    sr = 22050
    n = int(sr * ms / 1000)
    data = b""
    for i in range(n):
        t = i / sr
        f = f0 + (f1 - f0) * i / n
        env = min(1, i / 40) * (1 - i / n) ** 2
        data += struct.pack("<h", int(32767 * vol * env * math.sin(2 * math.pi * f * t)))
    with wave.open(path, "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr); w.writeframes(data)

out = os.path.join(os.path.dirname(__file__), "..", "assets", "skins", "r4", "sfx")
tone(os.path.join(out, "move.wav"), 1400, 1600, 45)
tone(os.path.join(out, "confirm.wav"), 880, 1760, 120)
tone(os.path.join(out, "back.wav"), 900, 450, 110)
