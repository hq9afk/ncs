#!/usr/bin/env python3
"""Generate embedded_resources.cpp from shaders/, keyed by path relative to shaders/.

Usage: embed_resources.py <project_source_root> <output_cpp_path>
"""

import sys
from pathlib import Path


def escape(data: bytes) -> str:
    out = []
    for b in data:
        c = chr(b)
        if c == '"':
            out.append('\\"')
        elif c == '\\':
            out.append('\\\\')
        elif c == '\n':
            out.append('\\n')
        elif c == '\r':
            out.append('\\r')
        elif c == '\t':
            out.append('\\t')
        elif 32 <= b < 127:
            out.append(c)
        else:
            out.append('\\%03o' % b)
    return ''.join(out)


def _demo() -> None:
    """Round-trip check: escape() output, read back as a C++ string literal, must equal the input."""
    import re

    def cpp_decode(s: str) -> bytes:
        out = bytearray()
        i = 0
        while i < len(s):
            c = s[i]
            if c == '\\':
                nxt = s[i + 1]
                if nxt in ('n', 'r', 't', '"', '\\'):
                    out += {'n': b'\n', 'r': b'\r', 't': b'\t', '"': b'"', '\\': b'\\'}[nxt]
                    i += 2
                else:
                    octal = re.match(r'[0-7]{1,3}', s[i + 1:i + 4])
                    out.append(int(octal.group(), 8))
                    i += 1 + len(octal.group())
            else:
                out += c.encode('latin-1')
                i += 1
        return bytes(out)

    for sample in (b'plain', b'"quote"', b'back\\slash', b'new\nline\ttab', bytes([0, 1, 255, 127])):
        assert cpp_decode(escape(sample)) == sample, sample


def main() -> None:
    _demo()
    root = Path(sys.argv[1])
    out_path = Path(sys.argv[2])

    entries = []

    shaders_dir = root / "shaders"
    for path in sorted(shaders_dir.rglob("*")):
        if path.is_file():
            entries.append((str(path.relative_to(shaders_dir)), path))

    lines = [
        '#include "embedded_resources.h"',
        '',
        'const std::map<std::string, std::string> EmbeddedResources::files = {',
    ]
    for key, path in entries:
        lines.append('    {"%s", "%s"},' % (key, escape(path.read_bytes())))
    lines.append('};')
    lines.append('')

    out_path.write_text('\n'.join(lines))


if __name__ == "__main__":
    main()
