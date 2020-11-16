const fs = require('fs')
const { platform } = require('os')
const path = require('path')

const mainEsyJson = require('../../esy.json')

if (!mainEsyJson.resolutions) mainEsyJson.resolutions = {}

if (platform == 'win32')
    mainEsyJson.resolutions['@opam/conf-libssl'] =
        'esy-packages/esy-openssl#4476291'

if (platform == 'darwin')
    mainEsyJson.resolutions['ocaml'] = {
        source: 'esy-ocaml/ocaml#1805622dc40c1c569a1720078b5707eeba32b07d',
        override: {
            build: [
                './esy-configure -no-cfi -cc "gcc -Wno-implicit-function-declaration" -prefix $cur__install',
                './esy-build',
            ],
        },
    }

const esyJson = JSON.stringify(mainEsyJson, null, 2)

const esyJsonPath = path.join(__dirname, '..', '..', 'esy.json')

fs.unlinkSync(esyJsonPath)

fs.writeFileSync(esyJsonPath, esyJson, {
    encoding: 'utf8',
})
