const fs = require('fs')
const path = require('path')

const platform = process.platform

// implementing it b/c we don't want to depend on fs.copyFileSync
// which appears only in node@8.x
function copyFileSync(sourcePath, destPath) {
    const data = fs.readFileSync(sourcePath)
    const stat = fs.statSync(sourcePath)
    fs.writeFileSync(destPath, data)
    fs.chmodSync(destPath, stat.mode)
}

const copyPlatformBinaries = (platformName) => {
    const sourcePath = path.join(__dirname, 'bins', `ppx-${platformName}`)
    const destPath = path.join(__dirname, 'ppx')

    if (fs.existsSync(destPath)) {
        fs.unlinkSync(destPath)
    }
    copyFileSync(sourcePath, destPath)
    fs.chmodSync(destPath, 0777)
}

switch (platform) {
    case 'win32':
        copyPlatformBinaries('windows')
        break
    case 'linux':
        copyPlatformBinaries('linux')
    case 'darwin':
        copyPlatformBinaries('macos')
        break
    default:
        console.warn(
            'error: no release built for the ' + platform + ' platform',
        )
        process.exit(1)
}
