from pathlib import Path
from tempfile import TemporaryDirectory
from unittest import TestCase
from zipfile import ZipFile

from package_check import check_package, package_path


class PackageCheckTests(TestCase):
    def test_accepts_windows_line_endings_in_text_files(self) -> None:
        source_path = package_path(["package_check.py"])
        with TemporaryDirectory() as directory:
            archive_path = Path(directory) / "windows.zip"
            with ZipFile(source_path) as source, ZipFile(archive_path, "w") as archive:
                for entry in source.infolist():
                    content = source.read(entry.filename)
                    if entry.filename in {"README", "NeuralFX/luts/neuralfx-cool.cube"}:
                        content = content.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
                    archive.writestr(entry, content)

            check_package(archive_path)
