const fs = require('fs')
const { platform } = require('os')
const path = require('path')

const mainEsyJson = require('../../esy.json')

if (!mainEsyJson.resolutions) mainEsyJson.resolutions = {}

if (platform == 'win32')
    mainEsyJson.resolutions['@opam/conf-libssl'] =
        'esy-packages/esy-openssl#4476291'

if (platform == 'darwin') mainEsyJson.dependencies.ocaml = '4.6.1001'

const esyJson = JSON.stringify(mainEsyJson, null, 2)

const esyJsonPath = path.join(__dirname, '..', '..', 'esy.json')

fs.unlinkSync(esyJsonPath)

fs.writeFileSync(esyJsonPath, esyJson, {
    encoding: 'utf8',
})
