import html, markdown, os, re, shutil, subprocess, sys

root = os.path.dirname(os.path.abspath(__file__))
out_dir = os.path.join(root, "api")
src = open(os.path.join(root, "api_documentation.c")).read()
headers = "".join(open(os.path.join(root, "..", h)).read() for h in ("rcut_core.h", "rcut_draw.h", "rcut_sdl.h"))

def signatures(name):
    out = []
    for m in re.finditer(r"^[^\n;#/{}=]*\b" + name + r"(?:_\w+)?\s*\((?:[^();{]|\([^()]*\))*\)(?=\s*[;{])", headers, re.M):
        out.append(re.sub(r"\b(" + name + r"\w*) \(", r"\1(", re.sub(r"\s+", " ", m.group(0).replace("static inline ", "")), count=1) + ";")
    if not out: sys.exit("no declaration for " + name)
    return "\n".join(out)

def definition(kind, name):
    m = re.search(r"^(typedef\s+)?" + kind + r"\s+" + name + r"\b[^;{]*\{", headers, re.M)
    if m:
        depth, i = 0, m.end() - 1
        while True:
            depth += {"{": 1, "}": -1}.get(headers[i], 0)
            if depth == 0: break
            i += 1
        return headers[m.start():headers.index(";", i) + 1]
    m = re.search(r"\}\s*" + name + r"\s*;", headers)
    if not m: sys.exit("no definition for %s %s" % (kind, name))
    depth, i = 0, m.start()
    while True:
        depth += {"}": 1, "{": -1}.get(headers[i], 0)
        if depth == 0: break
        i -= 1
    start = headers.rindex("typedef", 0, i)
    return headers[start:m.end()]

def defines(name):
    pattern = re.escape(name[:-1]) + r"\w*" if name.endswith("*") else name + r"\b"
    out = re.findall(r"^[ \t]*#define[ \t]+" + pattern + r"(?:[^\n]*\\\n)*[^\n]*", headers, re.M)
    if not out: sys.exit("no define for " + name)
    return "\n".join(l.strip() for l in out)

entries = []
module, section = None, None
lines = src.split("\n")
i = 0
while i < len(lines):
    if not lines[i].startswith(("//!", "//@")):
        i += 1
        continue
    desc, see, params, kind, name = [], [], [], None, None
    start = i
    while i < len(lines) and lines[i].startswith(("//!", "//@")):
        if lines[i].startswith("//! @note"):
            desc += ["", "@note " + lines[i][10:]]
        elif lines[i].startswith("//! @"):
            cmd, *args = lines[i][5:].split()
            if cmd == "see_also": see += args
            elif cmd == "param": params.append((args[0], " ".join(args[1:])))
            else: sys.exit("unknown command @" + cmd)
        elif lines[i].startswith("//!"):
            desc.append(lines[i][4:])
        else:
            k, *args = lines[i][3:].split()
            if k == "module":
                module, section = args[0], None
                start = i + 1
            elif k == "section":
                name = " ".join(args)
                j = i + 1
                if not desc:
                    while j < len(lines) and lines[j].startswith("//!"): j += 1
                    if j >= len(lines) or not lines[j].strip(): desc = [l[4:] for l in lines[i + 1:j]]
                    else: j = i + 1
                elif j < len(lines) and lines[j].strip():
                    sys.exit("line %d: @section must be followed by an empty line" % (i + 1))
                if start > 0 and lines[start - 1].strip() and not lines[start - 1].startswith("//@module"):
                    sys.exit("line %d: @section must be preceded by an empty line" % (i + 1))
                section = dict(module=module, kind="section", name=name, anchor="s-" + re.sub(r"\W+", "-", name), desc=desc)
                entries.append(section)
                i = j
                break
            else:
                kind, name = k, args[0] if args else None
                opaque = args[1:] == ["opaque"]
                i += 1
                break
        i += 1
    if kind is None: continue
    if kind == "example":
        m = re.match(r"static void (\w+)\(", lines[i]) if i < len(lines) else None
        if not m: sys.exit("line %d: @example must be followed by a function" % i)
        if not entries or entries[-1]["kind"] == "section": sys.exit("line %d: @example without entry" % i)
        i += 2
        body, shown = [], True
        while lines[i] != "}":
            shown = shown and lines[i].strip() != "//@example_end"
            if shown: body.append(lines[i][1:] if lines[i].startswith("\t") else lines[i])
            i += 1
        entries[-1]["examples"].append(dict(fn=m.group(1), desc=desc, code="\n".join(body), screenshot=name == "screenshot"))
        continue
    e = dict(module=module, section=section, kind=kind, name=name, desc=desc, see=see, params=params, examples=[])
    if kind == "function":
        e.update(code=signatures(name))
    elif kind == "define":
        e.update(code=defines(name))
    elif kind == "struct" and opaque:
        e.update(code="typedef struct %s %s;" % (name, name))
    elif kind in ("enum", "struct"):
        e.update(code=definition(kind, name))
    else:
        sys.exit("unknown directive @" + kind)
    for p, _ in params:
        if not re.search(r"\b%s\b" % p, e["code"]): sys.exit("%s: no parameter %s" % (e["name"], p))
    entries.append(e)

