#!/usr/bin/env python3
"""Optional plot of U_manual vs U_cali from serial_log.txt."""

import re
import sys
from pathlib import Path

try:
    import matplotlib.pyplot as plt
except ImportError:
    print("pip install matplotlib")
    sys.exit(1)

path = Path(sys.argv[1] if len(sys.argv) > 1 else "serial_log.txt")
raws = []
manual = []
cali = []
pat = re.compile(r"(\d+)\s+(\d+)\s+(\d+)\s+(\d+\.\d+)")
for line in path.read_text(encoding="utf-8").splitlines():
    m = pat.search(line)
    if not m:
        continue
    raws.append(int(m.group(1)))
    manual.append(int(m.group(2)))
    cali.append(int(m.group(3)))

if not raws:
    print("no table rows in", path)
    sys.exit(1)

plt.figure(figsize=(7, 4))
plt.plot(raws, manual, "o-", label="U_manual")
plt.plot(raws, cali, "s-", label="U_cali")
plt.xlabel("RAW")
plt.ylabel("mV")
plt.title("ADC: formula vs calibrated (nonlinearity at high RAW)")
plt.legend()
plt.grid(True)
plt.tight_layout()
out = path.with_suffix(".png")
plt.savefig(out, dpi=120)
print("saved", out)
