Import("env") # type: ignore
import os

PROJECT_DIR = env.get("PROJECT_DIR") # type: ignore
SRC_DIR = os.path.join(PROJECT_DIR, "core_apps")
OUT_FILE = os.path.join(PROJECT_DIR, "core_apps", "generated", "core_apps_bundle.h")

def sanitize(name):
    return name.replace(".", "_").replace("-", "_")

def generate():
    os.makedirs(os.path.dirname(OUT_FILE), exist_ok=True)

    entries = []
    with open(OUT_FILE, "w") as out:
        out.write("#pragma once\n")
        out.write("// AUTO-GENERATED — do not edit by hand.\n\n")

        for filename in sorted(os.listdir(SRC_DIR)):
            if not filename.endswith(".lua"):
                continue
            path = os.path.join(SRC_DIR, filename)
            varname = "script_" + sanitize(filename[:-4])

            with open(path, "r") as f:
                content = f.read()

            out.write(f'const char* {varname} = R"LUASRC(\n{content}\n)LUASRC";\n\n')
            entries.append(varname)

    print("=== GENERATED CORE APPS: " + ", ".join(entries) + " ===")

generate()