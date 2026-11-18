"""dnf/zypper repository metadata (repodata/) for RPMs, in plain Python: the same
primary.xml, filelists.xml, other.xml and repomd.xml createrepo_c writes, read from each
package's own header. No GLib or librpm, so it runs anywhere uv does (NixOS included).

  write_repodata(repodata_dir, [(rpm_path, href)], location_base)
"""
import gzip
import hashlib
import os
import struct
import time
from xml.sax.saxutils import escape, quoteattr

# Header tags (rpm's rpmtag.h).
NAME, VERSION, RELEASE, EPOCH, SUMMARY, DESCRIPTION = 1000, 1001, 1002, 1003, 1004, 1005
BUILDTIME, BUILDHOST, SIZE, VENDOR, LICENSE, PACKAGER, GROUP = 1006, 1007, 1009, 1011, 1014, 1015, 1016
URL, ARCH, FILEMODES, FILEFLAGS, SOURCERPM, ARCHIVESIZE = 1020, 1022, 1030, 1037, 1044, 1046
PROVIDENAME, REQUIREFLAGS, REQUIRENAME, REQUIREVERSION = 1047, 1048, 1049, 1050
CONFLICTFLAGS, CONFLICTNAME, CONFLICTVERSION = 1053, 1054, 1055
OBSOLETENAME, PROVIDEFLAGS, PROVIDEVERSION, OBSOLETEFLAGS, OBSOLETEVERSION = 1090, 1112, 1113, 1114, 1115
DIRINDEXES, BASENAMES, DIRNAMES = 1116, 1117, 1118
SIG_PAYLOADSIZE = 1007  # in the signature header

RPMFILE_GHOST = 1 << 6
SENSE_LESS, SENSE_GREATER, SENSE_EQUAL = 2, 4, 8
SENSE_PREREQ, SENSE_SCRIPT_PRE, SENSE_SCRIPT_POST = 1 << 6, 1 << 9, 1 << 10


def _header(f):
    """One header structure at f's position: {tag: value}, plus its byte length."""
    start = f.tell()
    magic = f.read(8)
    assert magic[:3] == b"\x8e\xad\xe8", "not an rpm header"
    n, size = struct.unpack(">II", f.read(8))
    index = [struct.unpack(">iiii", f.read(16)) for _ in range(n)]
    data = f.read(size)
    tags = {}
    for tag, typ, off, count in index:
        if typ == 6 or typ == 9 and count == 1:  # STRING, I18NSTRING (first)
            tags[tag] = data[off:data.index(b"\0", off)].decode("utf-8", "replace")
        elif typ in (8, 9):  # STRING_ARRAY
            out, p = [], off
            for _ in range(count):
                e = data.index(b"\0", p)
                out.append(data[p:e].decode("utf-8", "replace"))
                p = e + 1
            tags[tag] = out
        elif typ in (2, 3, 4, 5):  # INT8/16/32/64
            fmt = {2: "b", 3: "h", 4: "i", 5: "q"}[typ]
            width = struct.calcsize(fmt)
            tags[tag] = list(struct.unpack(f">{count}{fmt}", data[off:off + width * count]))
        elif typ == 7:  # BIN
            tags[tag] = data[off:off + count]
    return tags, f.tell() - start


def read_rpm(path):
    with open(path, "rb") as f:
        assert f.read(4) == b"\xed\xab\xee\xdb", f"{path} isn't an rpm"
        f.seek(96)  # the lead
        sig, sig_len = _header(f)
        f.seek(96 + sig_len + (-sig_len % 8))  # the signature header is 8-byte aligned
        start = f.tell()
        tags, length = _header(f)
    tags["_header_range"] = (start, start + length)
    tags["_payload"] = (sig.get(SIG_PAYLOADSIZE) or [0])[0]
    return tags


def _one(tags, tag, default=""):
    """A tag's single value: a string, or the first of a number array."""
    v = tags.get(tag, default)
    return v[0] if isinstance(v, list) and v and not isinstance(v[0], str) else v


