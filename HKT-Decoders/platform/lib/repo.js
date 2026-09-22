'use strict';
/**
 * 解码器库访问层：平台专属数据目录（DATA_DIR）为解码器唯一权威存储。
 * 读：索引/详情/代码/文档；写：建设备/新增版本/台账编辑/样例/文档/删除（移入回收站）。
 */
const fs = require('fs');
const path = require('path');

// 数据目录：默认仓库内 platform-data/（本地开发），服务器用卷挂载 /data
const DATA_DIR = process.env.DATA_DIR ? path.resolve(process.env.DATA_DIR) : path.resolve(__dirname, '..', 'platform-data');
const DEC = path.join(DATA_DIR, 'decoders');
const TRASH = path.join(DATA_DIR, '.trash');

function ensureDataDir() {
  fs.mkdirSync(DEC, { recursive: true });
  fs.mkdirSync(TRASH, { recursive: true });
}

function safeJoin(rel) {
  const p = path.resolve(DEC, normalizeRel(rel));
  if (p !== DEC && !p.startsWith(DEC + path.sep)) return null;
  return p;
}

/** 兼容两种入参：相对 DEC 的路径，或带 decoders/ 前缀的完整仓库内路径 */
function normalizeRel(rel) {
  let s = String(rel || '').replace(/\\/g, '/');
  if (s === 'decoders' || s.startsWith('decoders/')) s = s.slice('decoders/'.length);
  return s;
}

function readJson(p) {
  try { return JSON.parse(fs.readFileSync(p, 'utf8')); } catch (e) { return null; }
}

function writeJson(p, obj) {
  fs.mkdirSync(path.dirname(p), { recursive: true });
  fs.writeFileSync(p, JSON.stringify(obj, null, 2) + '\n');
}

/* ---------------- 读 ---------------- */

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

