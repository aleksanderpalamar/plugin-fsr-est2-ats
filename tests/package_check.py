from pathlib import Path
from re import fullmatch, search
from subprocess import check_output
from sys import argv, executable
from zipfile import ZipFile


LUT_PATH = "NeuralFX/luts/neuralfx-cool.cube"


def normalized_text(content: bytes) -> str:
    return content.decode("utf-8").replace("\r\n", "\n").replace("\r", "\n")


def expected_files() -> set[str]:
    files = {"dxgi.dll", "NeuralFX/neuralfx.ini", "README", LUT_PATH}
    files.update(f"NeuralFX/shaders/{path.name}" for path in Path("shaders").glob("*.hlsl"))
    return files


def package_path(arguments: list[str]) -> Path:
    if len(arguments) == 2:
        return Path(arguments[1])
    if len(arguments) != 1:
        raise SystemExit("Usage: package_check.py [package.zip]")

    cmake = Path("CMakeLists.txt").read_text(encoding="utf-8")
    version = search(r"\bproject\s*\(\s*NeuralFX_ETS2\s+VERSION\s+(\d+\.\d+\.\d+)\b", cmake)
    if not version:
        raise SystemExit("Could not find the project version in CMakeLists.txt")
    package = Path("dist") / f"FSR-ets2-ats-{version.group(1)}.zip"
    if not package.is_file():
        raise SystemExit(f"Current version package not found: {package}")
    return package


def check_notices(readme: str) -> None:
    if "Português" not in readme or "English" not in readme:
        raise SystemExit("Package README must include Portuguese and English")
    for notice in ("LICENSE", "THIRD_PARTY_NOTICES.md"):
        if normalized_text(Path(notice).read_bytes()).strip() not in readme:
            raise SystemExit(f"Package README is missing {notice} text")


def check_lut(package: ZipFile, names: set[str]) -> None:
    lut_files = {name for name in names if name.endswith(".cube")}
    if lut_files != {LUT_PATH}:
        raise SystemExit(f"Unexpected package LUTs: {sorted(lut_files)}")
    generated = normalized_text(check_output([executable, "scripts/generate_lut.py"]))
    if normalized_text(Path("luts/neuralfx-cool.cube").read_bytes()) != generated:
        raise SystemExit("The project LUT does not match its generator")
    if normalized_text(package.read(LUT_PATH)) != generated:
        raise SystemExit("The package LUT does not match its generator")


def check_version(path: Path, readme: str) -> None:
    version = fullmatch(r"FSR-ets2-ats-(\d+\.\d+\.\d+)\.zip", path.name)
    if not version:
        return
    if not readme.startswith(f"FSR for ETS2 and ATS {version.group(1)} (Windows x64)"):
        raise SystemExit("Package README version differs from archive version")


def check_package(path: Path) -> None:
    with ZipFile(path) as package:
        names = set(package.namelist())
        missing = expected_files() - names
        if missing:
            raise SystemExit(f"Missing package files: {sorted(missing)}")

        excluded = names & {"LICENSE", "THIRD_PARTY_NOTICES.md", "LEIA-ME.txt"}
        if excluded:
            raise SystemExit(f"Unexpected package files: {sorted(excluded)}")

        damaged = package.testzip()
        if damaged:
            raise SystemExit(f"Damaged package file: {damaged}")

        config = normalized_text(package.read("NeuralFX/neuralfx.ini"))
        if "mode=raytracing" not in config.splitlines():
            raise SystemExit("Package must start in raytracing mode")
        readme = normalized_text(package.read("README"))
        check_notices(readme)
        check_version(path, readme)
        check_lut(package, names)

    print(f"Package verified: {path}")


if __name__ == "__main__":
    check_package(package_path(argv))
