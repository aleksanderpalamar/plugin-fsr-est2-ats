from pathlib import Path
from os import chdir, environ, getcwd, name
from shutil import copyfile
from subprocess import run
from tempfile import TemporaryDirectory
from unittest import TestCase, skipIf
from zipfile import ZipFile

from package_check import check_package, package_path


def stub_package_scripts(root: Path) -> Path:
    scripts = root / "scripts"
    scripts.mkdir()
    package_script = scripts / "package.sh"
    copyfile(Path(__file__).resolve().parents[1] / "scripts/package.sh", package_script)
    build_script = scripts / "build.sh"
    build_script.write_text(
        "#!/bin/sh\n"
        "printf '%s\\n' \"${NEURALFX_PACKAGE_VERSION-}\" > \"$PACKAGE_TEST_DIR/version\"\n"
        "printf '%s\\n' \"$@\" > \"$PACKAGE_TEST_DIR/arguments\"\n",
        encoding="utf-8",
    )
    build_script.chmod(0o755)
    return package_script


@skipIf(name == "nt", "Package script requires a POSIX shell")
class PackageScriptTests(TestCase):
    def test_script_passes_requested_version_to_build(self) -> None:
        with TemporaryDirectory() as directory:
            root = Path(directory)
            package_script = stub_package_scripts(root)
            result = run(["bash", str(package_script), "2.0.3"], cwd=root,
                         env={**environ, "PACKAGE_TEST_DIR": str(root)},
                         capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((root / "version").read_text().strip(), "2.0.3")
            self.assertEqual((root / "arguments").read_text().splitlines(),
                             ["--target", "neuralfx_package"])

    def test_script_rejects_invalid_version(self) -> None:
        with TemporaryDirectory() as directory:
            root = Path(directory)
            package_script = stub_package_scripts(root)
            result = run(["bash", str(package_script), "2.0"], cwd=root,
                         env={**environ, "PACKAGE_TEST_DIR": str(root)},
                         capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Usage", result.stderr)

    def test_script_uses_project_version_without_argument(self) -> None:
        with TemporaryDirectory() as directory:
            root = Path(directory)
            package_script = stub_package_scripts(root)
            result = run(["bash", str(package_script)], cwd=root,
                         env={**environ, "PACKAGE_TEST_DIR": str(root),
                              "NEURALFX_PACKAGE_VERSION": "9.9.9"},
                         capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((root / "version").read_text().strip(), "")


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

    def test_rejects_readme_version_different_from_archive(self) -> None:
        source_path = package_path(["package_check.py"])
        with TemporaryDirectory() as directory:
            archive_path = Path(directory) / "FSR-ets2-ats-2.0.3.zip"
            with ZipFile(source_path) as source, ZipFile(archive_path, "w") as archive:
                for entry in source.infolist():
                    archive.writestr(entry, source.read(entry.filename))
            with self.assertRaisesRegex(SystemExit, "version"):
                check_package(archive_path)
