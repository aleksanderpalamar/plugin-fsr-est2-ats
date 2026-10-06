from pathlib import Path
from os import chdir, getcwd
from tempfile import TemporaryDirectory
from unittest import TestCase
from zipfile import ZipFile

from package_check import check_package, package_path


class PackageCheckTests(TestCase):
    def test_selects_current_version_with_older_package_present(self) -> None:
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "CMakeLists.txt").write_text(
                "project(NeuralFX_ETS2 VERSION 2.0.0 LANGUAGES CXX)\n",
                encoding="utf-8",
            )
            packages = root / "dist"
            packages.mkdir()
            (packages / "FSR-ets2-ats-1.0.0.zip").touch()
            current = packages / "FSR-ets2-ats-2.0.0.zip"
            current.touch()
            previous = getcwd()
            try:
                chdir(root)
                self.assertEqual(package_path(["package_check.py"]), Path("dist") / current.name)
            finally:
                chdir(previous)

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
