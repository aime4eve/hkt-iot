'use strict';
/** HKT 负载解码器管理调试平台 — Fastify 服务
 *  平台专属数据目录（DATA_DIR）是解码器唯一权威存储：检索/测试/回归/对比 + 管理（录入/更新/导入导出）。
 */
const path = require('path');
const fs = require('fs');
const Fastify = require('fastify');
const fastifyStatic = require('@fastify/static');
const multipart = require('@fastify/multipart');
const AdmZip = require('adm-zip');

const repo = require('./lib/repo');
const { runDecode } = require('./lib/sandbox');
const { parsePayload, stableStringify, deepEqual, judgeResult } = require('./lib/payload');

const PORT = Number(process.env.PORT || 8620);
const DEFAULT_TIMEOUT = Number(process.env.DECODE_TIMEOUT_MS || 3000);

const app = Fastify({ bodyLimit: 64 * 1024 * 1024, logger: false });
app.register(fastifyStatic, {
  root: path.join(__dirname, 'web'),
  prefix: '/',
  setHeaders(res, absPath) {
    // 前端发版后浏览器必须拿新文件：HTML 不缓存，JS/CSS 每次复核
    if (absPath.endsWith('.html')) res.setHeader('Cache-Control', 'no-store');
    else if (absPath.endsWith('.js') || absPath.endsWith('.css')) res.setHeader('Cache-Control', 'no-cache');
  },
});
app.register(multipart, { limits: { fileSize: 48 * 1024 * 1024 } });

/* ---------------- 读：设备与文件 ---------------- */

app.get('/api/devices', async () => ({ devices: repo.listDevices(), dataDir: repo.DATA_DIR }));

app.get('/api/device', async (req, reply) => {
  const d = repo.deviceDetail(String(req.query.id || ''));
  if (!d) return reply.code(404).send({ error: '设备不存在: ' + req.query.id });
  return d;
});

app.get('/api/file', async (req, reply) => {
  const r = repo.readTextFile(req.query.path);
  if (!r) return reply.code(404).send({ error: '文件不存在' });
  if (r.error) return reply.code(400).send({ error: r.error });
  return r;
});