shots = set(re.findall(r"^\s*DOC\((\w+)", src, re.M))
for e in entries:
    for x in e.get("examples", []):
        if x["screenshot"] != (x["fn"] in shots):
            sys.exit("%s: %s" % (x["fn"], "missing in docs[]" if x["screenshot"] else "in docs[] but not @example screenshot"))

fns = {e["name"] for e in entries if e["kind"] == "function"}
def owner(n): return max((f for f in fns if n == f or n.startswith(f + "_")), key=len)
for e in entries:
    if e["kind"] == "function":
        e["code"] = "\n".join(l for l in e["code"].split("\n") if owner(re.search(r"(\w+)\(", l).group(1)) == e["name"])

anchors, used = {}, set()
for e in entries:
    if e["kind"] == "section": continue
    e["anchor"] = e["name"] if e["name"] not in used else e["kind"] + "-" + e["name"]
    used.add(e["anchor"])
    names = re.findall(r"#define\s+(\w+)" if e["kind"] == "define" else r"(\w+)\(", e["code"])
    if e["kind"] == "enum":
        body = re.sub(r"//[^\n]*|/\*.*?\*/|^[ \t]*#[^\n]*", "", e["code"][e["code"].index("{") + 1:e["code"].rindex("}")], flags=re.S | re.M)
        names = [m.group(1) for m in (re.match(r"\s*([A-Za-z_]\w*)", p) for p in body.split(",")) if m]
    for n in [e["name"]] + names:
        anchors.setdefault(n, e)
def href(t, module):
    return "%s#%s" % ("" if t["module"] == module else t["module"] + ".html", t["anchor"])

def resolve(n):
    t = anchors.get(n) or (n.endswith("s") and anchors.get(n[:-1])) or (n.endswith("es") and anchors.get(n[:-2]))
    if not t: print("unresolved reference: " + n, file=sys.stderr)
    return t

def link(n, module):
    t = resolve(n)
    return "<a href=\"%s\"><code>%s</code></a>" % (href(t, module), n) if t else "<code>%s</code>" % n

keywords = set("""auto break case char const continue default do double else enum extern float for goto if inline int long
register restrict return short signed sizeof static struct switch typedef union unsigned void volatile while bool true false NULL""".split())
tokens = re.compile(r"(?P<c>//[^\n]*|/\*.*?\*/)|(?P<s>\"(?:\\.|[^\"\n])*\"|'(?:\\.|[^'\n])*')|(?P<p>^[ \t]*#\w+)|(?P<n>\b\d[\w.]*)|(?P<w>\b[A-Za-z_]\w*\b)", re.S | re.M)

def code(src, module, self=None, cls=None):
    esc = lambda s: html.escape(s, False)
    out, pos = [], 0
    for m in tokens.finditer(src):
        out.append(esc(src[pos:m.start()]))
        pos = m.end()
        k, v = m.lastgroup, m.group(0)
        if k == "w":
            t = anchors.get(v)
            if t and t is not self:
                out.append("<a class=\"%s\" href=\"%s\">%s</a>" % ("f" if t["kind"] == "function" else "e" if t["kind"] == "define" or v != t["name"] else "t", href(t, module), v))
                continue
            k = "k" if v in keywords else "t" if v.endswith("_t") else "e" if re.fullmatch(r"[A-Z][A-Z0-9_]+", v) else None
        out.append("<span class=\"%s\">%s</span>" % (k, esc(v)) if k else esc(v))
    out.append(esc(src[pos:]))
    return "<pre%s>%s</pre>" % (" class=\"%s\"" % cls if cls else "", "".join(out))

