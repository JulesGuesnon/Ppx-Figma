const fs = require('fs')
const path = require('path')

const mainEsyJson = require('../../esy.json')

mainEsyJson.resolutions['@opam/conf-libssl'] =
    'esy-packages/esy-openssl#4476291'

const esyJson = JSON.stringify(mainEsyJson, null, 2)

const esyJsonPath = path.join(__dirname, '..', '..', 'esy.json')

fs.unlinkSync(esyJsonPath)

fs.writeFileSync(path.join(__dirname, '..', '..', 'esy.json'), esyJson, {
    encoding: 'utf8',
})