const MIME = { '.pdf': 'application/pdf', '.docx': 'application/vnd.openxmlformats-officedocument.wordprocessingml.document', '.doc': 'application/msword', '.md': 'text/plain; charset=utf-8', '.txt': 'text/plain; charset=utf-8', '.json': 'application/json', '.xlsx': 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet' };
app.get('/api/doc', async (req, reply) => {
  const info = repo.getFileInfo(req.query.path);
  if (!info) return reply.code(404).send({ error: '文件不存在' });
  const ext = path.extname(info.abs).toLowerCase();
  reply.type(MIME[ext] || 'application/octet-stream');
  if (['.pdf', '.docx', '.xlsx', '.doc'].includes(ext)) {
    reply.header('content-disposition', 'inline; filename="' + encodeURIComponent(path.basename(info.abs)) + '"');
  }
  return fs.createReadStream(info.abs);
});

/* ---------------- 写：管理 ---------------- */

app.post('/api/devices', async (req, reply) => {
  const b = req.body || {};
  const errs = repo.validateDevicePayload(b);
  if (errs.length) return reply.code(400).send({ error: errs.join('；') });
  const r = repo.createDevice(b);
  if (r.error) return reply.code(400).send(r);
  return r;
});

app.patch('/api/device', async (req, reply) => {
  const r = repo.updateMeta(String((req.body || {}).id || ''), (req.body || {}).patch || {});
  if (r.error) return reply.code(400).send(r);
  return { ok: true };
});

app.post('/api/device/codecs', async (req, reply) => {
  const b = req.body || {};
  const r = repo.addCodec(String(b.id || ''), b, String(b.code || ''));
  if (r.error) return reply.code(400).send(r);
  return { ok: true, file: r.file };
});

app.post('/api/device/codec-current', async (req, reply) => {
  const { id, file } = req.body || {};
  const r = repo.setCurrent(String(id || ''), String(file || ''));
  if (r.error) return reply.code(400).send(r);
  return { ok: true };
});

app.post('/api/device/codec-status', async (req, reply) => {
  const { id, file, status, basis } = req.body || {};
  const r = repo.setStatus(String(id || ''), String(file || ''), String(status || ''), Array.isArray(basis) ? basis : []);
  if (r.error) return reply.code(400).send(r);
  return { ok: true };
});

app.delete('/api/device/codec', async (req, reply) => {
  const { id, file } = req.body || {};
  const r = repo.deleteCodec(String(id || ''), String(file || ''));
  if (r.error) return reply.code(400).send(r);
  return { ok: true };
});

app.delete('/api/device', async (req, reply) => {
  const { id } = req.body || {};
  if (!id) return reply.code(400).send({ error: '需要 id' });
  const r = repo.deleteDevice(String(id));
  if (r.error) return reply.code(404).send(r);
  return { ok: true, mode: r.mode, trashName: r.trashName || null };
});

app.post('/api/device/samples', async (req, reply) => {
  const { id, samples } = req.body || {};
  const r = repo.saveSamples(String(id || ''), samples);
  if (r.error) return reply.code(400).send(r);
  return { ok: true, count: r.count };
});

app.post('/api/device/docs', async (req, reply) => {
  const data = await req.file();
  if (!data) return reply.code(400).send({ error: '未收到文件' });
  const buf = await data.toBuffer();
  const r = repo.saveDoc(String(req.query.id || ''), data.filename, buf);
  if (r.error) return reply.code(400).send(r);
  return r;
});

/** zip 批量导入：zip 内按目录结构携带若干设备目录（含 decoder.json）。
 *  策略：新增设备直接入库；已存在的设备只补缺文件（js/docs/samples），不覆盖服务器上的台账与已改内容。 */
app.post('/api/import', async (req, reply) => {
  const data = await req.file();
  if (!data) return reply.code(400).send({ error: '未收到文件' });
  if (!/\.zip$/i.test(data.filename)) return reply.code(400).send({ error: '请上传 .zip 包' });
  repo.ensureDataDir();
  let zip;
  try { zip = new AdmZip(await data.toBuffer()); } catch (e) { return reply.code(400).send({ error: 'zip 解析失败: ' + e.message }); }
  const result = { devices: [], skipped: [], filesWritten: 0 };
  // 以 decoder.json 为锚点收集设备目录
  const deviceDirs = new Map(); // dirInZip -> Set(files)
  for (const e of zip.getEntries()) {
    if (e.isDirectory) continue;
    const rel = e.entryName.replace(/\\/g, '/').replace(/^\.\//, '');
    if (rel.split('/').some(p => p.startsWith('.') || p === 'node_modules')) continue;
    const parts = rel.split('/');
    parts.pop();
    let dir = parts.join('/');
    while (dir && !deviceDirs.has(dir)) {
      const probe = zip.getEntry(dir + '/decoder.json') || zip.getEntry(dir + '/');
      if (probe) break;
      const idx = dir.lastIndexOf('/');
      dir = idx < 0 ? '' : dir.slice(0, idx);
    }
    // 找到包含 decoder.json 的最内层目录
    let d = parts.join('/');
    while (d) {
      if (zip.getEntry(d + '/decoder.json')) { if (!deviceDirs.has(d)) deviceDirs.set(d, new Set()); deviceDirs.get(d).add(rel); break; }
      const idx = d.lastIndexOf('/');
      d = idx < 0 ? '' : d.slice(0, idx);
    }
  }
  if (!deviceDirs.size) return reply.code(400).send({ error: 'zip 内未找到任何含 decoder.json 的设备目录' });
  for (const [dirInZip, files] of deviceDirs) {
    // 设备 id = decoder.json 所在目录相对路径；剥掉包内可能的顶层公共前缀已由收集逻辑处理
    const metaEntry = zip.getEntry(dirInZip + '/decoder.json');
    let meta;
    try { meta = JSON.parse(zip.readAsText(metaEntry)); } catch (e) { result.skipped.push(dirInZip + '（decoder.json 无法解析）'); continue; }
    if (!meta.modelKey || !meta.origin) { result.skipped.push(dirInZip + '（元数据缺 modelKey/origin）'); continue; }
    const id = dirInZip.replace(/^decoders\//, '');
    const dstDir = path.join(repo.DEC, id);
    const existed = fs.existsSync(path.join(dstDir, 'decoder.json'));
    fs.mkdirSync(dstDir, { recursive: true });
    for (const rel of files) {
      const inDev = rel.slice(dirInZip.length + 1);
      if (!inDev) continue;
      const dst = path.join(dstDir, inDev);
      if (fs.existsSync(dst)) continue; // 增量：不覆盖
      fs.mkdirSync(path.dirname(dst), { recursive: true });
      fs.writeFileSync(dst, zip.readFile(zip.getEntry(rel)));
      result.filesWritten++;
    }
    if (!existed) {
      // 新设备：直接采用包内台账
      result.devices.push({ id, action: 'created' });
    } else {
      // 已存在：若服务器台账缺少包内新增的 codec 文件，补录 codecs 记录
      const srvMeta = JSON.parse(fs.readFileSync(path.join(dstDir, 'decoder.json'), 'utf8'));
      let changed = false;
      for (const c of (meta.codecs || [])) {
        if (!srvMeta.codecs.some(x => x.file === c.file) && fs.existsSync(path.join(dstDir, c.file))) {
          srvMeta.codecs.push({ ...c, current: false });
          changed = true;
        }
      }
      if (changed) { fs.writeFileSync(path.join(dstDir, 'decoder.json'), JSON.stringify(srvMeta, null, 2) + '\n'); }
      result.devices.push({ id, action: 'merged' });
    }
  }
  return result;
});

/** 全量导出备份（zip 下载） */
app.get('/api/export', async (req, reply) => {
  repo.ensureDataDir();
  const zip = new AdmZip();
  const add = (dir, base) => {
    for (const ent of fs.readdirSync(dir, { withFileTypes: true })) {
      if (ent.name.startsWith('.')) continue;
      const p = path.join(dir, ent.name);
      const rel = base ? base + '/' + ent.name : ent.name;
      if (ent.isDirectory()) add(p, rel); else zip.addLocalFile(p, base || '');
    }
  };
  add(repo.DEC, 'decoders');
  const buf = zip.toBuffer();
  reply.type('application/zip');
  reply.header('content-disposition', 'attachment; filename="hkt-decoders-backup-' + new Date().toISOString().slice(0, 10) + '.zip"');
  return buf;
});

/* ---------------- 解码 ---------------- */

async function decodeOne(deviceId, file, item, globalFPort, timeoutMs, codeCache) {
  const parsed = parsePayload(item.raw);
  const card = { raw: item.raw, format: null, byteLen: null, fPort: null, ok: false, timedOut: false, data: null, errors: [], warnings: [], logs: [], durationMs: null };
  if (item.fPort != null) card.fPort = Number(item.fPort);
  else if (globalFPort != null) card.fPort = Number(globalFPort);
  if (parsed.error) { card.errors = [parsed.error]; return card; }
  card.format = parsed.format;
  card.byteLen = parsed.bytes.length;
  let code = codeCache && codeCache.get(file);
  if (!code) {
    code = repo.readCodecCode(deviceId, file);
    if (code == null) { card.errors = ['解码器文件不存在: ' + file]; return card; }
    if (codeCache) codeCache.set(file, code);
  }
  const t0 = Date.now();
  const r = await runDecode(code, { bytes: parsed.bytes, fPort: card.fPort }, timeoutMs);
  card.durationMs = Date.now() - t0;
  card.timedOut = !!r.timedOut;
  card.logs = r.logs || [];
  if (!r.ok) {
    card.errors = [r.error || '执行失败'];
    if (r.result) { const j = judgeResult(r.result); card.data = j.data; card.warnings = j.warnings; }
    return card;
  }
  const j = judgeResult(r.result);
  card.data = j.data;
  card.errors = j.errors;
  card.warnings = j.warnings;
  card.ok = !j.hasError;
  return card;
}

function loadCodecOr4xx(reply, deviceId, file) {
  const detail = repo.deviceDetail(deviceId);
  if (!detail) { reply.code(404).send({ error: '设备不存在: ' + deviceId }); return null; }
  const known = detail.codecs.find(c => c.file === file);
  if (!known) { reply.code(400).send({ error: '解码器文件不在该设备台账中: ' + file }); return null; }
  const code = repo.readCodecCode(deviceId, file);
  if (code == null) { reply.code(404).send({ error: '解码器文件不存在: ' + file }); return null; }
  return { detail, code };
}

app.post('/api/decode', async (req, reply) => {
  const { device, file, payloads, fPort, timeoutMs } = req.body || {};
  if (!Array.isArray(payloads) || !payloads.length) return reply.code(400).send({ error: 'payloads 不能为空' });
  const ctx = loadCodecOr4xx(reply, String(device || ''), String(file || ''));
  if (!ctx) return reply;
  const t = Number(timeoutMs) > 0 ? Number(timeoutMs) : DEFAULT_TIMEOUT;
  const cache = new Map();
  const results = [];
  for (const item of payloads.slice(0, 200)) {
    results.push(await decodeOne(String(device), String(file), item || {}, fPort == null ? null : Number(fPort), t, cache));
  }
  return { results, summary: { total: results.length, ok: results.filter(r => r.ok).length, failed: results.filter(r => !r.ok).length } };
});

/* ---------------- 黄金样例回归 ---------------- */

app.post('/api/regression', async (req, reply) => {
  const { device, file } = req.body || {};
  const detail = repo.deviceDetail(String(device || ''));
  if (!detail) return reply.code(404).send({ error: '设备不存在' });
  const samples = detail.samples || [];
  if (!samples.length) return { cases: [], summary: { total: 0, pass: 0, fail: 0, note: '该设备暂无黄金样例' } };
  const current = detail.codecs.filter(c => c.current).map(c => c.file);
  const all = detail.codecs.map(c => c.file);
  const cache = new Map();
  const cases = [];
  for (const s of samples) {
    const files = file ? [String(file)] : (s.forFile ? [s.forFile] : (current.length ? current : all));
    for (const f of files) {
      const row = { name: s.name, source: s.source, file: f, payload: s.payload, fPort: s.fPort, ok: false, errors: [], expected: s.expectData || null, got: null, note: s.note || '' };
      const codec = repo.readCodecCode(String(device), f);
      if (codec == null) { row.errors = ['解码器文件缺失: ' + f]; cases.push(row); continue; }
      const r = await decodeOne(String(device), f, { raw: s.payload, fPort: s.fPort }, s.fPort, DEFAULT_TIMEOUT, cache);
      row.got = r.data;
      row.rawErrors = r.errors;
      const parsed = parsePayload(s.payload);
      if (parsed.error) { row.errors = ['样例负载非法: ' + parsed.error]; cases.push(row); continue; }
      if (r.timedOut) { row.errors = ['执行超时']; cases.push(row); continue; }
      const hasErr = r.errors.length > 0;
      if (s.expectError === true && !hasErr) row.errors = ['期望报错，但解码未报错'];
      if (s.expectError === false && hasErr) row.errors = ['意外报错: ' + r.errors.join('; ')];
      if (s.expectData != null && !deepEqual(r.data, s.expectData)) {
        row.errors.push('输出与期望不一致');
        row.diff = diffKeys(s.expectData, r.data);
      }
      row.ok = row.errors.length === 0;
      cases.push(row);
    }
  }
  return { cases, summary: { total: cases.length, pass: cases.filter(c => c.ok).length, fail: cases.filter(c => !c.ok).length } };
});

function diffKeys(expected, got) {
  const diffs = [];
  const keys = new Set([...Object.keys(expected || {}), ...Object.keys(got || {})]);
  for (const k of keys) {
    const a = expected ? expected[k] : undefined;
    const b = got ? got[k] : undefined;
    if (stableStringify(a) !== stableStringify(b)) diffs.push({ key: k, expected: a === undefined ? null : a, got: b === undefined ? null : b });
  }
  return diffs;
}

/* ---------------- 版本对比 ---------------- */

app.post('/api/diff', async (req, reply) => {
  const { device, fileA, fileB, payloads, fPort } = req.body || {};
  if (!Array.isArray(payloads) || !payloads.length) return reply.code(400).send({ error: 'payloads 不能为空' });
  if (!fileA || !fileB) return reply.code(400).send({ error: '需要选择两个版本' });
  const ctxA = loadCodecOr4xx(reply, String(device), String(fileA)); if (!ctxA) return reply;
  const ctxB = loadCodecOr4xx(reply, String(device), String(fileB)); if (!ctxB) return reply;
  const results = [];
  const cache = new Map();
  for (const item of payloads.slice(0, 200)) {
    const a = await decodeOne(String(device), fileA, item, fPort == null ? null : Number(fPort), DEFAULT_TIMEOUT, cache);
    const b = await decodeOne(String(device), fileB, item, fPort == null ? null : Number(fPort), DEFAULT_TIMEOUT, cache);
    results.push({
      raw: item.raw,
      a: { file: fileA, ...a },
      b: { file: fileB, ...b },
      equal: stableStringify(a.data) === stableStringify(b.data) && a.ok === b.ok,
    });
  }
  return { results, summary: { total: results.length, same: results.filter(r => r.equal).length, changed: results.filter(r => !r.equal).length } };
});

app.setNotFoundHandler((req, reply) => {
  if (req.raw.url && req.raw.url.startsWith('/api/')) return reply.code(404).send({ error: '接口不存在' });
  return reply.code(200).sendFile('index.html');
});

repo.ensureDataDir();
app.listen({ port: PORT, host: '0.0.0.0' }).then(() => console.log(`HKT 负载解码器管理调试平台: http://0.0.0.0:${PORT}  数据目录: ${repo.DATA_DIR}`));
