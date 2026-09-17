'use strict';
/**
 * 解码器执行沙箱：每条负载在独立 worker 线程中运行，
 * worker 内用 vm.runInNewContext 的同步超时兜底死循环，
 * 父进程再用硬超时 terminate 兜底（双保险，IRC01 截断帧死循环教训）。
 */
const { Worker } = require('worker_threads');

const WORKER_SRC = `
const { workerData, parentPort } = require('worker_threads');
const vm = require('vm');
const { code, input, timeoutMs } = workerData;
const logs = [];
const capture = (level) => (...args) => {
  try { logs.push(level + ': ' + args.map(a => (typeof a === 'string' ? a : JSON.stringify(a))).join(' ')); } catch (e) { /* ignore */ }
};
const sandbox = {
  module: { exports: {} },
  console: { log: capture('log'), warn: capture('warn'), error: capture('error'), info: capture('log') },
  JSON, Math, Object, Array, String, Number, Boolean, Date, RegExp, Error, TypeError, RangeError,
  parseInt, parseFloat, isNaN, isFinite, encodeURIComponent, decodeURIComponent,
};
try {
  vm.runInNewContext(code, sandbox, { timeout: timeoutMs, displayErrors: true });
  let fn = sandbox.decodeUplink;
  if (typeof fn !== 'function' && typeof sandbox.Decoder === 'function') {
    const D = sandbox.Decoder;
    fn = (i) => {
      const port = i.fPort === undefined || i.fPort === null ? 0 : i.fPort;
      const r = D(i.bytes, port);
      if (r && typeof r === 'object' && !Array.isArray(r) && (r.data !== undefined || r.errors !== undefined)) return r;
      return { data: r };
    };
  }
  if (typeof fn !== 'function') throw new Error('脚本未提供 decodeUplink/Decoder 入口');
  const result = fn(input);
  parentPort.postMessage({ ok: true, result, logs });
} catch (e) {
  parentPort.postMessage({ ok: false, error: String((e && e.message) || e), logs });
}
`;

function runDecode(code, input, timeoutMs = 3000) {
  return new Promise((resolve) => {
    let settled = false;
    const finish = (r) => { if (!settled) { settled = true; resolve(r); } };
    let worker;
    try {
      worker = new Worker(WORKER_SRC, { eval: true, workerData: { code, input, timeoutMs } });
    } catch (e) {
      return finish({ ok: false, error: 'worker 启动失败: ' + String((e && e.message) || e), logs: [] });
    }
    const t0 = Date.now();
    const timer = setTimeout(() => {
      worker.terminate().finally(() =>
        finish({ ok: false, timedOut: true, error: '执行超时（>' + timeoutMs + 'ms），已强制终止（疑似死循环）', logs: [] }));
    }, timeoutMs + 500);
    const looksTimeout = (msg) => /timed out|超时/i.test(String((msg && msg.error) || ''));
    worker.on('message', (m) => { clearTimeout(timer); if (m && m.ok === false && looksTimeout(m)) m.timedOut = true; finish(m); });
    worker.on('error', (e) => { clearTimeout(timer); finish({ ok: false, error: String((e && e.message) || e), logs: [] }); });
    worker.on('exit', () => {
      clearTimeout(timer);
      // vm 的 timeout 会直接终止 worker 线程（走不到 catch/message），按耗时判归为超时熔断
      if (Date.now() - t0 >= timeoutMs) {
        finish({ ok: false, timedOut: true, error: '执行超时（>' + timeoutMs + 'ms），已强制终止（疑似死循环）', logs: [] });
      } else {
        finish({ ok: false, error: 'worker 异常退出（无结果）', logs: [] });
      }
    });
  });
}

module.exports = { runDecode };