function deviceDetail(id) {
  const dir = safeJoin(id);
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

function readTextFile(rel, cap = 512 * 1024) {
  const p = path.resolve(DEC, normalizeRel(rel));
  if (!p.startsWith(DEC + path.sep) || !fs.existsSync(p) || !fs.statSync(p).isFile()) return null;
  if (fs.statSync(p).size > cap) return { error: '文件过大' };
  return { content: fs.readFileSync(p, 'utf8') };
}

function getFileInfo(rel) {
  const p = path.resolve(DEC, normalizeRel(rel));
  if (!p.startsWith(DEC + path.sep) || !fs.existsSync(p) || !fs.statSync(p).isFile()) return null;
  return { abs: p, size: fs.statSync(p).size };
}

function readCodecCode(deviceId, file) {
  const p = safeJoin(path.join(deviceId, file));
  if (!p || !fs.existsSync(p) || !/\.js$/i.test(p)) return null;
  return fs.readFileSync(p, 'utf8');
}

function getMeta(id) {
  const dir = safeJoin(id);
  if (!dir) return null;
  return { dir, meta: readJson(path.join(dir, 'decoder.json')) };
}

/* ---------------- 写 ---------------- */

const NAME_RE = /^[a-z0-9][a-z0-9-]*$/;
const FILE_RE = /^[a-z0-9][a-z0-9-]*_(chirpstack|ttn|thingsboard|private)(_(cn|en))?_v\d+\.\d+\.\d+\.js$/;
const VER_RE = /^\d+\.\d+\.\d+$/;

function validateDevicePayload(b) {
  const errs = [];
  if (!b.modelKey || !NAME_RE.test(b.modelKey)) errs.push('modelKey 需为小写字母/数字/连字符');
  if (!b.model || !String(b.model).trim()) errs.push('model 必填');
  if (!b.name || !String(b.name).trim()) errs.push('name 必填');
  if (!['in-house', 'out-sourced'].includes(b.origin)) errs.push('origin 必须是 in-house 或 out-sourced');
  const auth = b.authority || {};
  if (!['firmware', 'protocol-doc'].includes(auth.type)) errs.push('authority.type 必须是 firmware 或 protocol-doc');
  if (auth.type === 'firmware' && !(auth.path || '').trim()) errs.push('自研设备必须填固件代码路径');
  if (auth.type === 'protocol-doc' && !(auth.file || '').trim() && !auth.missing) errs.push('采购设备必须上传或指定协议文档（或明确标记 missing 并说明）');
  return errs;
}

/** 新建设备目录 + decoder.json；out-sourced 时 vendorKey 决定厂商目录名（默认 oem） */
function createDevice(b) {
  ensureDataDir();
  const vendorKey = String(b.vendorKey || '').trim() || 'oem';
  if (vendorKey !== 'oem' && !NAME_RE.test(vendorKey)) return { error: 'vendorKey 需为小写字母/数字/连字符' };
  const id = (b.origin === 'in-house' ? 'in-house/' : 'out-sourced/' + vendorKey + '/') + b.modelKey;
  const dir = path.join(DEC, id);
  if (fs.existsSync(dir)) return { error: '设备已存在: ' + id };
  // 型号/别名精确查重（大小写、空格、连字符不敏感）——拦截 B1325L 这类换位录重
  const norm = s => String(s || '').toLowerCase().replace(/[\s-]+/g, '');
  const newNames = [norm(b.model), norm(b.modelKey), ...(Array.isArray(b.aliases) ? b.aliases : []).map(norm)].filter(Boolean);
  for (const s of listDevices()) {
    const oldNames = [norm(s.model), norm(s.modelKey), ...(s.aliases || []).map(norm)].filter(Boolean);
    if (oldNames.some(x => newNames.includes(x))) return { error: '型号/别名与现有设备重复: ' + s.model + '（' + s.id + '）。如确为两台不同设备，请先核对型号' };
  }
  const meta = {
    model: String(b.model).trim(),
    modelKey: b.modelKey,
    aliases: Array.isArray(b.aliases) ? b.aliases : [],
    name: String(b.name).trim(),
    origin: b.origin,
    vendor: String(b.vendor || '').trim() || (b.origin === 'in-house' ? 'HKT' : 'OEM（待补）'),
    category: String(b.category || '').trim() || '未分类',
    authority: b.authority.type === 'firmware'
      ? { type: 'firmware', path: String(b.authority.path).trim(), ref: String(b.authority.ref || '').trim() || null }
      : { type: 'protocol-doc', file: String(b.authority.file || '').trim() || null, section: String(b.authority.section || '').trim() || null, missing: !!b.authority.missing, note: String(b.authority.note || '').trim() || null },
    fPortDefault: b.fPortDefault == null || b.fPortDefault === '' ? null : Number(b.fPortDefault),
    codecs: [],
    deviations: [],
    notes: String(b.notes || '').trim(),
  };
  fs.mkdirSync(path.join(dir, 'docs'), { recursive: true });
  writeJson(path.join(dir, 'decoder.json'), meta);
  return { id, meta };
}

/** 编辑台账（白名单字段），返回更新后的 meta */
function updateMeta(id, patch) {
  const { dir, meta } = getMeta(id) || {};
  if (!meta) return { error: '设备不存在' };
  if (patch.name !== undefined) meta.name = String(patch.name).trim();
  if (patch.model !== undefined) meta.model = String(patch.model).trim();
  if (patch.vendor !== undefined) meta.vendor = String(patch.vendor).trim();
  if (patch.category !== undefined) meta.category = String(patch.category).trim();
  if (patch.notes !== undefined) meta.notes = String(patch.notes).trim();
  if (patch.fPortDefault !== undefined) meta.fPortDefault = patch.fPortDefault === null || patch.fPortDefault === '' ? null : Number(patch.fPortDefault);
  if (patch.aliases !== undefined) meta.aliases = String(patch.aliases).split(/[,，\s]+/).map(s => s.trim()).filter(Boolean);
  if (patch.authority !== undefined && patch.authority.type) {
    const a = patch.authority;
    if (a.type === 'firmware') meta.authority = { type: 'firmware', path: String(a.path || '').trim(), ref: String(a.ref || '').trim() || null };
    else meta.authority = { type: 'protocol-doc', file: String(a.file || '').trim() || null, section: String(a.section || '').trim() || null, missing: !!a.missing, note: String(a.note || '').trim() || null };
  }
  writeJson(path.join(dir, 'decoder.json'), meta);
  return { meta };
}

/** 新增解码器版本（写 js 文件 + 更新 codecs），mode: paste(代码) 或 upload(内容) */
function addCodec(id, b, codeText) {
  const { dir, meta } = getMeta(id) || {};
  if (!meta) return { error: '设备不存在' };
  const platform = b.platform, lang = b.lang || null;
  if (!['chirpstack', 'ttn', 'thingsboard', 'private'].includes(platform)) return { error: 'platform 必须是 chirpstack/ttn/thingsboard/private' };
  if (lang !== null && !['cn', 'en'].includes(lang)) return { error: 'lang 只能是 cn/en 或空' };
  if (!VER_RE.test(b.version || '')) return { error: '版本号需为 x.y.z 三段数字' };
  if (!String(b.changelog || '').trim()) return { error: 'changelog 必填（变更说明）' };
  if (!codeText || !codeText.trim()) return { error: '解码器代码为空' };
  const file = `${meta.modelKey}_${platform}${lang ? '_' + lang : ''}_v${b.version}.js`;
  if (!FILE_RE.test(file)) return { error: '文件名不符合规范: ' + file };
  if (meta.codecs.some(c => c.file === file)) return { error: '该文件已存在（版本重复）: ' + file };
  // 轻度校验：必须能解析且含入口
  if (!/function\s+decodeUplink|function\s+Decoder\s*\(/.test(codeText)) return { error: '代码中未找到 decodeUplink 或 Decoder 入口函数' };
  fs.writeFileSync(path.join(dir, file), codeText);
  const sameSlot = meta.codecs.filter(c => c.platform === platform && (c.lang || null) === lang);
  meta.codecs.push({
    platform, lang, file,
    decoderVersion: b.version,
    firmwareVersion: String(b.firmwareVersion || '').trim() || null,
    current: sameSlot.length === 0, // 该平台+语言首个版本自动 current
    status: 'unverified',
    basis: [],
    verifiedAt: null,
    changelog: String(b.changelog).trim(),
  });
  writeJson(path.join(dir, 'decoder.json'), meta);
  return { file, meta };
}

/** 同平台+语言内切换 current；返回受影响的文件列表 */
function setCurrent(id, file) {
  const { dir, meta } = getMeta(id) || {};
  if (!meta) return { error: '设备不存在' };
  const target = meta.codecs.find(c => c.file === file);
  if (!target) return { error: '版本不存在: ' + file };
  for (const c of meta.codecs) {
    if (c.platform === target.platform && (c.lang || null) === (target.lang || null)) c.current = c.file === file;
  }
  writeJson(path.join(dir, 'decoder.json'), meta);
  return { meta };
}

/** 核验状态；verified 必须带 basis */
function setStatus(id, file, status, basis) {
  const { dir, meta } = getMeta(id) || {};
  if (!meta) return { error: '设备不存在' };
  const c = meta.codecs.find(x => x.file === file);
  if (!c) return { error: '版本不存在: ' + file };
  if (!['verified', 'unverified'].includes(status)) return { error: 'status 只能是 verified/unverified' };
  if (status === 'verified' && (!Array.isArray(basis) || !basis.length)) return { error: '标记已核验必须填写核验依据（basis）' };
  c.status = status;
  c.basis = status === 'verified' ? basis.map(String) : [];
  c.verifiedAt = status === 'verified' ? new Date().toISOString().slice(0, 10) : null;
  writeJson(path.join(dir, 'decoder.json'), meta);
  return { meta };
}

/** 删除版本：移入 .trash/（不直接销毁）。current 也可删（强确认在前端），
 *  删除后该平台+语言槽可能无 current（设备可正常展示，回归跳过该槽）。 */
function deleteCodec(id, file) {
  const { dir, meta } = getMeta(id) || {};
  if (!meta) return { error: '设备不存在' };
  const c = meta.codecs.find(x => x.file === file);
  if (!c) return { error: '版本不存在: ' + file };
  const src = path.join(dir, file);
  if (fs.existsSync(src)) {
    const dst = path.join(TRASH, Date.now() + '_' + file.replace(/\//g, '_'));
    fs.renameSync(src, dst);
  }
  meta.codecs = meta.codecs.filter(x => x.file !== file);
  // 清理绑定到被删文件的样例指向，回退为"跑全部 current"逻辑
  const sp = path.join(dir, 'samples', 'samples.json');
  if (fs.existsSync(sp)) {
    try {
      const samples = JSON.parse(fs.readFileSync(sp, 'utf8'));
      let changed = false;
      for (const s of samples) { if (s.forFile === file) { delete s.forFile; changed = true; } }
      if (changed) writeJson(sp, samples);
    } catch (e) { /* 样例损坏时不阻塞删除 */ }
  }
  writeJson(path.join(dir, 'decoder.json'), meta);
  return { meta, wasCurrent: !!c.current };
}

/** 保存黄金样例（整体替换，JSON 结构校验） */
function saveSamples(id, samples) {
  const meta = getMeta(id);
  if (!meta || !fs.existsSync(path.join(meta.dir, 'decoder.json'))) return { error: '设备不存在: ' + id };
  const dir = meta.dir;
  if (!Array.isArray(samples)) return { error: 'samples 必须是数组' };
  for (let i = 0; i < samples.length; i++) {
    const s = samples[i];
    if (!s.name || !s.payload) return { error: `第 ${i + 1} 条样例缺少 name 或 payload` };
    if (!/^[0-9a-fA-F\s:]+$/.test(s.payload)) return { error: `第 ${i + 1} 条样例 payload 需为 hex` };
    if (!['manual', 'firmware', 'field', 'regression'].includes(s.source)) return { error: `第 ${i + 1} 条样例 source 需为 manual/firmware/field/regression` };
  }
  writeJson(path.join(dir, 'samples', 'samples.json'), samples);
  return { count: samples.length };
}

/** 上传协议文档到 docs/ */
function saveDoc(id, filename, buf) {
  const { dir, meta } = getMeta(id) || {};
  if (!meta) return { error: '设备不存在' };
  const safe = String(filename).replace(/[/\\]/g, '_');
  if (!/\.(pdf|docx?|md|txt|xlsx)$/i.test(safe)) return { error: '仅支持 pdf/docx/doc/md/txt/xlsx' };
  if (buf.length > 40 * 1024 * 1024) return { error: '文件超过 40MB 限制' };
  fs.mkdirSync(path.join(dir, 'docs'), { recursive: true });
  fs.writeFileSync(path.join(dir, 'docs', safe), buf);
  // 若 authority 指向的文档缺失，自动补上
  if (meta.authority.type === 'protocol-doc' && (!meta.authority.file || meta.authority.missing)) {
    meta.authority.file = 'docs/' + safe;
    meta.authority.missing = false;
    writeJson(path.join(dir, 'decoder.json'), meta);
  }
  return { file: 'docs/' + safe };
}

/** 删除设备：无实质内容（无解码器，且除 decoder.json 外无任何文件）直接删除目录；
 *  有内容则整体移入 .trash/（与删版本同一回收站，不直接销毁）。
 *  删除后若厂商目录（out-sourced/<vendor>/）因此变空则一并清理。 */
function deleteDevice(id) {
  const dir = safeJoin(id);
  if (!dir || !fs.existsSync(path.join(dir, 'decoder.json'))) return { error: '设备不存在: ' + id };
  const detail = deviceDetail(id);
  const hasContent = detail.codecs.length > 0 || detail.files.some(f => f !== 'decoder.json');
  const parent = path.dirname(dir);
  const pruneVendor = () => {
    // 只清理 out-sourced/<vendor>/ 这一层空目录；DEC 根与 in-house/ 不动
    if (path.dirname(parent) !== DEC && fs.existsSync(parent) && fs.readdirSync(parent).length === 0) fs.rmdirSync(parent);
  };
  if (!hasContent) {
    fs.rmSync(dir, { recursive: true, force: true });
    pruneVendor();
    return { mode: 'purged' };
  }
  fs.mkdirSync(TRASH, { recursive: true });
  const trashName = Date.now() + '_device_' + String(id).replace(/\//g, '_');
  fs.renameSync(dir, path.join(TRASH, trashName));
  pruneVendor();
  return { mode: 'trashed', trashName };
}

module.exports = {
  DATA_DIR, DEC, ensureDataDir, safeJoin,
  listDevices, deviceDetail, readTextFile, getFileInfo, readCodecCode,
  getMeta, validateDevicePayload, createDevice, updateMeta,
  addCodec, setCurrent, setStatus, deleteCodec, deleteDevice, saveSamples, saveDoc,
  FILE_RE, VER_RE, NAME_RE,
};
