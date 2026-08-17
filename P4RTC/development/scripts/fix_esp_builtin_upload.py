Import("env")


if env.subst("$UPLOAD_PROTOCOL") == "esp-builtin":
    build_dir = env.subst("$BUILD_DIR").replace("\\", "/")
    firmware = "%s/firmware.bin" % build_dir

    fixed_flags = []
    for flag in env.get("UPLOADERFLAGS", []):
        fixed_flag = str(flag).replace("$SOURCE", firmware).replace("\\", "/")
        if fixed_flag.startswith("program_esp "):
            fixed_flag = fixed_flag.replace("{{", "").replace("}}", "")
        fixed_flags.append(fixed_flag)

    env.Replace(
        UPLOADERFLAGS=fixed_flags,
        UPLOADCMD="$UPLOADER $UPLOADERFLAGS",
    )
