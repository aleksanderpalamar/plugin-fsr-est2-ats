from pathlib import Path
from sys import argv
from zipfile import ZipFile


def expected_files() -> set[str]:
    files = {"dxgi.dll", "NeuralFX/neuralfx.ini", "README"}
    files.update(f"NeuralFX/shaders/{path.name}" for path in Path("shaders").glob("*.hlsl"))
    files.update(f"NeuralFX/luts/{path.name}" for path in Path("luts").glob("*.cube"))
    return files


def package_path(arguments: list[str]) -> Path:
    if len(arguments) == 2:
        return Path(arguments[1])
    if len(arguments) != 1:
        raise SystemExit("Usage: package_check.py [package.zip]")

    packages = list(Path("dist").glob("FSR-ets2-ats-*.zip"))
    if len(packages) != 1:
        raise SystemExit(f"Expected one package in dist, found {len(packages)}")
    return packages[0]


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

        readme = package.read("README").decode("utf-8")
        if "Português" not in readme or "English" not in readme:
            raise SystemExit("Package README must include Portuguese and English")

    print(f"Package verified: {path}")


if __name__ == "__main__":
    check_package(package_path(argv))