def _num(tags, tag):
    v = tags.get(tag)
    return v[0] if isinstance(v, list) and v else 0


def _evr(s):
    """"1:2.3-4" -> (epoch, version, release)."""
    epoch, _, rest = s.rpartition(":") if ":" in s else ("", "", s)
    ver, _, rel = rest.partition("-")
    return epoch, ver, rel


def _deps(tags, names, flags, versions, requires=False):
    out = []
    for name, flag, ver in zip(tags.get(names, []), tags.get(flags, []), tags.get(versions, [])):
        if name.startswith("rpmlib("):
            continue
        attrs = f"name={quoteattr(name)}"
        cmp = flag & (SENSE_LESS | SENSE_GREATER | SENSE_EQUAL)
        if ver and cmp:
            op = {SENSE_EQUAL: "EQ", SENSE_LESS: "LT", SENSE_GREATER: "GT",
                  SENSE_LESS | SENSE_EQUAL: "LE", SENSE_GREATER | SENSE_EQUAL: "GE"}[cmp]
            e, v, r = _evr(ver)
            attrs += f' flags="{op}" epoch="{e or 0}" ver={quoteattr(v)}'
            if r:
                attrs += f" rel={quoteattr(r)}"
        if requires and flag & (SENSE_PREREQ | SENSE_SCRIPT_PRE | SENSE_SCRIPT_POST):
            attrs += ' pre="1"'
        out.append(f"<rpm:entry {attrs}/>")
    return out


def _files(tags):
    """(path, type) for every file: type "dir", "ghost" or ""."""
    dirs, bases, idx = tags.get(DIRNAMES, []), tags.get(BASENAMES, []), tags.get(DIRINDEXES, [])
    modes, flags = tags.get(FILEMODES, []), tags.get(FILEFLAGS, [])
    out = []
    for i, base in enumerate(bases):
        path = dirs[idx[i]] + base
        mode = modes[i] & 0xFFFF if i < len(modes) else 0
        kind = "dir" if (mode & 0o170000) == 0o040000 else "ghost" if i < len(flags) and flags[i] & RPMFILE_GHOST else ""
        out.append((path, kind))
    return out


def _file_xml(path, kind):
    return f'<file type="{kind}">{escape(path)}</file>' if kind else f"<file>{escape(path)}</file>"


def _primary_file(path):
    # What createrepo_c lists in primary.xml (the rest is in filelists.xml).
    return path.startswith("/etc/") or "bin/" in path or path == "/usr/lib/sendmail"


