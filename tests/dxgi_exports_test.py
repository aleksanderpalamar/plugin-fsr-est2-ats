from os import environ
from pathlib import Path
from re import match
from shutil import which
from subprocess import check_output
from unittest import TestCase


WINDOWS_11_NAMES = {
    "ApplyCompatResolutionQuirking", "CompatString", "CompatValue",
    "CreateDXGIFactory", "CreateDXGIFactory1", "CreateDXGIFactory2",
    "DXGID3D10CreateDevice", "DXGID3D10CreateLayeredDevice",
    "DXGID3D10GetLayeredDeviceSize", "DXGID3D10RegisterLayers",
    "DXGIDeclareAdapterRemovalSupport", "DXGIDisableVBlankVirtualization",
    "DXGIDumpJournal", "DXGIGetDebugInterface1", "DXGIReportAdapterConfiguration",
    "PIXBeginCapture", "PIXEndCapture", "PIXGetCaptureState",
    "SetAppCompatStringPointer", "UpdateHMDEmulationStatus",
}


def exports(path: Path) -> tuple[set[int], set[str]]:
    output = check_output(["objdump", "-p", str(path)], text=True)
    ordinals: set[int] = set()
    names: set[str] = set()
    section = ""
    for line in output.splitlines():
        if line.startswith("Export Address Table -- Ordinal Base"):
            section = "ordinals"
            continue
        if line.startswith("[Ordinal/Name Pointer] Table"):
            section = "names"
            continue
        if section == "ordinals":
            entry = match(r"\s*\[\s*\d+\] \+base\[\s*(\d+)\] [0-9a-fA-F]+ Export RVA", line)
            if entry:
                ordinals.add(int(entry.group(1)))
        if section == "names":
            entry = match(r"\s*\[\s*\d+\] \+base\[\s*\d+\]\s+[0-9a-fA-F]+\s+(\w+)", line)
            if entry:
                names.add(entry.group(1))
    return ordinals, names


class DxgiExportTests(TestCase):
    def test_proxy_covers_observed_windows_11_exports(self) -> None:
        if not which("objdump"):
            self.skipTest("objdump is unavailable")
        proxy_path = Path(__file__).resolve().parents[1] / "build/windows-x64/dxgi.dll"
        if not proxy_path.is_file():
            self.skipTest("Cross-compiled proxy is unavailable")
        proxy_ordinals, proxy_names = exports(proxy_path)
        self.assertFalse(WINDOWS_11_NAMES - proxy_names, WINDOWS_11_NAMES - proxy_names)
        self.assertFalse(set(range(1, 21)) - proxy_ordinals,
                         set(range(1, 21)) - proxy_ordinals)

    def test_proxy_covers_native_windows_exports(self) -> None:
        if not which("objdump"):
            self.skipTest("objdump is unavailable")
        system_path = environ.get("NEURALFX_SYSTEM_DXGI")
        if not system_path:
            self.skipTest("NEURALFX_SYSTEM_DXGI is unavailable")
        proxy_path = Path(__file__).resolve().parents[1] / "build/windows-x64/dxgi.dll"
        if not proxy_path.is_file():
            self.skipTest("Cross-compiled proxy is unavailable")
        system_ordinals, system_names = exports(Path(system_path))
        proxy_ordinals, proxy_names = exports(proxy_path)
        self.assertFalse(system_names - proxy_names, system_names - proxy_names)
        self.assertFalse(system_ordinals - proxy_ordinals, system_ordinals - proxy_ordinals)
