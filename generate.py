#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path


def usage(prog: str) -> None:
    print(
        f"Usage: {prog} <width> <height> <density> <min_house> <max_house> <output.txt>",
        file=sys.stderr,
    )
    print(f"Example: {prog} 80 60 0.4 9 36 city.txt", file=sys.stderr)
    print("Outputs:", file=sys.stderr)
    print("  map_txt/<output.txt>", file=sys.stderr)
    print("  map_preview/<output_base>.jpg", file=sys.stderr)


def run_cmd(cmd: list[str], cwd: Path | None = None) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)


def main() -> int:
    if len(sys.argv) != 7:
        usage(sys.argv[0])
        return 1

    script_dir = Path(__file__).resolve().parent
    txt_dir = script_dir / "map_txt"
    images_dir = script_dir / "map_preview"
    sauce_dir = script_dir / "Sauce"

    txt_dir.mkdir(parents=True, exist_ok=True)
    images_dir.mkdir(parents=True, exist_ok=True)

    mapgen_bin = sauce_dir / "mapgen"
    mapview_bin = sauce_dir / "mapview"
    bmp_to_jpg_bin = sauce_dir / "bmp_to_jpg"

    if not mapgen_bin.exists() or not mapview_bin.exists() or not bmp_to_jpg_bin.exists():
        run_cmd(["make"], cwd=script_dir)

    mapgen_args = sys.argv[1:6]
    requested_output = Path(sys.argv[6]).name
    if not requested_output.endswith(".txt"):
        requested_output = f"{requested_output}.txt"

    map_txt_path = txt_dir / requested_output
    bmp_path = images_dir / f"{map_txt_path.stem}.bmp"
    jpg_path = images_dir / f"{map_txt_path.stem}.jpg"

    try:
        run_cmd([str(mapgen_bin), *mapgen_args, str(map_txt_path)])
        run_cmd([str(mapview_bin), str(map_txt_path), "--export-bmp", str(bmp_path)])
        run_cmd([str(bmp_to_jpg_bin), str(bmp_path), str(jpg_path)])
    except subprocess.CalledProcessError as exc:
        print(f"Error: command failed with exit code {exc.returncode}", file=sys.stderr)
        return exc.returncode

    if bmp_path.exists():
        os.remove(bmp_path)

    print(f"[DONE] JPG created: {jpg_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
