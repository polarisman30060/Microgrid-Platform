import csv
import os
import subprocess


Import("env")

project_dir = env.subst("$PROJECT_DIR")
partition_table = os.path.join(project_dir, "partitions.csv")
web_offset = None

with open(partition_table, newline="", encoding="utf-8-sig") as file:
    for row in csv.reader(file):
        if len(row) >= 5 and row[0].strip() == "web_a":
            if row[1].strip() != "data" or row[2].strip() != "spiffs":
                raise RuntimeError("web_a must be a SPIFFS data partition")
            web_offset = row[3].strip()
            break

if not web_offset:
    raise RuntimeError("web_a SPIFFS partition is missing an explicit offset")

build_dir = env.subst("$BUILD_DIR")
web_image = os.path.join(build_dir, "web_a.bin")
web_source = os.path.join(project_dir, "web", "index.html")


def build_web_image(target, source, env, **kwargs):
    ninja = env.WhereIs("ninja")
    if not ninja:
        ninja_package = env.PioPlatform().get_package_dir("tool-ninja")
        if ninja_package:
            ninja = os.path.join(ninja_package, "ninja.exe")
    if not ninja:
        raise RuntimeError("Could not find Ninja to generate the web filesystem image")
    subprocess.check_call(
        [ninja, "-C", build_dir, "spiffs_web_a_bin"],
        cwd=project_dir,
    )
    return 0


image_node = env.Command(
    web_image,
    [partition_table, web_source],
    build_web_image,
)
firmware_image = os.path.join(build_dir, env.subst("$PROGNAME") + ".bin")
env.Depends(firmware_image, image_node)

if env.subst("$UPLOAD_PROTOCOL") == "esptool":
    env.Append(UPLOADERFLAGS=[web_offset, web_image])
