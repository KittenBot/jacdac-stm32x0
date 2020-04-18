const fs = require("fs")
const child_process = require("child_process")
let verbose = 1
if (process.argv[2] == "-q") {
    process.argv.shift()
    verbose = 0
}
const fn = process.argv[2]
const buf = fs.readFileSync(fn)

const flash_size = parseInt(process.argv[3])
const bl_size = parseInt(process.argv[4])
const profiles_path = process.argv[5]

if (isNaN(bl_size) || !profiles_path) {
    throw "USAGE: node patch-bin.js file.elf flash_size_in_k bootloader_size_in_k profiles_path"
}

let pos = 0

function log(msg) {
    if (verbose)
        console.log(msg)
}

let w0 = buf.readUInt32LE(0)
if (w0 == 0x464c457f) {
    const out = child_process.execSync("arm-none-eabi-objdump -h " + fn, { encoding: "utf-8" })
    const m = /^\s*\d+\s+\.text\s.*\s([0-9a-fA-F]{8})\s+\d\*\*/m.exec(out)
    if (!m) {
        throw ("invalid output: " + out)
    }
    pos = parseInt(m[1], 16)
    log("detected ELF file, text at " + pos.toString(16))
} else {
    log("assuming BIN file")
}

w0 = buf.readUInt32LE(pos)
if ((w0 & 0xff00_0000) == 0x2000_0000) {
    log("app mode")

    const flashBase = 0x800_0000

    const basename = fn.replace(/\.elf$/, "")

    // figure out device class
    const profile_name = basename.replace(/.*\/app-/, "")
    const src = fs.readFileSync(profiles_path + "/" + profile_name + ".c", "utf8")
    const m = /DEVICE_CLASS\((0x3[0-9a-f]+)\)/.exec(src)
    if (!m)
        throw "DEVICE_CLASS(0x3...) missing"
    const dev_class = parseInt(m[1])
    log("device class: " + dev_class.toString(16))

    const reset = buf.readUInt32LE(pos + 4)
    const app_reset = buf.readInt32LE(pos + 13 * 4)
    if (app_reset == 0 || app_reset == -1) {
        buf.writeUInt32LE(reset, pos + 13 * 4)
        log("patching app_reset to " + reset.toString(16))
    }

    const bl_reset_handler = flashBase + (flash_size - bl_size) * 1024 + 8 * 4 + 1
    buf.writeUInt32LE(bl_reset_handler, pos + 4)
    log("setting global reset to " + bl_reset_handler.toString(16))

    buf.fill(0xff, pos + 7 * 4, pos + (7 + 4) * 4)
    buf.writeUInt32LE(dev_class, pos + 8 * 4)
    log("clearing devinfo area")

} else if (w0 == 0x9fddf13b) {
    log("setting random seed")
    require("crypto").randomFillSync(buf, pos + 16, 8)
} else {
    throw "can't detect file type"
}


fs.writeFileSync(fn, buf)
