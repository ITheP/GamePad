import hashlib
import os
import shutil
from datetime import datetime

Import("env")


def sha256_file(path):
    sha = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            sha.update(chunk)
    return sha.hexdigest()


def archive_elf(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    progname = env.subst("$PROGNAME")
    project_dir = env.subst("$PROJECT_DIR")
    env_name = env.subst("$PIOENV")

    elf_path = os.path.join(build_dir, f"{progname}.elf")
    if not os.path.isfile(elf_path):
        print(f"[archive_elf] ELF not found: {elf_path}")
        return

    sha = sha256_file(elf_path)

    archive_dir = os.path.join(project_dir, "artifacts", "elf")
    os.makedirs(archive_dir, exist_ok=True)

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    base_name = f"{progname}-{env_name}-{timestamp}-{sha}"

    dest_elf = os.path.join(archive_dir, f"{base_name}.elf")

    shutil.copy2(elf_path, dest_elf)

    print(f"[archive_elf] Archived: {dest_elf}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", archive_elf)
