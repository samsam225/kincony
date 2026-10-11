Import("env")
from pathlib import Path
root = Path(env.subst("$PROJECT_DIR"))
html = (root / "web/index.html").read_text(encoding="utf-8")
js = (root / "web/app.js").read_text(encoding="utf-8")
html = html.replace('<script src="/app.js"></script>', '<script>' + js + '</script>')
config = (root / "config/example.json").read_text(encoding="utf-8")
(root / "include/generated.h").write_text('#pragma once\nstatic const char PAGE[] PROGMEM = R"AVUI(' + html + ')AVUI";\nstatic const char DEFAULT_CONFIG[] = R"AVCFG(' + config + ')AVCFG";\n', encoding="utf-8")