def write_repodata(repodata, packages, location_base):
    os.makedirs(repodata, exist_ok=True)
    primary, filelists, other = [], [], []
    for path, href in packages:
        t = read_rpm(path)
        pkgid = hashlib.sha256(open(path, "rb").read()).hexdigest()
        name, arch = _one(t, NAME), _one(t, ARCH)
        epoch = _num(t, EPOCH)
        ver = f'<version epoch="{epoch or 0}" ver={quoteattr(_one(t, VERSION))} rel={quoteattr(_one(t, RELEASE))}/>'
        files = _files(t)
        start, end = t["_header_range"]
        fmt = [
            f"<rpm:license>{escape(_one(t, LICENSE))}</rpm:license>",
            f"<rpm:vendor>{escape(_one(t, VENDOR))}</rpm:vendor>",
            f"<rpm:group>{escape(_one(t, GROUP))}</rpm:group>",
            f"<rpm:buildhost>{escape(_one(t, BUILDHOST))}</rpm:buildhost>",
            f"<rpm:sourcerpm>{escape(_one(t, SOURCERPM))}</rpm:sourcerpm>",
            f'<rpm:header-range start="{start}" end="{end}"/>',
        ]
        for tag, (names, flags, versions), req in (
                ("provides", (PROVIDENAME, PROVIDEFLAGS, PROVIDEVERSION), False),
                ("requires", (REQUIRENAME, REQUIREFLAGS, REQUIREVERSION), True),
                ("conflicts", (CONFLICTNAME, CONFLICTFLAGS, CONFLICTVERSION), False),
                ("obsoletes", (OBSOLETENAME, OBSOLETEFLAGS, OBSOLETEVERSION), False)):
            entries = _deps(t, names, flags, versions, req)
            if entries:
                fmt.append(f"<rpm:{tag}>" + "".join(entries) + f"</rpm:{tag}>")
        fmt += [_file_xml(p, k) for p, k in files if _primary_file(p)]
        build = _num(t, BUILDTIME)
        installed = _num(t, SIZE)
        archive = _num(t, ARCHIVESIZE) or t["_payload"]
        primary.append(
            f'<package type="rpm"><name>{escape(name)}</name><arch>{escape(arch)}</arch>{ver}'
            f'<checksum type="sha256" pkgid="YES">{pkgid}</checksum>'
            f"<summary>{escape(_one(t, SUMMARY))}</summary>"
            f"<description>{escape(_one(t, DESCRIPTION))}</description>"
            f"<packager>{escape(_one(t, PACKAGER))}</packager><url>{escape(_one(t, URL))}</url>"
            f'<time file="{int(os.path.getmtime(path))}" build="{build}"/>'
            f'<size package="{os.path.getsize(path)}" installed="{installed}" archive="{archive}"/>'
            f'<location xml:base={quoteattr(location_base)} href={quoteattr(href)}/>'
            f"<format>{''.join(fmt)}</format></package>")
        head = f'pkgid="{pkgid}" name={quoteattr(name)} arch={quoteattr(arch)}'
        filelists.append(f"<package {head}>{ver}" + "".join(_file_xml(p, k) for p, k in files) + "</package>")
        other.append(f"<package {head}>{ver}</package>")

    n = len(packages)
    docs = {
        "primary": '<?xml version="1.0" encoding="UTF-8"?>\n<metadata xmlns="http://linux.duke.edu/metadata/common" '
                   f'xmlns:rpm="http://linux.duke.edu/metadata/rpm" packages="{n}">\n' + "\n".join(primary) + "\n</metadata>\n",
        "filelists": '<?xml version="1.0" encoding="UTF-8"?>\n<filelists xmlns="http://linux.duke.edu/metadata/filelists" '
                     f'packages="{n}">\n' + "\n".join(filelists) + "\n</filelists>\n",
        "other": '<?xml version="1.0" encoding="UTF-8"?>\n<otherdata xmlns="http://linux.duke.edu/metadata/other" '
                 f'packages="{n}">\n' + "\n".join(other) + "\n</otherdata>\n",
    }
    now = int(time.time())
    records = []
    for kind, text in docs.items():
        raw = text.encode()
        packed = gzip.compress(raw, mtime=0)
        digest = hashlib.sha256(packed).hexdigest()
        name = f"{digest}-{kind}.xml.gz"  # named by checksum, so caches never mix two releases
        open(os.path.join(repodata, name), "wb").write(packed)
        records.append(
            f'<data type="{kind}"><checksum type="sha256">{digest}</checksum>'
            f'<open-checksum type="sha256">{hashlib.sha256(raw).hexdigest()}</open-checksum>'
            f'<location href="repodata/{name}"/><timestamp>{now}</timestamp>'
            f"<size>{len(packed)}</size><open-size>{len(raw)}</open-size></data>")
    open(os.path.join(repodata, "repomd.xml"), "w").write(
        '<?xml version="1.0" encoding="UTF-8"?>\n<repomd xmlns="http://linux.duke.edu/metadata/repo" '
        'xmlns:rpm="http://linux.duke.edu/metadata/rpm">\n'
        f"<revision>{now}</revision>\n" + "\n".join(records) + "\n</repomd>\n")
