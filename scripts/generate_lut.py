from itertools import product
from sys import stdout


SIZE = 33
SCALE = 1_000_000
WEIGHTS = (2126, 7152, 722)
CHANNEL_SHIFTS = ((-12000, 6000), (2000, 2000), (16000, -6000))


def rounded_div(numerator: int, denominator: int) -> int:
    if numerator >= 0:
        return (numerator + denominator // 2) // denominator
    return -((-numerator + denominator // 2) // denominator)


def level(index: int) -> int:
    return rounded_div(index * SCALE, SIZE - 1)


def grade_channel(value: int, luminance: int, shadow_shift: int, highlight_shift: int) -> int:
    muted = rounded_div(value * 92 + luminance * 8, 100)
    contrast = rounded_div(15 * (2 * muted - SCALE) * muted * (SCALE - muted), 100 * SCALE * SCALE)
    shadow = rounded_div((SCALE - luminance) ** 2, SCALE)
    highlight = rounded_div(luminance ** 2, SCALE)
    shift = rounded_div(shadow * shadow_shift + highlight * highlight_shift, SCALE)
    return max(0, min(SCALE, muted + contrast + shift))


def grade(red: int, green: int, blue: int) -> tuple[int, int, int]:
    channels = (red, green, blue)
    luminance = rounded_div(sum(value * weight for value, weight in zip(channels, WEIGHTS)), 10000)
    return tuple(
        grade_channel(value, luminance, *shifts)
        for value, shifts in zip(channels, CHANNEL_SHIFTS)
    )


def render_lut() -> str:
    lines = [
        'TITLE "NeuralFX Cool"',
        "DOMAIN_MIN 0.0 0.0 0.0",
        "DOMAIN_MAX 1.0 1.0 1.0",
        f"LUT_3D_SIZE {SIZE}",
    ]
    for blue, green, red in product(range(SIZE), repeat=3):
        graded = grade(level(red), level(green), level(blue))
        lines.append(" ".join(f"{channel // SCALE}.{channel % SCALE:06d}" for channel in graded))
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    stdout.write(render_lut())
