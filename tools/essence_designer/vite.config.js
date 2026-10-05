import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

// The one file all designer content is saved to. It lives inside the mod's
// folder so it is committed together with the game code.
const here = path.dirname(fileURLToPath(import.meta.url))
const DATA_FILE = path.resolve(here, '../../designer/essence_data.json')

const EMPTY_DATA = { version: 1, tagGroups: [], tags: [], damageTypes: [], essences: [], powers: [] }

// Safety copies. Before the data file is replaced, the version being replaced is copied into a
// "backups" folder beside it, at most once a minute, keeping the newest 300. If something is ever
// lost, an earlier copy can be put back.
const BACKUP_DIR = path.join(path.dirname(DATA_FILE), 'backups')
function backUp() {
  if (!fs.existsSync(DATA_FILE)) return
  fs.mkdirSync(BACKUP_DIR, { recursive: true })
  const existing = fs.readdirSync(BACKUP_DIR).filter((f) => f.endsWith('.json')).sort()
  const stamp = new Date().toISOString().replace(/[:T]/g, '-').slice(0, 16)
  const name = 'essence_data-' + stamp + '.json'
  if (existing.includes(name)) return
  fs.copyFileSync(DATA_FILE, path.join(BACKUP_DIR, name))
  for (const old of existing.slice(0, Math.max(0, existing.length - 299))) fs.unlinkSync(path.join(BACKUP_DIR, old))
}

function readStamp() {
  return fs.existsSync(DATA_FILE) ? String(fs.statSync(DATA_FILE).mtimeMs) : '0'
}

// The "small Node helper": two web addresses the app calls.
//   GET /api/data  -> hands the app the contents of the data file
//   PUT /api/data  -> writes the app's contents back to the data file
// The "stamp" is the file's last-changed time. The app sends back the stamp it
// loaded; if the file has changed since (for example Claude edited it), the
// save is refused so nothing gets overwritten.
function dataFilePlugin() {
  return {
    name: 'essence-data-file',
    configureServer(server) {
      server.middlewares.use('/api/data', (req, res) => {
        const send = (status, body) => {
          res.statusCode = status
          res.setHeader('Content-Type', 'application/json')
          res.end(JSON.stringify(body))
        }
        try {
          if (req.method === 'GET') {
            const data = fs.existsSync(DATA_FILE)
              ? JSON.parse(fs.readFileSync(DATA_FILE, 'utf8'))
              : EMPTY_DATA
            return send(200, { stamp: readStamp(), data: { ...EMPTY_DATA, ...data } })
          }
          if (req.method === 'PUT') {
            let body = ''
            req.on('data', (chunk) => (body += chunk))
            req.on('end', () => {
              try {
                const { stamp, data } = JSON.parse(body)
                if (stamp !== readStamp()) return send(409, { error: 'changed-on-disk' })
                if (!data || !Array.isArray(data.tagGroups) || !Array.isArray(data.tags)) {
                  return send(400, { error: 'bad-data' })
                }
                // Write to a temporary file first, then swap it in, so a crash
                // mid-save can never leave a half-written data file.
                fs.mkdirSync(path.dirname(DATA_FILE), { recursive: true })
                backUp()
                const temp = DATA_FILE + '.tmp'
                fs.writeFileSync(temp, JSON.stringify(data, null, 2) + '\n')
                fs.renameSync(temp, DATA_FILE)
                send(200, { stamp: readStamp() })
              } catch (err) {
                send(500, { error: String(err) })
              }
            })
            return
          }
          send(405, { error: 'method-not-allowed' })
        } catch (err) {
          send(500, { error: String(err) })
        }
      })
    },
  }
}

export default defineConfig({
  plugins: [react(), dataFilePlugin()],
  server: { port: 5734, strictPort: true },
})
