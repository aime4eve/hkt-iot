'use strict';
/** HKT 负载解码器调试平台 — Fastify 服务 */
const path = require('path');
const { execFile } = require('child_process');
const fs = require('fs');
const Fastify = require('fastify');
const fastifyStatic = require('@fastify/static');

const repo = require('./lib/repo');
const { runDecode } = require('./lib/sandbox');
const { parsePayload, stableStringify, deepEqual, judgeResult } = require('./lib/payload');

const PORT = Number(process.env.PORT || 8620);
const DEFAULT_TIMEOUT = Number(process.env.DECODE_TIMEOUT_MS || 3000);

const app = Fastify({ bodyLimit: 8 * 1024 * 1024, logger: false });
app.register(fastifyStatic, { root: path.join(__dirname, 'web'), prefix: '/' });

/* ---------------- 设备与文件 ---------------- */

app.get('/api/devices', async () => ({ devices: repo.listDevices(), generatedAt: new Date().toISOString() }));

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
  if (ext === '.pdf' || ext === '.docx' || ext === '.xlsx' || ext === '.doc') {
    reply.header('content-disposition', 'inline; filename="' + encodeURIComponent(path.basename(info.abs)) + '"');
  }
  return fs.createReadStream(info.abs);
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
  if (!samples.length) return { cases: [], summary: { total: 0, pass: 0, fail: 0, note: '该设备暂无黄金样例（迁移债务台账）' } };
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

/* ---------------- 仓库同步 ---------------- */

app.post('/api/sync', async (req, reply) => {
  if (!fs.existsSync(path.join(repo.ROOT, '.git'))) return reply.code(400).send({ error: '解码器目录不是 git 仓库' });
  return new Promise((resolve) => {
    execFile('git', ['-C', repo.ROOT, 'pull', '--ff-only'], { timeout: 30000 }, (err, stdout, stderr) => {
      resolve({ ok: !err, output: (stdout || '') + (stderr || ''), error: err ? String(err.message) : null });
    });
  });
});

app.setNotFoundHandler((req, reply) => {
  if (req.raw.url && req.raw.url.startsWith('/api/')) return reply.code(404).send({ error: '接口不存在' });
  return reply.code(200).sendFile('index.html');
});

app.listen({ port: PORT, host: '0.0.0.0' }).then(() => console.log(`HKT 负载解码器调试平台: http://0.0.0.0:${PORT}  仓库根: ${repo.ROOT}`));