def text(t, module, inline=False):
    def ref(m):
        t = resolve(m.group(1))
        return "[`%s`](%s)" % (m.group(1), href(t, module)) if t else "`%s`" % m.group(1)
    def placeholder(m):
        if m.group(1): return m.group(1)
        print("placeholder: " + m.group(2), file=sys.stderr)
        return "<span class=\"todo\">&lt;%s&gt;</span>" % html.escape(m.group(2))
    t = re.sub(r"(```.*?```|`[^`\n]*`)|<([^<>\n]+)>", placeholder, t, flags=re.S)
    out = markdown.markdown(re.sub(r"@ref (\w+)", ref, t), extensions=["tables", "fenced_code"])
    out = re.sub(r"<p>@note (.*?)</p>", r'<div class="note"><strong>Note:</strong> \1</div>', out, flags=re.S)
    out = re.sub(r"<pre><code[^>]*>(.*?)</code></pre>", lambda m: code(html.unescape(m.group(1)), module), out, flags=re.S)
    return re.sub(r"^<p>(.*)</p>$", r"\1", out, flags=re.S) if inline else out + "\n"

style = """<meta http-equiv="X-UA-Compatible" content="IE=edge">
<meta charset="utf-8">
<link rel="stylesheet" href="style.css">"""

def page(title, body):
    return "<!DOCTYPE html>\n<html><head>\n<title>%s</title>\n%s\n</head><body>\n<h1>%s</h1>\n%s</body></html>\n" % (title, style, title, body)

modules = {}
for e in entries:
    mod = e["module"]
    if e["kind"] == "section":
        modules.setdefault(mod, []).append("<h2><a name=\"%s\"></a>%s</h2>\n%s" % (e["anchor"], e["name"], text("\n".join(e["desc"]), mod)))
        continue
    out = "<h3><a name=\"%s\"></a>%s</h3>\n" % (e["anchor"], e["name"])
    out += code(e["code"], mod, e, "sig") + "\n" + text("\n".join(e["desc"]), mod)
    if e["params"]:
        out += "<table class=\"params\"><tr><th>Parameter</th><th></th></tr>\n"
        out += "".join("<tr><td><code>%s</code></td><td>%s</td></tr>\n" % (p, text(d, mod, True)) for p, d in e["params"]) + "</table>\n"
    for x in e["examples"]:
        shot = "<img class=\"shot\" src=\"img/%s.png\">" % x["fn"] if x["screenshot"] else ""
        out += "<div class=\"example\">%s%s</div>\n" % (code(x["code"], mod), shot)
        if x["desc"]: out += "<div class=\"caption\">%s</div>\n" % text("\n".join(x["desc"]), mod)
    if e["see"]:
        out += "<p class=\"see\">See also: %s</p>\n" % ", ".join(link(n, mod) for n in e["see"])
    modules.setdefault(mod, []).append(out)

def sitemap_item(name, local):
    return "<LI><OBJECT type=\"text/sitemap\"><param name=\"Name\" value=\"%s\"><param name=\"Local\" value=\"%s\"></OBJECT>\n" % (name, local)

def sitemap(body):
    return "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\n<HTML><HEAD></HEAD><BODY>\n<UL>\n%s</UL>\n</BODY></HTML>\n" % body

os.makedirs(out_dir, exist_ok=True)
files = ["index.html", "style.css"]
for mod, out in modules.items():
    files.append(mod + ".html")
    open(os.path.join(out_dir, mod + ".html"), "w").write(page(mod, "\n".join(out)))
open(os.path.join(out_dir, "index.html"), "w").write(page("rcut API", "<ul class=\"index\">\n%s</ul>\n" % "".join("<li><a href=\"%s.html\">%s</a></li>\n" % (m, m) for m in modules)))
files += ["img/" + f for f in sorted(os.listdir(os.path.join(out_dir, "img")))]

toc = sitemap_item("rcut API", "index.html")
for mod in modules:
    toc += sitemap_item(mod, mod + ".html") + "<UL>\n"
    nested = False
    for e in entries:
        if e["module"] != mod: continue
        if e["kind"] == "section" and nested: toc += "</UL>\n"
        toc += sitemap_item(e["name"], "%s.html#%s" % (mod, e["anchor"]))
        if e["kind"] == "section":
            toc += "<UL>\n"
            nested = True
    toc += "</UL>\n" * (2 if nested else 1)
open(os.path.join(out_dir, "rcut.hhc"), "w").write(sitemap(toc))
open(os.path.join(out_dir, "rcut.hhk"), "w").write(sitemap("".join(sitemap_item(n, "%s.html#%s" % (e["module"], e["anchor"])) for n, e in sorted(anchors.items()))))

open(os.path.join(out_dir, "rcut.hhp"), "w").write("""[OPTIONS]
Compiled file=rcut.chm
Contents file=rcut.hhc
Index file=rcut.hhk
Default topic=index.html
Title=rcut API
Language=0x409 English (United States)
Full-text search=Yes
Binary Index=Yes

[FILES]
%s
""" % "\n".join(files))

if shutil.which("chmcmd"):
    subprocess.run(["chmcmd", "rcut.hhp"], cwd=out_dir, check=True)
