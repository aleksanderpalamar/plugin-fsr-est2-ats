from os import environ
from pathlib import Path
from struct import unpack_from
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


def uint16(data: bytes, offset: int) -> int:
    return unpack_from("<H", data, offset)[0]


def uint32(data: bytes, offset: int) -> int:
    return unpack_from("<I", data, offset)[0]


def sections(data: bytes, header: int) -> list[tuple[int, int, int]]:
    count = uint16(data, header + 6)
    optional_size = uint16(data, header + 20)
    first = header + 24 + optional_size
    result = []
    for index in range(count):
        offset = first + index * 40
        size = max(uint32(data, offset + 8), uint32(data, offset + 16))
        result.append((uint32(data, offset + 12), size, uint32(data, offset + 20)))
    return result


def file_offset(rva: int, entries: list[tuple[int, int, int]]) -> int:
    for address, size, raw in entries:
        if address <= rva < address + size:
            return raw + rva - address
    raise ValueError(f"RVA is outside PE sections: {rva:#x}")


def export_directory(data: bytes) -> tuple[int, list[tuple[int, int, int]]]:
    if data[:2] != b"MZ":
        raise ValueError("Missing PE DOS signature")
    header = uint32(data, 0x3c)
    if data[header:header + 4] != b"PE\0\0":
        raise ValueError("Missing PE signature")
    optional = header + 24
    magic = uint16(data, optional)
    if magic not in (0x10b, 0x20b):
        raise ValueError(f"Unsupported PE optional header: {magic:#x}")
    directory = optional + (112 if magic == 0x20b else 96)
    rva = uint32(data, directory)
    if rva == 0:
        raise ValueError("PE has no export directory")
    entries = sections(data, header)
    return file_offset(rva, entries), entries


def exports(path: Path) -> tuple[set[int], set[str]]:
    data = path.read_bytes()
    table, entries = export_directory(data)
    base = uint32(data, table + 16)
    function_count = uint32(data, table + 20)
    name_count = uint32(data, table + 24)
    functions = file_offset(uint32(data, table + 28), entries)
    names_table = file_offset(uint32(data, table + 32), entries)
    ordinals = {
        base + index for index in range(function_count)
        if uint32(data, functions + index * 4) != 0
    }
    names = set()
    for index in range(name_count):
        name_rva = uint32(data, names_table + index * 4)
        start = file_offset(name_rva, entries)
        end = data.index(b"\0", start)
        names.add(data[start:end].decode("ascii"))
    return ordinals, names


class DxgiExportTests(TestCase):
    def test_proxy_covers_observed_windows_11_exports(self) -> None:
        proxy_path = Path(__file__).resolve().parents[1] / "build/windows-x64/dxgi.dll"
        if not proxy_path.is_file():
            self.skipTest("Cross-compiled proxy is unavailable")
        proxy_ordinals, proxy_names = exports(proxy_path)
        self.assertFalse(WINDOWS_11_NAMES - proxy_names, WINDOWS_11_NAMES - proxy_names)
        self.assertFalse(set(range(1, 21)) - proxy_ordinals,
                         set(range(1, 21)) - proxy_ordinals)

    def test_proxy_covers_native_windows_exports(self) -> None:
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
