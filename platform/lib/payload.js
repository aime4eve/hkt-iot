'use strict';
/** 负载输入解析与结果判定工具 */

/** 解析单条负载文本 → { format, bytes } 或 { error } */
function parsePayload(raw) {
  let s = String(raw == null ? '' : raw).trim();
  if (!s) return { error: '空负载' };
  let format = null;
  const m = s.match(/^(hex|b64|base64)\s*:\s*([\s\S]*)$/i);
  if (m) {
    format = /^b/i.test(m[1]) ? 'base64' : 'hex';
    s = m[2].trim();
  }
  const compact = s.replace(/[\s,:-]/g, '').replace(/^0x/i, '');
  if (!format) {
    format = (/^[0-9a-fA-F]+$/.test(compact) && compact.length % 2 === 0) ? 'hex' : 'base64';
  }
  if (format === 'hex') {
    if (!compact.length) return { error: '空负载' };
    if (!/^[0-9a-fA-F]+$/.test(compact) || compact.length % 2 !== 0) {
      return { error: '非法 hex（含非十六进制字符或长度为奇数）' };
    }
    return { format: 'hex', bytes: [...Buffer.from(compact, 'hex')] };
  }
  const b64 = s.replace(/\s+/g, '');
  if (!b64.length) return { error: '空负载' };
  if (!/^[A-Za-z0-9+/]+={0,2}$/.test(b64)) return { error: '非法 base64 字符' };
  const buf = Buffer.from(b64, 'base64');
  if (!buf.length) return { error: 'base64 解码结果为空' };
  return { format: 'base64', bytes: [...buf] };
}

/** 稳定序列化（键排序），用于深比较与展示 */
function stableStringify(v) {
  if (v === null || typeof v !== 'object') return JSON.stringify(v);
  if (Array.isArray(v)) return '[' + v.map(stableStringify).join(',') + ']';
  return '{' + Object.keys(v).sort().map(k => JSON.stringify(k) + ':' + stableStringify(v[k])).join(',') + '}';
}
function deepEqual(a, b) { return stableStringify(a) === stableStringify(b); }

/** 归一化解码结果：返回 { hasError, errors, data, warnings } */
function judgeResult(r) {
  const data = (r && typeof r === 'object' && r.data !== undefined) ? r.data : r;
  const errors = [];
  const warnings = [];
  if (r && Array.isArray(r.errors)) errors.push(...r.errors);
  if (data && typeof data === 'object' && !Array.isArray(data) && typeof data.error === 'string') errors.push(data.error);
  if (r && Array.isArray(r.warnings)) warnings.push(...r.warnings);
  return { hasError: errors.length > 0, errors, warnings, data: data === undefined ? null : data };
}

module.exports = { parsePayload, stableStringify, deepEqual, judgeResult };
