'use strict';
/** 解码器仓库访问：索引扫描、设备详情、路径安全 */
const fs = require('fs');
const path = require('path');

const ROOT = process.env.DECODERS_ROOT ? path.resolve(process.env.DECODERS_ROOT) : path.resolve(__dirname, '..', '..');
const DEC = path.join(ROOT, 'decoders');

function safeJoin(rel) {
  const p = path.resolve(ROOT, String(rel || ''));
  if (p !== ROOT && !p.startsWith(ROOT + path.sep)) return null;
  return p;
}

function readJson(p) {
  try { return JSON.parse(fs.readFileSync(p, 'utf8')); } catch (e) { return null; }
}

/** 扫描 decoders/ 下全部 decoder.json，返回设备索引（含派生统计） */
function listDevices() {
  const out = [];
  if (!fs.existsSync(DEC)) return out;
  (function walk(dir) {
    for (const ent of fs.readdirSync(dir, { withFileTypes: true }).sort((a, b) => a.name.localeCompare(b.name))) {
      if (!ent.isDirectory() || ent.name.startsWith('.')) continue;
      const full = path.join(dir, ent.name);
      const metaPath = path.join(full, 'decoder.json');
      if (fs.existsSync(metaPath)) {
        const meta = readJson(metaPath);
        if (meta) out.push(summary(path.relative(DEC, full).replace(/\\/g, '/'), meta, full));
      } else {
        walk(full);
      }
    }
  })(DEC);
  return out;
}

function summary(id, meta, full) {
  const codecs = Array.isArray(meta.codecs) ? meta.codecs : [];
  const auth = meta.authority || {};
  let docExists = false;
  if (auth.type === 'protocol-doc' && auth.file) docExists = fs.existsSync(path.join(full, auth.file));
  let sampleCount = 0;
  const sp = path.join(full, 'samples', 'samples.json');
  if (fs.existsSync(sp)) { const s = readJson(sp); if (Array.isArray(s)) sampleCount = s.length; }
  return {
    id,
    model: meta.model,
    modelKey: meta.modelKey,
    aliases: meta.aliases || [],
    name: meta.name,
    origin: meta.origin,
    vendor: meta.vendor,
    category: meta.category,
    notes: meta.notes || '',
    authority: { ...auth, exists: auth.type === 'firmware' ? true : docExists },
    fPortDefault: meta.fPortDefault == null ? null : meta.fPortDefault,
    deviations: meta.deviations || [],
    codecs,
    stats: {
      codecCount: codecs.length,
      currentCount: codecs.filter(c => c.current).length,
      verifiedCount: codecs.filter(c => c.status === 'verified').length,
      unverifiedCount: codecs.filter(c => c.status !== 'verified').length,
      platforms: [...new Set(codecs.map(c => c.platform))],
      sampleCount,
      missingAuthority: auth.missing === true || (auth.type === 'protocol-doc' && auth.file && !docExists),
    },
  };
}

/** 设备详情：元数据 + 黄金样例 + 文件清单 */
function deviceDetail(id) {
  const dir = safeJoin(path.join('decoders', id));
  if (!dir || !fs.existsSync(path.join(dir, 'decoder.json'))) return null;
  const meta = readJson(path.join(dir, 'decoder.json'));
  const sum = summary(id, meta, dir);
  const samples = readJson(path.join(dir, 'samples', 'samples.json')) || [];
  const files = [];
  (function walk(d, rel) {
    for (const ent of fs.readdirSync(d, { withFileTypes: true }).sort((a, b) => a.name.localeCompare(b.name))) {
      if (ent.name.startsWith('.')) continue;
      const r = rel ? rel + '/' + ent.name : ent.name;
      if (ent.isDirectory()) walk(path.join(d, ent.name), r);
      else files.push(r);
    }
  })(dir, '');
  return { ...sum, samples, files };
}

/** 读文本文件（代码查看），限制大小 */
function readTextFile(rel, cap = 512 * 1024) {
  const p = safeJoin(rel);
  if (!p || !fs.existsSync(p) || !fs.statSync(p).isFile()) return null;
  if (fs.statSync(p).size > cap) return { error: '文件过大' };
  return { content: fs.readFileSync(p, 'utf8') };
}

/** 文档/数据文件流（协议手册查看） */
function getFileInfo(rel) {
  const p = safeJoin(rel);
  if (!p || !fs.existsSync(p) || !fs.statSync(p).isFile()) return null;
  return { abs: p, size: fs.statSync(p).size };
}

/** 读取设备目录下解码器代码（供沙箱执行） */
function readCodecCode(deviceId, file) {
  const p = safeJoin(path.join('decoders', deviceId, file));
  if (!p || !fs.existsSync(p)) return null;
  if (!/\.js$/i.test(p)) return null;
  return fs.readFileSync(p, 'utf8');
}

module.exports = { ROOT, DEC, listDevices, deviceDetail, readTextFile, getFileInfo, readCodecCode, safeJoin };
