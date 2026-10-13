from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory
from unittest import TestCase, main
import shutil
import sys


PROJECT = Path(__file__).resolve().parents[1]


class ScreenshotHostAssetsTests(TestCase):
    def command(self, *arguments):
        result = run(arguments, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def fixture(self, root):
        (root / "tests/assets").mkdir(parents=True)
        (root / "shaders").mkdir()
        (root / "shaders/screenshot.hlsl").write_text("shader fixture\n")
        (root / "tests/assets/screenshot_host.ini").write_text("enabled=true\n")
        (root / "tests/assets/screenshot.cube").write_text("LUT_3D_SIZE 2\n")
        (root / "tests/steam_screenshot_host.cpp").write_text("int main() { return 0; }\n")
        (root / "tests/steam_screenshot_host_api.cpp").write_text("int steam_fixture = 0;\n")
        (root / "dxgi.cpp").write_text("int dxgi_fixture = 0;\n")
        module = (PROJECT / "cmake/screenshot_host.cmake").as_posix()
        (root / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.24)\n"
            "project(ScreenshotAssets LANGUAGES CXX)\n"
            "add_library(dxgi SHARED dxgi.cpp)\n"
            'set_target_properties(dxgi PROPERTIES PREFIX "" SUFFIX ".dll")\n'
            f'include("{module}")\n'
        )

    def test_stages_runtime_assets_in_missing_configuration_directory(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            build = root / "build"
            generator = "Visual Studio 17 2022" if sys.platform == "win32" else "Ninja Multi-Config"
            self.command("cmake", "-S", str(root), "-B", str(build), "-G", generator)
            output = build / "screenshot-host/Release"
            shutil.rmtree(output.parent, ignore_errors=True)
            self.command("cmake", "--build", str(build), "--config", "Release",
                         "--target", "neuralfx_screenshot_host_assets")
            self.assertTrue(output.is_dir())
            self.assertEqual((output / "dxgi.dll").read_bytes(),
                             (build / "Release/dxgi.dll").read_bytes())
            for source, destination in (
                ("shaders/screenshot.hlsl", "NeuralFX/shaders/screenshot.hlsl"),
                ("tests/assets/screenshot_host.ini", "NeuralFX/neuralfx.ini"),
                ("tests/assets/screenshot.cube", "NeuralFX/luts/screenshot.cube"),
            ):
                self.assertEqual((output / destination).read_bytes(), (root / source).read_bytes())


if __name__ == "__main__":
    main()
