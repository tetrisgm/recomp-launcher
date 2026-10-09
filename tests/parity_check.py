#!/usr/bin/env python3
"""parity_check.py <repo>: every GameInfo, NetplayCallbacks and ModProvider
member is either used by the launcher (src/, titles/) or listed under
"Not wired by psxrecomp / not applicable" in docs/PARITY.md."""
import os, re, sys

root = sys.argv[1] if len(sys.argv) > 1 else "."
hdr = open(os.path.join(root, "src/recomp_launcher.h")).read()

def members(struct):
    m = re.search(r"typedef struct %s \{(.*?)\} %s;" % (struct, struct), hdr, re.S)
    body = re.sub(r"/\*.*?\*/", "", m.group(1), flags=re.S)
    body = re.sub(r"//[^\n]*", "", body)
    names = set()
    for decl in body.split(";"):
        decl = decl.strip()
        if not decl:
            continue
        f = re.search(r"\(\s*\*\s*(\w+)\s*\)", decl)
        if f:
            names.add(f.group(1)); continue
        f = re.findall(r"(\w+)\s*(?:\[[^\]]*\])*\s*$", decl)
        if f:
            names.add(f[-1])
    return names

src = ""
for d in ("src/r4l", "titles"):
    for dp, _, fs in os.walk(os.path.join(root, d)):
        for f in fs:
            if f.endswith((".cpp", ".c", ".h")):
                src += open(os.path.join(dp, f), errors="replace").read()
parity = open(os.path.join(root, "docs/PARITY.md")).read()
na_block = parity.split("## Not wired by psxrecomp / not applicable", 1)[-1]
na = set(re.findall(r"`(\w+)`", na_block))

missing = []
for struct in ("RecompLauncherCGameInfo", "RecompLauncherCNetplayCallbacks", "RecompLauncherCModProvider"):
    for name in sorted(members(struct)):
        if name in ("ctx",):
            continue
        used = re.search(r"(->|\.)%s\b" % re.escape(name), src)
        if not used and name not in na:
            missing.append("%s.%s" % (struct, name))
if missing:
    print("parity_check: %d members neither used nor listed as N/A:" % len(missing))
    for m in missing:
        print("  " + m)
    sys.exit(1)
print("parity_check: every ABI member is used or listed as N/A")
